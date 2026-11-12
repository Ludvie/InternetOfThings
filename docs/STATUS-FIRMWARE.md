# Status Firmware

Firmware `.bin` dan `.elf` belum disertakan karena keduanya harus dihasilkan dari kompilasi PlatformIO dengan toolchain ESP32. Tidak ada binary palsu atau placeholder di paket ini.

Perintah untuk membuat firmware:
```powershell
pio run
```

Output yang diharapkan:
```text
.pio/build/esp32doit-devkit-v1/firmware.bin
.pio/build/esp32doit-devkit-v1/firmware.elf
```
