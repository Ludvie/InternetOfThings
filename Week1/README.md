# Week 1 — ESP32 Environmental Telemetry Node (Lab-0)

Proyek ini mengikuti **Praktikum Minggu 1: Toolchain, Wokwi Simulator, & Git**.

## Fitur

- ESP32 DOIT DevKit V1 + DHT22 pada GPIO 4.
- LED merah indikator suhu pada GPIO 2 dengan resistor 220 ohm.
- Pushbutton hijau pada GPIO 15 dengan `INPUT_PULLUP`.
- Payload JSON pada Serial Monitor 115200 baud.
- Eksperimen blocking (`delay(2000)`) vs non-blocking (`millis()`).

## Struktur

```text
Week1/
├── .vscode/
├── build.ps1
├── diagram.json
├── docs/
│   └── STATUS-FIRMWARE.md
├── platformio.ini
├── src/
│   └── main.cpp
└── wokwi.toml
```

`.gitignore` berada di root repository dan mengabaikan hasil build PlatformIO.

## Build firmware

1. Buka folder `Week1/` langsung di VS Code (`File > Open Folder`).
2. Pastikan ekstensi PlatformIO IDE terpasang dan inisialisasinya selesai.
3. Buka terminal PlatformIO, lalu jalankan:
   ```powershell
   pio run
   ```
   Atau jalankan skrip:
   ```powershell
   .\build.ps1
   ```
4. Build yang berhasil menghasilkan:
   - `.pio/build/esp32doit-devkit-v1/firmware.bin`
   - `.pio/build/esp32doit-devkit-v1/firmware.elf`
5. Setelah kedua file tersedia, jalankan Wokwi Simulator.

**Penting:** `firmware.bin` harus dibuat oleh PlatformIO pada komputer yang memiliki toolchain ESP32. Jangan membuat file `.bin` kosong atau mengganti ekstensi file lain menjadi `.bin`; Wokwi tidak akan bisa menjalankannya.

## Eksperimen blocking vs non-blocking

1. Buka `src/main.cpp`.
2. Set `#define MODE_NON_BLOCKING 0`, build ulang, jalankan Wokwi, lalu tekan tombol hijau berulang kali. Amati respons yang terlambat/terlewat ketika `delay(2000)` berlangsung.
3. Ubah menjadi `#define MODE_NON_BLOCKING 1`, build ulang, jalankan kembali, dan bandingkan respons tombol.
4. Ubah suhu DHT22 ke 35 °C. LED merah seharusnya menyala karena suhu >= 32 °C.

## Pinout

| Komponen | ESP32 |
|---|---|
| DHT22 VCC | 3V3 |
| DHT22 GND | GND |
| DHT22 SDA/DATA | GPIO 4 |
| LED merah melalui resistor 220 ohm | GPIO 2 |
| Pushbutton | GPIO 15 ke GND |
| Serial Monitor | 115200 baud |

Catatan hardware: GPIO15 adalah strapping pin pada ESP32 klasik. Rangkaian mengikuti modul dan cocok untuk simulasi Wokwi; saat merakit perangkat fisik, pastikan tombol tidak menahan pin pada level yang mengganggu proses boot.
