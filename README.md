# 🚗 Smart Parking System Dual-Gate ESP8266 (ESP-NOW & NFC)

Sistem Manajemen Parkir Otomatis berbasis Internet of Things (IoT) yang mengintegrasikan dua mikrokontroler ESP8266 untuk mengelola pintu masuk (*Entry Gate*) dan pintu keluar (*Exit Gate*). Sistem ini menggunakan komunikasi nirkabel peer-to-peer **ESP-NOW** untuk mentransfer data identifikasi UID NFC secara instan tanpa membutuhkan jaringan router WiFi/Internet.

---

## 📋 Fitur Utama

- **Komunikasi Nirkabel Latensi Rendah:** Menggunakan protokol ESP-NOW untuk pertukaran data UID antar gate.
- **Sistem Panduan Audio (Voice Guidance):** Terintegrasi dengan DFPlayer Mini untuk memberikan instruksi suara interaktif pada tiap tahap alur parkir.
- **Otentikasi Keamanan NFC:** Memvalidasi kendaraan keluar berdasarkan kesesuaian UID kartu saat masuk.
- **Pintu Otomatis (Auto Barrier Gate):** Pengendalian palang menggunakan Servo SG90 yang terhubung secara independen di setiap gate.
- **Deteksi Kendaraan (Vehicle Presence Detection):** Menggunakan sensor inframerah KY-032 untuk memastikan palang tidak menutup sebelum kendaraan melewati area gate.

---

## 🛠️ Arsitektur Sistem & Cara Kerja

```
[ Gate Masuk (ESP8266 #1) ] --(ESP-NOW Protocol)--> [ Gate Keluar (ESP8266 #2) ]
        |                                                   |
 +------+\dots                                       +------+
 | IR Sensor (KY-032)                                | IR Sensor (KY-032)
 | NFC Reader (MFRC522)                              | NFC Reader (MFRC522)
 | Servo Gate (SG90)                                 | Servo Gate (SG90)
 | Audio Player (DFPlayer)                           | Audio Player (DFPlayer)
```

### 1. Alur Kerja Gate Masuk (*Entry Gate*)
1. **Deteksi Objek:** Kendaraan mendekati palang dan terdeteksi oleh Sensor IR KY-032.
2. **Instruksi Voice:** DFPlayer memutar `001.mp3` (*"Silahkan Tap Kartu"*).
3. **Pembacaan NFC:** Pengendara menempelkan kartu NFC pada MFRC522.
4. **Pengiriman Data:** ESP8266 menyimpan UID kartu dan mengirimkannya ke ESP8266 Gate Keluar melalui protokol ESP-NOW.
5. **Akses Diberikan:** DFPlayer memutar `002.mp3` (*"Silahkan Masuk"*) dan Servo membuka palang $90^\circ$.
6. **Penutupan Otomatis:** Setelah kendaraan masuk dan tidak lagi terdeteksi oleh Sensor IR, Servo menutup palang secara otomatis.

### 2. Alur Kerja Gate Keluar (*Exit Gate*)
1. **Deteksi Objek:** Kendaraan mendekati pintu keluar dan terdeteksi oleh Sensor IR KY-032.
2. **Instruksi Voice:** DFPlayer memutar `003.mp3` (*"Silahkan Tap Kartu"*).
3. **Validasi UID:** Pengendara menempelkan kartu NFC. Sistem membandingkan UID yang dibaca dengan UID yang diterima dari Gate Masuk.
4. **Kondisi Valid (Sukses):**
   - DFPlayer memutar `004.mp3` (*"Terima kasih, selamat jalan"*).
   - Servo membuka palang $90^\circ$.
   - Setelah kendaraan lewat (Sensor IR mendeteksi area bersih), Servo menutup palang dan data memori UID di-reset.
5. **Kondisi Invalid (Gagal):**
   - DFPlayer memutar `005.mp3` (*"Kartu yang Anda tap salah"*).
   - Palang pintu tetap tertutup.

---

## ⚡ Kode Program Utama

### 1. Source Code: Gate Masuk (`Gate_Masuk.ino`)

```cpp
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define SS_PIN D2
#define RST_PIN D1
#define IR_PIN D0
#define SERVO_PIN D8

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo gateServo;

SoftwareSerial mySoftwareSerial(D3, D4); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

typedef struct struct_message {
  byte uid[4];
} struct_message;

struct_message outgoingData;

// Ganti MAC Address di bawah sesuai dengan MAC Address ESP8266 Gate Keluar
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool waitingForCard = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  pinMode(IR_PIN, INPUT);
  gateServo.attach(SERVO_PIN);
  gateServo.write(0); // Posisi awal tertutup

  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer Error!"));
  }
  myDFPlayer.volume(25);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println(F("Gagal Inisialisasi ESP-NOW"));
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_COMMS);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMMS, 1, NULL, 0);
}

void loop() {
  int irState = digitalRead(IR_PIN);

  // Jika kendaraan terdeteksi oleh IR
  if (irState == LOW && !waitingForCard) {
    myDFPlayer.play(1); // 001.mp3: Silahkan Tap Kartu
    waitingForCard = true;
    delay(2000);
  }

  // Pembacaan Kartu NFC
  if (waitingForCard) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      for (byte i = 0; i < 4; i++) {
        outgoingData.uid[i] = mfrc522.uid.uidByte[i];
      }

      // Kirim UID ke Gate Keluar
      esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));

      myDFPlayer.play(2); // 002.mp3: Silahkan Masuk
      gateServo.write(90); // Buka Gate
      delay(3000);

      // Tunggu hingga kendaraan melewati sensor IR
      while (digitalRead(IR_PIN) == LOW) {
        delay(200);
      }
      
      gateServo.write(0); // Tutup Gate
      waitingForCard = false;
      
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }
}
```

---

### 2. Source Code: Gate Keluar (`Gate_Keluar.ino`)

```cpp
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define SS_PIN D2
#define RST_PIN D1
#define IR_PIN D0
#define SERVO_PIN D8

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo gateServo;

SoftwareSerial mySoftwareSerial(D3, D4);
DFRobotDFPlayerMini myDFPlayer;

typedef struct struct_message {
  byte uid[4];
} struct_message;

struct_message incomingData;
byte storedUID[4];
bool uidReceived = false;
bool waitingForExitTap = false;

void OnDataRecv(uint8_t *mac, uint8_t *incomingDataBytes, uint8_t len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));
  for (int i = 0; i < 4; i++) {
    storedUID[i] = incomingData.uid[i];
  }
  uidReceived = true;
  Serial.println(F("UID Diterima dari Gate Masuk."));
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  pinMode(IR_PIN, INPUT);
  gateServo.attach(SERVO_PIN);
  gateServo.write(0);

  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer Error!"));
  }
  myDFPlayer.volume(25);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println(F("Gagal Inisialisasi ESP-NOW"));
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMMS);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  int irState = digitalRead(IR_PIN);

  if (irState == LOW && uidReceived && !waitingForExitTap) {
    myDFPlayer.play(3); // 003.mp3: Silahkan Tap Kartu
    waitingForExitTap = true;
    delay(2000);
  }

  if (waitingForExitTap) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      bool isMatch = true;
      
      for (byte i = 0; i < 4; i++) {
        if (mfrc522.uid.uidByte[i] != storedUID[i]) {
          isMatch = false;
          break;
        }
      }

      if (isMatch) {
        myDFPlayer.play(4); // 004.mp3: Terima Kasih, Selamat Jalan
        gateServo.write(90); // Buka Gate
        delay(3000);

        while (digitalRead(IR_PIN) == LOW) {
          delay(200);
        }
        
        gateServo.write(0); // Tutup Gate
        uidReceived = false;
        waitingForExitTap = false;
      } else {
        myDFPlayer.play(5); // 005.mp3: Kartu Yang Anda Tap Salah
        gateServo.write(0);
        delay(2000);
        waitingForExitTap = false;
      }

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }
}
```

---

## 📌 Troubleshooting & Catatan Penting

1. **Catu Daya Eksternal:** Servo SG90 dan DFPlayer Mini dapat menyebabkan penurunan tegangan (*voltage dip*) jika ditenagai langsung dari pin 3.3V ESP8266. Disarankan menggunakan catu daya eksternal 5V (2A) yang dihubungkan ke VIN dan GND secara paralel.
2. **Resistor DFPlayer:** Pasang resistor $1\text{k}\Omega$ pada pin RX DFPlayer Mini untuk meredam *noise* dan mencegah kerusakan akibat perbedaan logika 3.3V ke 5V.
3. **Penyetelan Sensitivitas KY-032:** Putar potensiometer pada modul KY-032 untuk menyesuaikan jarak jangkauan deteksi objek kendaraan.