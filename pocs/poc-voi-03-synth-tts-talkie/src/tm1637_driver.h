#ifndef VOI03_TM1637_DRIVER_H
#define VOI03_TM1637_DRIVER_H

#include <Arduino.h>
#include "config.h"

// Segment bitmasks (Standard TM1637: a=0x01, b=0x02, c=0x04, d=0x08, e=0x10, f=0x20, g=0x40, dp=0x80)
#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x10
#define SEG_F 0x20
#define SEG_G 0x40
#define SEG_DP 0x80

class TM1637Driver {
public:
    TM1637Driver(uint8_t clkPin = PIN_TM1637_CLK, uint8_t dioPin = PIN_TM1637_DIO);

    void begin();
    void setBrightness(uint8_t brightness); // 0 to 7
    void clear();

    void displaySegments(const uint8_t segments[4]);
    void displayString(const char* str);
    void displayNumber(int num, bool leadingZeros = false);

    void showMode(SystemMode mode);
    void showNote(const char* noteName, int octave);
    void showBpm(int bpm);
    void showSong(int songNum);
    void showVolume(int vol);

private:
    uint8_t _clkPin;
    uint8_t _dioPin;
    uint8_t _brightness;

    void start();
    void stop();
    bool writeByte(uint8_t data);
    uint8_t charToSegment(char c);
};

#endif // VOI03_TM1637_DRIVER_H
