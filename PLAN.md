# FEMM 5.0 Android Port Planı

Kaynak: [dcm3c/femm-qt](https://github.com/dcm3c/femm-qt) (FEMM 5.0, sürüm 5.0.20261021).
Bu plan, Ekim 2026'daki `main` dalının incelenmesine dayanır. Kod henüz Android için
derlenmedi; aşağıdaki "engel" listesi kaynak kod okunarak çıkarıldı.

## 1. Mevcut kodun durumu

femm-qt, FEMM 4.2'nin MFC'den Qt 6'ya taşınmış hâli. Katmanlı ve taşınabilir bir yapısı var,
bu da Android portunu kolaylaştırıyor:

| Klasör | İçerik | Satır (yaklaşık) | Qt bağımlılığı | Android için durum |
|---|---|---|---|---|
| `solvers/` | `fkn` (manyetik), `belasolv` (elektrostatik), `hsolv` (ısı), `csolv` (akım) | 14 bin | Yok | Saf C++17, NDK ile derlenmeli |
| `tangle/` | Mesh üretici (Triangle yerine) | 6 bin | Yok | x86'ya özel kısım `#if` ile korunuyor |
| `libfemm/` | Belge modeli, son işlem, betik motoru | 28 bin | Yok | Çekirdek; alt süreç başlatma kodu var |
| `common/` | Ortak matematik | 4 bin | Yok | Sorunsuz olmalı |
| `ipc/` | TCP sunucusu, `femm_headless` | 1 bin | Yok | İlk aşamada gerekmez |
| `gui/` | Qt 6 Widgets arayüzü | 20 bin | Widgets | Çalışır ama dokunmatik için uygun değil |
| `micropython/` + `third_party/bdwgc` | Gömülü MicroPython + Boehm GC | 96 bin (çoğu üçüncü taraf) | Yok | Süreç içinde çalışıyor, taşınabilir |
| `pybridge/`, `pylib/` | Ayrı süreçte CPython (matplotlib için) | 4 bin | Yok | Android'de doğrudan çalışmaz |

Derleme sistemi CMake 3.20+, C++17. Qt yoksa proje "headless" derleniyor; bu, çekirdeği
Android'e arayüzden bağımsız taşımamızı sağlıyor.

## 2. Android'e özgü engeller

1. **Çözücüler ayrı program olarak çalışıyor.** `gui/solver_progress_dialog.cpp` çözücüyü
   `QProcess` ile başlatıyor. Android 10'dan beri uygulamanın yazabildiği klasörlerden
   program çalıştırmak yasak. İki çözüm:
   - Hızlı yol (prototip): çözücüleri `libfkn.so` gibi adlarla APK'nın native lib klasörüne
     koymak (`extractNativeLibs=true`) ve oradan çalıştırmak.
   - Kalıcı yol: her çözücünün `main()`'ini çağrılabilir bir fonksiyona çevirmek
     (`int fkn_solve(const char* path, progress_cb, cancel_flag)`), aynı süreçte arka plan
     iş parçacığında çalıştırmak. Çözücülerdeki `exit()` çağrıları ve global değişkenler
     temizlenmeli.
2. **CPython köprüsü** (`libfemm/pybridge_host.cpp`) `posix_spawn` ile sistemdeki Python'u
   arıyor. Android'de sistem Python'u yok. İlk sürümde kapatılmalı; gömülü MicroPython yeterli.
   matplotlib gerekirse ileride Chaquopy değerlendirilebilir.
3. **Dosya erişimi.** Android dosyaları `content://` adresleriyle veriyor (Storage Access
   Framework). `.fem`/`.ans` açma-kaydetme bu yapıya uyarlanmalı; malzeme kütüphaneleri
   (`matlibs/*.dat`) ve örnekler APK içine `assets` olarak konmalı.
4. **Arayüz.** Mevcut arayüz fare, sağ tık, menü ve diyaloglara dayanıyor. Telefonda
   kullanışsız, tablette idare eder.
5. **ARM ve sayısal sonuçlar.** README sonuçların "bit-bit tekrarlanabilir" olduğunu söylüyor.
   ARM64'te derleyici FMA kullanırsa sonuçlar x86'dan çok az farklı çıkabilir. Testler
   toleranslı karşılaştırılmalı ya da `-ffp-contract=off` ile derlenmeli.
6. **Bellek ve süre.** Büyük modeller telefonda yavaş ve bellek açısından sınırda olabilir.
   Uzun çözümler için ön plan servisi (foreground service) ve iptal desteği gerekir.

## 3. Lisans

- Arayüz, `libfemm`, `tangle`, ortak kod: MIT.
- `solvers/`: GPL-3.0-or-later. Çözücüleri içeren APK bütün olarak GPL-3 koşullarına tabi
  olur: kaynak kodu açık olmalı. Google Play'de yayınlamaya engel değil.
- Qt: LGPL-3. Qt kütüphaneleri APK'da paylaşımlı (`.so`) olarak durduğu sürece uygun.
- Çözücü değişikliklerini upstream'e göndermek istersek `CLA-solvers.md` imzalanmalı.

## 4. Depo stratejisi

- Bu depo `dcm3c/femm-qt`'nin bir kopyası (fork) olarak başlatılmalı, `upstream` uzak deposu
  eklenmeli: upstream'deki düzeltmeler düzenli olarak birleştirilir.
- Android'e özel kod mümkün olduğunca ayrı tutulmalı: `android/` (Gradle, manifest, Java/Kotlin
  köprüsü), `cmake/android.cmake`, `#ifdef __ANDROID__` blokları en az düzeyde. Böylece
  upstream ile birleştirme çatışmaları küçük kalır.
- Genel işe yarayan değişiklikler (çözücüyü kütüphane olarak çağırma, ARM düzeltmeleri)
  upstream'e PR olarak önerilmeli.

## 5. Aşamalar

### Aşama 0: Temel çizgi (1 hafta)
- Fork'u oluştur, Linux'ta `linux_build.sh` ile masaüstü sürümünü derle.
- `bash testing/_verify_all.sh` çalıştır, sonuçları referans olarak sakla.
- GitHub Actions'ta Linux derleme + test işi kur.

### Aşama 1: Çekirdek Android'de (2-3 hafta)
- Android NDK (r27+) ile Qt'siz "headless" derleme: `common`, `tangle`, `libfemm`,
  `solvers`, `micropython`. Hedefler: `arm64-v8a` (gerçek cihaz), `x86_64` (emülatör).
- Windows'a özel `#include <windows.h>` yollarının ve x86 MXCSR kodunun Android'de doğru
  atlandığını doğrula.
- Çözücüleri `adb shell` ile emülatörde/cihazda çalıştır, Aşama 0 sonuçlarıyla toleranslı
  karşılaştır.
- **Çıktı:** ARM64'te doğru sonuç veren mesher + 4 çözücü.

### Aşama 2: Çözücüleri kütüphaneye çevir (2 hafta)
- Her çözücü için C API: giriş dosyası, ilerleme geri çağrısı, iptal bayrağı, hata kodu.
- `exit()` ve global durumları temizle; aynı süreçte arka arkaya çözüm yapılabildiğini test et.
- Masaüstünde de bu yolu kullanabilecek şekilde tasarla (upstream'e önerilebilir).

### Aşama 3: Arayüz MVP – Qt Widgets Android'de (3-4 hafta)
- Qt 6 for Android ile mevcut `gui/`'yi derle; `QProcess` yolunu Aşama 2'deki kütüphane
  çağrısıyla değiştir.
- CPython köprüsünü Android'de kapat; MicroPython konsolu çalışsın.
- Dosya aç/kaydet için SAF, `matlibs` ve `examples` için `assets`.
- Hedef cihaz: tablet. Örnek bir `.fem` dosyasını aç → mesh → çöz → sonuç gör akışı uçtan uca
  çalışsın.
- **Çıktı:** İlk kurulabilir APK (dahili test).

### Aşama 4: Dokunmatik arayüz (4-8 hafta, en büyük iş)
Karar noktası: mevcut Widgets arayüzünü uyarlamak mı, yeni bir arayüz mü?
- **Seçenek A – Widgets'ı uyarla:** iki parmakla yakınlaştırma/kaydırma, uzun basma = sağ tık,
  büyük araç çubukları. En hızlısı, ama telefonda hâlâ sıkışık.
- **Seçenek B – QML (Qt Quick) arayüzü** `libfemm` üzerine: mobil için en iyi deneyim, C++
  çekirdek aynen kalır, masaüstüyle kod paylaşımı yüksek. **Önerilen.**
- **Seçenek C – Kotlin/Jetpack Compose + JNI:** en "yerli" Android hissi ama Qt'den kopar,
  çizim ve son işlem görünümleri baştan yazılır.

Öneri: Aşama 3'te Widgets ile çalışan bir sürüm çıkar, Aşama 4'te QML'e geç. İlk QML
sürümü "görüntüleyici + çözücü" odaklı olabilir: hazır modeli aç, malzeme/sınır koşulu değiştir,
çöz, alan çizgileri/renk haritası ve integral sonuçlarını gör. Sıfırdan geometri çizme sonraya.

### Aşama 5: Sağlamlaştırma ve yayın (2-3 hafta)
- Uzun çözümler için ön plan servisi, iptal, bellek uyarıları.
- Cihaz testleri (düşük RAM'li telefon, tablet, Chromebook).
- Uygulama imzalama, AAB üretimi, GPL için kaynak kodu bağlantısı, lisans ekranı.
- CI'da Android APK derleme işi.

Toplam kaba tahmin: tek geliştiriciyle 4-6 ay.

## 6. Kapsam dışı (ilk sürüm)

- CPython/matplotlib köprüsü
- TCP üzerinden dış programlardan kontrol (MATLAB, Julia vb.)
- Lua 4 istemcisi
- DXF içe aktarma (sonradan eklenebilir)

## 7. Açık sorular

1. Hedef cihaz önce tablet mi, telefon mu?
2. İlk sürüm "görüntüle-değiştir-çöz" mü, yoksa tam model çizimi de gerekli mi?
3. Google Play'de yayın düşünülüyor mu, yoksa APK dağıtımı yeterli mi?
4. Değişiklikleri upstream'e (dcm3c/femm-qt) geri göndermek hedefleniyor mu?
