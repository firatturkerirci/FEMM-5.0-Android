# FEMM 5.0 for Android (tablet)

This fork of [dcm3c/femm-qt](https://github.com/dcm3c/femm-qt) builds FEMM 5.0
as an Android app for tablets. Technical notes: [docs/android.md](docs/android.md).

## Kurulum (Türkçe)

1. Tabletin tarayıcısında şu bağlantıyı aç; en yeni sürüm doğrudan iner:
   https://github.com/firatturkerirci/FEMM-5.0-Android/releases/latest/download/femm-android-arm64-v8a.apk
   (Tüm sürümler: deponun **Releases** sayfası.)
2. Dosyayı aç. Android "bilinmeyen kaynaklardan yükleme" izni isterse, dosyayı
   açtığın uygulamaya (tarayıcı veya Dosyalar) bu izni ver.
3. FEMM'i aç. Sol tarafta **Script** paneli, sağda model görünümü vardır.

## Kullanım: yapay zeka ile betik yazdırıp çözdürme

1. **AI Prompt** düğmesine bas. FEMM'in betik ortamını anlatan metin panoya
   kopyalanır.
2. Yapay zeka uygulamasını (ChatGPT, Claude vb.) aç, metni yapıştır ve
   sonuna modelini tarif et. Örnek: *"Eksenel simetrik, 500 sarımlı bir
   bobin ve çelik dalıcı; dalıcıya etkiyen kuvveti 0-20 mm arası konumlar için
   hesapla."*
3. Yapay zekanın verdiği kodu kopyala, FEMM'de **Paste**'e bas. Kod bloğu
   işaretleri (```python) otomatik temizlenir.
4. **Run**'a bas. Model çizilir, çözülür; sonuç sağda, yazdırılan değerler
   panelin altında görünür. Görünümü iki parmakla yakınlaştırıp kaydırabilirsin.
5. Hata olursa hatalı satır kırmızıyla işaretlenir. **Copy Error**'a bas,
   yapay zekaya yapıştır, düzeltilmiş betiği alıp 3. adıma dön.

Uygulamayı arka plana aldığında açık modeller kaydedilir. Android uygulamayı
bu arada kapatırsa, yeniden açtığında modeller, sonuçlar ve betik geri gelir.

Betikler ve oluşturdukları dosyalar (`.fem`, `.ans`, `.csv`) uygulamanın çalışma
klasöründe tutulur. **Save** ile betiği istediğin yere kaydedebilir, **Open** ile
tabletteki herhangi bir `.py` dosyasını açabilirsin.

## Sınırlar

* Betikler FEMM'in içindeki MicroPython'da çalışır: matplotlib, scipy ve
  pandas yoktur; numpy'nin bir alt kümesi vardır. AI Prompt metni yapay zekaya
  bunu söyler.
* MATLAB, Octave veya harici Python'dan TCP ile kontrol Android'de desteklenmez.

## License

Same as femm-qt: the app is MIT, the solvers are GPL-3.0-or-later, Qt is
LGPL-3.0. See [README.md](README.md) and [LICENSE](LICENSE).
