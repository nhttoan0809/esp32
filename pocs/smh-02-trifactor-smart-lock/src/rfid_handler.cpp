#include "rfid_handler.h"
#include "config.h"

RfidHandler rfidHandler;

RfidHandler::RfidHandler()
    : mfrc522(PIN_RFID_SS, PIN_RFID_RST),
      cardDetected(false),
      lastUid(""),
      lastReadTime(0),
      readerOnline(false) {}

void RfidHandler::begin() {
    SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
    mfrc522.PCD_Init();
    delay(4); // Brief init stabilization

    byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    if (v == 0x00 || v == 0xFF) {
        readerOnline = false;
        Serial.println(F("[RFID] Warning: MFRC522 communication failed or not connected!"));
    } else {
        readerOnline = true;
        Serial.print(F("[RFID] MFRC522 detected successfully, Firmware Version: 0x"));
        Serial.println(v, HEX);
    }
}

bool RfidHandler::isReaderConnected() const {
    return readerOnline;
}

bool RfidHandler::hasCard() {
    return cardDetected;
}

String RfidHandler::getCardUid() {
    if (!cardDetected) return "";
    String uid = lastUid;
    cardDetected = false;
    lastUid = "";
    return uid;
}

void RfidHandler::update() {
    if (!readerOnline) return;

    // Throttle reads to prevent repeated triggers on the same tap (500ms debounce)
    if (millis() - lastReadTime < 500) {
        return;
    }

    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }

    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    // Convert UID to clean uppercase HEX string
    String uidStr = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
            uidStr += "0";
        }
        uidStr += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidStr.toUpperCase();

    lastUid = uidStr;
    cardDetected = true;
    lastReadTime = millis();

    // Halt card and stop encryption
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
