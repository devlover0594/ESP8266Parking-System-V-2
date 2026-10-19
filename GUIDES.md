# Panduan Langkah demi Langkah Implementasi Sistem Parkir ESP8266

Dokumen ini berisi panduan teknis langkah demi langkah untuk merakit, mengonfigurasi, dan menguji sistem parkir pintar berbasis ESP8266 dual-gate.

---

## Langkah 1: Persiapan Alat dan Bahan

### Hardware
| Komponen | Gate Masuk | Gate Keluar | Total |
| :--- | :---: | :---: | :---: |
| ESP8266 NodeMCU / Wemos D1 Mini | 1 | 1 | 2 |
| Modul RFID MFRC522 | 1 | 1 | 2 |
| Sensor Obstacle IR (KY-032) | 1 | 1 | 2 |
| Servo Motor SG90 / MG996R | 1 | 1 | 2 |
| DFPlayer Mini + MicroSD | 1 | 1 | 2 |
| Speaker Kecil (8 Ohm / 0.5W–3W) | 1 | 1 | 2 |
| Resistor 1k$\Omega$ (untuk RX DFPlayer) | 1 | 1 | 2 |
| Breadboard & Kabel Jumper | Secukupnya | Secukupnya | - |
| Catu Daya / Power Supply (5V 2A-3A) | 1 | 1 | 2 |

---

## Langkah 2: Persiapan Kartu MicroSD dan Audio

1. Format kartu MicroSD dengan format **FAT32**.
2. Buat folder bernama `mp3` di dalam MicroSD (opsional, atau letakkan langsung di root directory).
3. Siapkan file audio dalam format `.mp3` dan beri nama sesuai penamaan ketat berikut:
   * `001.mp3` : *"Silahkan tap kartu"*
   * `002.mp3` : *"Silahkan masuk"*
   * `003.mp3` : *"Silahkan tap kartu"*
   * `004.mp3` : *"Terima kasih, selamat jalan"*
   * `005.mp3` : *"Kartu yang Anda tap salah"*
4. Masukkan file ke MicroSD:
   * **MicroSD Gate Masuk:** Membutuhkan `001.mp3` dan `002.mp3`.
   * **MicroSD Gate Keluar:** Membutuhkan `003.mp3`, `004.mp3`, dan `005.mp3`.

---

## Langkah 3: Skema Pengabelan (Wiring Pinout)

Konfigurasi pin ini berlaku identik untuk **Gate Masuk** maupun **Gate Keluar**:

### 1. Modul RFID-RC522 (SPI)
| Pin MFRC522 | Pin ESP8266 | Keterangan |
| :--- | :--- | :--- |
| **VCC** | 3.3V | *Jangan hubungkan ke 5V!* |
| **RST** | D1 (GPIO 5) | Reset Pin |
| **GND** | GND | Ground |
| **MISO** | D6 (GPIO 12) | SPI MISO |
| **MOSI** | D7 (GPIO 13) | SPI MOSI |
| **SCK** | D5 (GPIO 14) | SPI Clock |
| **SDA (SS)** | D2 (GPIO 4) | SPI Chip Select |

### 2. Servo SG90
| Pin Servo | Pin ESP8266 / Power |
| :--- | :--- |
| **VCC (Merah)** | 5V (VIN / External Power) |
| **GND (Cokelat/Hitam)** | GND |
| **Signal (Kuning/Oranye)** | D8 (GPIO 15) |

### 3. Sensor Obstacle IR (KY-032)
| Pin KY-032 | Pin ESP8266 |
| :--- | :--- |
| **VCC** | 3.3V / 5V |
| **GND** | GND |
| **OUT** | D0 (GPIO 16) |

### 4. DFPlayer Mini
| Pin DFPlayer | Pin ESP8266 / Power |
| :--- | :--- |
| **VCC** | 5V |
| **GND** | GND |
| **RX** | Resistor 1k$\Omega$ $\rightarrow$ D4 (GPIO 2) |
| **TX** | D3 (GPIO 0) |
| **SPK_1 / SPK_2** | Terhubung ke Speaker |

---

## Langkah 4: Mendapatkan MAC Address ESP8266

ESP-NOW membutuhkan alamat MAC hardware dari ESP8266 penerima.

1. Hubungkan ESP8266 yang akan dijadikan **Gate Keluar** ke komputer.
2. Buka Arduino IDE, unggah skrip berikut:

```cpp
#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("MAC Address Gate Keluar: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
```

3. Buka **Serial Monitor** (baud rate `115200`).
4. Catat Alamat MAC yang muncul (contoh: `24:0A:C4:9A:12:34`).
5. Ubah format tersebut ke dalam array `uint8_t` pada program Gate Masuk:
   `uint8_t broadcastAddress[] = {0x24, 0x0A, 0xC4, 0x9A, 0x12, 0x34};`

---

## Langkah 5: Pengunggahan Program Utama

1. Buka Arduino IDE dan pastikan library berikut telah terinstall:
   * `MFRC522` oleh GitHubCommunity
   * `DFRobotDFPlayerMini` oleh DFRobot
2. Upload kode **Gate Masuk** ke ESP8266 Pertama (pastikan MAC Address Gate Keluar sudah disesuaikan).
3. Upload kode **Gate Keluar** ke ESP8266 Kedua.

---

## Langkah 6: Skenario Pengujian Sistem

### Pengujian Gate Masuk
1. Dekatkan halangan ke Sensor IR Gate Masuk.
   * **Hasil:** Audio `001.mp3` ("Silahkan tap kartu") berbunyi.
2. Tap kartu NFC ke Reader MFRC522.
   * **Hasil:**
     * Audio `002.mp3` ("Silahkan masuk") berbunyi.
     * Servo berputar ke $90^\circ$ (Gate Terbuka).
     * Data UID dikirim via ESP-NOW.
3. Jauhkan halangan dari Sensor IR (kendaraan selesai melintas).
   * **Hasil:** Servo kembali ke $0^\circ$ (Gate Tertutup).

### Pengujian Gate Keluar
1. Dekatkan halangan ke Sensor IR Gate Keluar.
   * **Hasil:** Audio `003.mp3` ("Silahkan tap kartu") berbunyi.
2. Tap kartu NFC **yang sama** dengan yang digunakannya saat masuk.
   * **Hasil:**
     * Audio `004.mp3` ("Terima kasih, selamat jalan") berbunyi.
     * Servo berputar ke $90^\circ$ (Gate Terbuka).
3. Jauhkan halangan dari Sensor IR.
   * **Hasil:** Servo kembali ke $0^\circ$ (Gate Tertutup) dan memori UID direset.

### Pengujian Kartu Salah
1. Ulangi alur masuk.
2. Di Gate Keluar, tap kartu NFC **berbeda** yang belum terdaftar.
   * **Hasil:**
     * Audio `005.mp3` ("Kartu yang anda tap salah") berbunyi.
     * Palang pintu tetap tertutup ($0^\circ$).