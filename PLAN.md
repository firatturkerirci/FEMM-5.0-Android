# FEMM 5.0 Android Port Planı

Kaynak: [dcm3c/femm-qt](https://github.com/dcm3c/femm-qt) (FEMM 5.0, sürüm 5.0.20261021).
Bu plan, Ekim 2026'daki `main` dalının incelenmesine dayanır. Kod henüz Android için
derlenmedi; aşağıdaki "engel" listesi kaynak kod okunarak çıkarıldı.

## 0. Kararlar

| Konu | Karar |
|---|---|
| Hedef cihaz | Tablet (yatay ekran öncelikli) |
| Ana kullanım | Yapay zekaya (uygulama dışında) yazdırılan Python betiğini uygulamada çalıştırıp çözdürmek. Elle geometri çizimi öncelik değil. |
| Yapay zeka | Uygulamaya gömülmeyecek. Uygulama, betiği almayı, çalıştırmayı ve hatayı geri kopyalamayı kolaylaştıracak. |
| Dağıtım | GitHub Releases üzerinden imzalı APK |
| Upstream | Genel işe yarayan değişiklikler `dcm3c/femm-qt`'ye PR olarak gönderilecek |

## 1. Mevcut kodun durumu

femm-qt, FEMM 4.2'nin MFC'den Qt 6'ya taşınmış hâli. Katmanlı ve taşınabilir bir yapısı var:

| Klasör | İçerik | Satır (yaklaşık) | Qt bağımlılığı | Android için durum |
|---|---|---|---|---|
| `solvers/` | `fkn` (manyetik), `belasolv` (elektrostatik), `hsolv` (ısı), `csolv` (akım) | 14 bin | Yok | Saf C++17, NDK ile derlenmeli |
| `tangle/` | Mesh üretici (Triangle yerine) | 6 bin | Yok | x86'ya özel kısım `#if` ile korunuyor |
| `libfemm/` | Belge modeli, son işlem, betik motoru | 28 bin | Yok | Çekirdek; alt süreç başlatma kodu var |
| `common/` | Ortak matematik | 4 bin | Yok | Sorunsuz olmalı |
| `ipc/` | TCP sunucusu, `femm_headless` | 1 bin | Yok | Gerekmez |
| `gui/` | Qt 6 Widgets arayüzü | 20 bin | Widgets | Tablette kullanılabilir, uyarlama gerekir |
| `micropython/` + `third_party/bdwgc` | Gömülü MicroPython + Boehm GC + ulab | 96 bin (çoğu üçüncü taraf) | Yok | Süreç içinde çalışıyor, taşınabilir |
| `pybridge/`, `pylib/` | Ayrı süreçte CPython (matplotlib için) | 4 bin | Yok | Android'de çalışmaz, kapatılacak |

Derleme sistemi CMake 3.20+, C++17. Qt yoksa proje "headless" derleniyor.

### Betik çalıştırma için iyi haberler

- Gömülü MicroPython, `import femm; femm.mi_drawline(...)` biçimindeki **standart pyFEMM
  betiklerini değiştirmeden** çalıştırıyor (`libfemm/py_init_script.cpp`, `femm` modülü bir
  vekil; `openfemm`/`closefemm` zararsız boş fonksiyonlar). Yapay zeka araçları pyFEMM'i iyi
  bildiği için ürettikleri kod büyük ölçüde doğrudan çalışır.
- Tüm `mi_`/`mo_`/`ei_`/`eo_`/`hi_`/`ho_`/`ci_`/`co_` fonksiyonları (300'den fazla) mevcut;
  betik geometriyi sıfırdan çizebilir, malzeme atayabilir, çözüp sonuç okuyabilir.
- `import numpy as np` ulab'a yönlendiriliyor (numpy'nin bir alt kümesi).
- Arayüzde zaten "File > Run Script" ve Python konsolu (`gui/py_console.cpp`) var.

### Sınırlar (yapay zekaya söylenmesi gerekenler)

- Tam CPython değil, MicroPython: bazı standart kütüphane modülleri yok.
- numpy sadece ulab kadar; scipy çok sınırlı.
- matplotlib yok (masaüstünde CPython köprüsüyle çalışıyor, Android'de çalışmayacak).

## 2. Android'e özgü engeller

1. **Çözücüler ayrı program olarak çalışıyor.** `gui/solver_progress_dialog.cpp` çözücüyü
   `QProcess` ile başlatıyor. Android 10'dan beri uygulamanın yazabildiği klasörlerden
   program çalıştırmak yasak.
   - Hızlı yol (prototip): çözücüleri `libfkn.so` gibi adlarla APK'nın native lib klasörüne
     koymak (`extractNativeLibs=true`) ve oradan çalıştırmak.
   - Kalıcı yol: her çözücünün `main()`'ini çağrılabilir bir fonksiyona çevirmek
     (`int fkn_solve(const char* path, progress_cb, cancel_flag)`), aynı süreçte arka plan
     iş parçacığında çalıştırmak. `exit()` çağrıları ve global değişkenler temizlenmeli.
     Bu değişiklik upstream için de değerli.
2. **CPython köprüsü** (`libfemm/pybridge_host.cpp`) `posix_spawn` ile sistem Python'u
   arıyor. Android'de kapatılacak.
3. **Dosya erişimi.** Android dosyaları `content://` adresleriyle veriyor. Betikler ve
   `.fem` dosyaları uygulamaya özel bir çalışma klasörüne kopyalanıp orada çalıştırılmalı
   (betiklerdeki `open("model.fem")` gibi göreli yollar böylece çalışır). Malzeme
   kütüphaneleri (`matlibs/*.dat`) ve örnekler APK'ya `assets` olarak konacak.
4. **ARM ve sayısal sonuçlar.** ARM64'te derleyici FMA kullanırsa sonuçlar x86'dan çok az
   farklı çıkabilir. Testler toleranslı karşılaştırılmalı ya da `-ffp-contract=off` kullanılmalı.
5. **Uzun çözümler.** Kullanıcı uygulamadan çıkınca Android işlemi durdurabilir. Uzun
   çözümler için ön plan servisi (bildirimde ilerleme) ve iptal düğmesi gerekir.

## 3. Hedef kullanım akışı

```
Yapay zeka (ChatGPT, Claude vb.)
   │  1. "FEMM betik ortamı" komut metnini yapıştır + modeli tarif et
   ▼
Python betiği
   │  2. Paylaş / kopyala-yapıştır / .py dosyası aç
   ▼
FEMM Android ── 3. Çalıştır → mesh → çöz → sonuç
   │  4. Hata olursa: "Hatayı kopyala" (satır no + mesaj + betik)
   ▼
Yapay zekaya geri yapıştır → düzeltilmiş betik → 2. adıma dön
```

Uygulamada bunu destekleyecek özellikler:

- **Betik ekranı:** sol tarafta düzenleyici (satır numaraları, Python renklendirme),
  sağ tarafta model/sonuç görünümü, altta çıktı konsolu. Tablet yatay ekranına göre.
- **Betiği alma yolları:** panodan yapıştır; `.py` dosyası aç; Android "Paylaş" menüsünden
  metin veya dosya alma (yapay zeka uygulamasından tek dokunuşla FEMM'e gönderme).
- **"Yapay zeka komut metnini kopyala" düğmesi:** betik ortamını tarif eden hazır metin
  (MicroPython olduğu, ulab sınırları, matplotlib olmadığı, dosya yolları, sonuçların
  `print` ile yazılması, örnek bir betik). Aynı metin depoda `docs/ai-prompt.md` olarak durur.
- **"Hatayı kopyala" düğmesi:** hata mesajı, satır numarası ve ilgili satırları yapay zekaya
  yapıştırmaya hazır biçimde panoya koyar.
- **Sonuç görüntüleme:** renk haritası, akı çizgileri, mesh; iki parmakla yakınlaştırma ve
  kaydırma. Görünüm için mevcut `results_widget` kullanılır.
- **Basit grafik:** matplotlib yerine `plot(x, y)` gibi küçük bir fonksiyon; mevcut
  `gui/plot_widget.cpp` ile çizilir (frekans taraması, kuvvet-konum eğrisi gibi sonuçlar için).
- **Çıktıları paylaşma:** betiğin yazdığı dosyalar (`.csv`, `.txt`, `.ans`) ve ekran görüntüsü
  Android "Paylaş" ile dışarı aktarılır.

## 4. Lisans

- Arayüz, `libfemm`, `tangle`, ortak kod: MIT.
- `solvers/`: GPL-3.0-or-later. APK bütün olarak GPL-3 koşullarına tabi olur; kaynak kodu
  zaten bu GitHub deposunda açık olacağı için sorun yok. Release notlarında kaynak koda ve
  lisanslara bağlantı verilmeli, uygulamada "Lisanslar" ekranı olmalı.
- Qt: LGPL-3. Qt kütüphaneleri APK'da paylaşımlı `.so` olarak durduğu sürece uygun.
- Çözücü (`solvers/`) değişikliklerini upstream'e göndermek için `CLA-solvers.md`
  imzalanmalı. Diğer klasörler için CLA gerekmiyor (bkz. `CONTRIBUTING.md`).

## 5. Depo ve upstream stratejisi

- Bu depo `dcm3c/femm-qt`'nin geçmişiyle birlikte başlatılır; `upstream` uzak deposu eklenir
  ve upstream düzenli olarak birleştirilir.
- Android'e özel kod ayrı tutulur: `android/` (Gradle, manifest, Java/Kotlin köprüsü),
  `gui/android/` (betik ekranı gibi tablet arayüzü parçaları), `#ifdef __ANDROID__` en az düzeyde.
- Upstream'e gönderilecek değişiklikler küçük ve ayrı PR'lar olarak hazırlanır:
  1. Çözücüleri kütüphane olarak çağırma API'si (masaüstüne de fayda: daha hızlı başlangıç,
     daha iyi iptal) — CLA gerekir.
  2. ARM64 / Android derleme düzeltmeleri.
  3. "Hatayı kopyala", paylaşımdan betik alma gibi platformdan bağımsız arayüz özellikleri.
  4. `android/` paketleme ve CI işi (bakımcı isterse).
- Büyük işe başlamadan önce upstream'de bir issue açılıp yaklaşım bakımcıyla konuşulur.

## 6. Aşamalar

### Aşama 0: Temel çizgi (1 hafta)
- femm-qt geçmişini bu depoya al, `upstream` uzak deposunu ekle.
- Linux'ta `linux_build.sh` ile derle, `bash testing/_verify_all.sh` çalıştır, sonuçları
  referans olarak sakla.
- GitHub Actions'ta Linux derleme + test işi kur.

### Aşama 1: Çekirdek Android'de (2-3 hafta)
- Android NDK ile Qt'siz derleme: `common`, `tangle`, `libfemm`, `solvers`, `micropython`.
  Hedefler: `arm64-v8a` (tablet), `x86_64` (emülatör).
- `femm_headless` ile örnek betikleri (`examples/*.py`) `adb shell` üzerinden çalıştır,
  sonuçları Aşama 0 ile toleranslı karşılaştır.
- **Çıktı:** ARM64'te doğru sonuç veren mesher, 4 çözücü ve betik motoru.

### Aşama 2: Çözücüleri kütüphaneye çevir (2 hafta)
- C API: giriş dosyası, ilerleme geri çağrısı, iptal bayrağı, hata kodu.
- Aynı süreçte arka arkaya (ör. frekans taraması döngüsünde) çözüm yapılabildiğini test et.
- Upstream'e issue + PR.

### Aşama 3: İlk APK – mevcut arayüz tablette (3 hafta)
- Qt 6 for Android ile `gui/`'yi derle; `QProcess` yolunu kütüphane çağrısıyla değiştir.
- CPython köprüsünü kapat; MicroPython konsolu ve "Run Script" çalışsın.
- Çalışma klasörü, `assets`'ten malzeme kütüphaneleri ve örnekler.
- **Kabul testi:** `examples/` içindeki bir pyFEMM betiğini tablette aç → çalıştır → sonucu gör.
- GitHub Actions: Android APK derleme işi; etiket (`v*`) atılınca imzalı APK'yı Release'e
  yükle. İmza anahtarı GitHub Secrets'ta.
- **Çıktı:** İlk indirilebilir APK (ön sürüm / pre-release).

### Aşama 4: Betik odaklı tablet arayüzü (3-4 hafta)
- Bölüm 3'teki betik ekranı: düzenleyici + görünüm + konsol yan yana.
- Paylaş menüsünden betik alma, panodan yapıştırma, `.py` açma.
- "Yapay zeka komut metnini kopyala" ve "Hatayı kopyala" düğmeleri; `docs/ai-prompt.md`.
- Sonuç görünümünde dokunmatik yakınlaştırma/kaydırma, büyük araç çubuğu düğmeleri.
- `plot(x, y)` fonksiyonu ve çıktı paylaşma.
- Uzun çözümler için ön plan servisi ve iptal.

### Aşama 5: Sağlamlaştırma ve 1.0 (2 hafta)
- Yapay zekaya üretilmiş 20-30 farklı betikle deneme (manyetik, elektrostatik, ısı, akım);
  sık çıkan hatalara göre `ai-prompt.md`'yi ve uyumluluk katmanını iyileştir.
- Farklı tabletlerde test (düşük RAM'li cihaz dâhil).
- Lisanslar ekranı, README'de kurulum anlatımı ("bilinmeyen kaynaklardan yükleme").
- v1.0 Release.

Toplam kaba tahmin: tek geliştiriciyle 3-4 ay.

## 7. Kapsam dışı (ilk sürüm)

- Uygulamaya gömülü yapay zeka
- Telefon için ayrı arayüz
- Dokunmatikle sıfırdan geometri çizimi (betikle yapılacak; mevcut çizim araçları olduğu gibi kalır)
- CPython/matplotlib köprüsü
- TCP üzerinden dış programlardan kontrol (MATLAB, Julia vb.)
- Google Play yayını
