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

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  pinMode(IR_PIN, INPUT);
  gateServo.attach(SERVO_PIN);
  gateServo.write(0);

  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Error!");
  }
  myDFPlayer.volume(25);

  // Inisialisasi ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMMS);
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Gate Keluar Siap");
}

// Callback saat menerima UID dari Gate Masuk
void OnDataRecv(uint8_t *mac, uint8_t *incomingDataBytes, uint8_t len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));
  for (int i = 0; i < 4; i++) {
    storedUID[i] = incomingData.uid[i];
  }
  uidReceived = true;
  Serial.println("UID Berhasil Disimpan dari Gate Masuk!");
}

void loop() {
  int irState = digitalRead(IR_PIN);

  // 1. Kendaraan terdeteksi di gate keluar dan sudah ada data UID dari gate masuk
  if (irState == LOW && uidReceived && !waitingForExitTap) {
    myDFPlayer.play(3); // Putar 003.mp3 (Silahkan tap kartu)
    waitingForExitTap = true;
    delay(2000);
  }

  // 2. Membaca kartu saat di gate keluar
  if (waitingForExitTap) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      bool isMatch = true;
      
      // Validasi UID
      for (byte i = 0; i < 4; i++) {
        if (mfrc522.uid.uidByte[i] != storedUID[i]) {
          isMatch = false;
          break;
        }
      }

      if (isMatch) {
        // Jika Valid (Benar)
        myDFPlayer.play(4); // Putar 004.mp3 (Terimakasih, selamat jalan)
        gateServo.write(90); // Buka Gate
        delay(4000);

        // Tunggu sampai kendaraan melewati sensor IR
        while (digitalRead(IR_PIN) == LOW) {
          delay(500);
        }
        gateServo.write(0); // Tutup Gate
        
        // Reset status
        uidReceived = false;
        waitingForExitTap = false;
      } else {
        // Jika Gagal / Salah Kartu
        myDFPlayer.play(5); // Putar 005.mp3 (Kartu yang anda tap salah)
        gateServo.write(0); // Pastikan tertutup
        delay(3000);
        waitingForExitTap = false; // Meminta tap ulang
      }

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }
}
