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

// Struktur data untuk ESP-NOW
typedef struct struct_message {
  byte uid[4];
} struct_message;

struct_message incomingReadings;
struct_message outgoingData;

// MAC Address ESP8266 Gate Keluar (Ganti dengan MAC Address Gate Keluar Anda)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool waitingForCard = false;
unsigned long irTimer = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  pinMode(IR_PIN, INPUT);
  gateServo.attach(SERVO_PIN);
  gateServo.write(0); // Posisi tertutup

  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Error!");
  }
  myDFPlayer.volume(25); // Atur volume (0-30)

  // Inisialisasi ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_COMMS);
  esp_now_register_send_cb(OnDataSent);
  
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMMS, 1, NULL, 0);
  Serial.println("Gate Masuk Siap");
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  // Status pengiriman data ESP-NOW
}

void loop() {
  int irState = digitalRead(IR_PIN); // Asumsi LOW saat kendaraan terdeteksi

  // 1. Kendaraan terdeteksi oleh sensor IR di gate masuk
  if (irState == LOW && !waitingForCard) {
    myDFPlayer.play(1); // Putar 001.mp3 (Silahkan Tap Kartu)
    waitingForCard = true;
    delay(2000); // Debounce
  }

  // 2. Saat menunggu tap kartu NFC
  if (waitingForCard) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      // Ambil UID Kartu (4 byte)
      for (byte i = 0; i < 4; i++) {
        outgoingData.uid[i] = mfrc522.uid.uidByte[i];
      }

      // Kirim UID ke Gate Keluar via ESP-NOW
      esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));

      // Buka Gate
      myDFPlayer.play(2); // Putar 002.mp3 (Silahkan masuk)
      gateServo.write(90); // Buka
      delay(4000);         // Waktu kendaraan masuk

      // Tutup Gate setelah kendaraan lewat & tidak terdeteksi IR
      while (digitalRead(IR_PIN) == LOW) {
        delay(500); // Tunggu sampai kendaraan benar-benar lewat
      }
      gateServo.write(0); // Tutup
      waitingForCard = false;
      
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }
}
