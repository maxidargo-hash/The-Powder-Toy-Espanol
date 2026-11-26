# The Powder Toy Español para Android y tablets

Esta edición parte de The Powder Toy Android 1.24, compilación 132. Conserva los controles táctiles del port SDL 1.2: botones laterales, cambio de tamaño con las teclas de volumen, zoom, sellos y desplazamiento de la paleta. El juego se compila con las opciones originales `TOUCHUI`, `NOMOD`, `SDL1_2`, `LUACONSOLE` y `GRAVFFT`.

- Motor original [jacob1/The-Powder-Toy@0635c0e](https://github.com/jacob1/The-Powder-Toy/tree/0635c0eeb46ce249551f51919d4d8712d54827e9)
- Adaptación SDL y código del lanzador [jacob1/android-sdl@cc6c726](https://github.com/jacob1/android-sdl/tree/cc6c7263502dfe0d22bcb9c639fbfae8f3ff9355)
- Fuente de los glifos españoles [The-Powder-Toy/The-Powder-Toy](https://github.com/The-Powder-Toy/The-Powder-Toy)

El empaquetado utiliza las actividades Android, los recursos y las bibliotecas SDL del APK original suministrado como referencia. Sustituye `libapplication.so` en cada arquitectura y cambia las cadenas y la identidad de la aplicación. La versión en español utiliza `io.github.maxidargo_hash.powdertoy.es`, por lo que se puede instalar junto a la edición oficial. Este APK declara Android 5.0 o posterior y contiene `arm64-v8a`, `armeabi-v7a`, `x86` y `x86_64`.

## Compilar y firmar

Necesitas Linux x86-64, Python 3, Ninja, Java, Android NDK r28 o posterior, Android Build Tools 35.0.0, Apktool 2.12.1 y una clave para firmar el APK.

Usa el APK original de Android 1.24, con `versionCode=132` y SHA-256 `431789879324569dab4d2480c458d9af4bcdfb13c3846e8d526b1e1739e2ec47`. El script de empaquetado comprueba ese SHA-256. Prepara las cabeceras del port SDL:

```sh
git init android-sdl
git -C android-sdl fetch --depth 1 https://github.com/jacob1/android-sdl.git cc6c7263502dfe0d22bcb9c639fbfae8f3ff9355
git -C android-sdl checkout --detach FETCH_HEAD
```

Desde la raíz de este proyecto, ejecuta los siguientes comandos con tus rutas locales. Define `ANDROID_KEYSTORE_PASS` en el entorno de la compilación.

```sh
python3 android/build-native.py "The Powder Toy.apk" \
  --ndk /ruta/android-ndk-r28c --sdl-port /ruta/android-sdl \
  --build-dir build-android-tablet

python3 android/package-apk.py "The Powder Toy.apk" \
  --native-dir build-android-tablet --apktool /ruta/apktool.jar \
  --build-tools /ruta/android-sdk/build-tools/35.0.0 \
  --keystore /ruta/firma.p12 --key-alias androidkey \
  --output The-Powder-Toy-Espanol-1.24-tablet.apk
```

Licencia del código del juego en [LICENSE](../LICENSE).
