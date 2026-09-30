#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class RfidHandler {
public:
    RfidHandler();
    void begin();
    void update();

    bool hasCard();
    String getCardUid();
    bool isReaderConnected() const;

private:
    MFRC522 mfrc522;
    bool cardDetected;
    String lastUid;
    unsigned long lastReadTime;
    bool readerOnline;
};

extern RfidHandler rfidHandler;

#endif // RFID_HANDLER_H
