#include "tm1637_driver.h"

// Bảng ánh xạ font ký tự số và chữ cơ bản trên 7 thanh
static const uint8_t DIGIT_MAP[] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
    SEG_B | SEG_C,                                         // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                 // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                 // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                         // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,         // 6
    SEG_A | SEG_B | SEG_C,                                 // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G          // 9
};

TM1637Driver::TM1637Driver(uint8_t clkPin, uint8_t dioPin)
    : _clkPin(clkPin), _dioPin(dioPin), _brightness(5) {}

void TM1637Driver::begin() {
    pinMode(_clkPin, OUTPUT);
    pinMode(_dioPin, OUTPUT);
    digitalWrite(_clkPin, HIGH);
    digitalWrite(_dioPin, HIGH);
    setBrightness(_brightness);
    clear();
}

void TM1637Driver::start() {
    digitalWrite(_dioPin, HIGH);
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(_dioPin, LOW);
    delayMicroseconds(5);
    digitalWrite(_clkPin, LOW);
    delayMicroseconds(5);
}

void TM1637Driver::stop() {
    digitalWrite(_clkPin, LOW);
    delayMicroseconds(5);
    digitalWrite(_dioPin, LOW);
    delayMicroseconds(5);
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(_dioPin, HIGH);
    delayMicroseconds(5);
}

bool TM1637Driver::writeByte(uint8_t data) {
    // 8 bit LSB first
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(_clkPin, LOW);
        delayMicroseconds(5);
        digitalWrite(_dioPin, (data & 0x01) ? HIGH : LOW);
        delayMicroseconds(5);
        digitalWrite(_clkPin, HIGH);
        delayMicroseconds(5);
        data >>= 1;
    }

    // Đọc bit ACK từ TM1637
    digitalWrite(_clkPin, LOW);
    pinMode(_dioPin, INPUT);
    delayMicroseconds(5);
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(5);
    bool ack = (digitalRead(_dioPin) == LOW);
    digitalWrite(_clkPin, LOW);
    pinMode(_dioPin, OUTPUT);
    delayMicroseconds(5);

    return ack;
}

void TM1637Driver::setBrightness(uint8_t brightness) {
    _brightness = brightness > 7 ? 7 : brightness;
    start();
    writeByte(0x88 | (_brightness & 0x07)); // 0x88: Display ON + độ sáng
    stop();
}

void TM1637Driver::clear() {
    uint8_t empty[4] = {0, 0, 0, 0};
    displaySegments(empty);
}

void TM1637Driver::displaySegments(const uint8_t segments[4]) {
    // Lệnh ghi dữ liệu tự động tăng địa chỉ (0x40)
    start();
    writeByte(0x40);
    stop();

    // Lệnh thiết lập địa chỉ bắt đầu DIG1 (0xC0)
    start();
    writeByte(0xC0);
    for (uint8_t i = 0; i < 4; i++) {
        writeByte(segments[i]);
    }
    stop();

    // Lệnh kích hoạt hiển thị
    start();
    writeByte(0x88 | (_brightness & 0x07));
    stop();
}

uint8_t TM1637Driver::charToSegment(char c) {
    if (c >= '0' && c <= '9') {
        return DIGIT_MAP[c - '0'];
    }

    switch (c) {
        case 'A': case 'a': return SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
        case 'B': case 'b': return SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
        case 'C':           return SEG_A | SEG_D | SEG_E | SEG_F;
        case 'c':           return SEG_D | SEG_E | SEG_G;
        case 'D': case 'd': return SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
        case 'E': case 'e': return SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
        case 'F': case 'f': return SEG_A | SEG_E | SEG_F | SEG_G;
        case 'G': case 'g': return SEG_A | SEG_C | SEG_D | SEG_E | SEG_F;
        case 'H':           return SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
        case 'h':           return SEG_C | SEG_E | SEG_F | SEG_G;
        case 'I': case 'i': return SEG_E | SEG_F;
        case 'J': case 'j': return SEG_B | SEG_C | SEG_D | SEG_E;
        case 'K': case 'k': return SEG_D | SEG_E | SEG_F | SEG_G;
        case 'L': case 'l': return SEG_D | SEG_E | SEG_F;
        case 'N': case 'n': return SEG_C | SEG_E | SEG_G;
        case 'O': case 'o': return SEG_C | SEG_D | SEG_E | SEG_G;
        case 'P': case 'p': return SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;
        case 'Q': case 'q': return SEG_A | SEG_B | SEG_C | SEG_F | SEG_G;
        case 'R': case 'r': return SEG_E | SEG_G;
        case 'S': case 's': return SEG_A | SEG_C | SEG_D | SEG_F | SEG_G;
        case 'T': case 't': return SEG_D | SEG_E | SEG_F | SEG_G;
        case 'U':           return SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
        case 'u':           return SEG_C | SEG_D | SEG_E;
        case 'Y': case 'y': return SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;
        case '-':           return SEG_G;
        case '_':           return SEG_D;
        case ' ':           return 0x00;
        default:            return 0x00;
    }
}

void TM1637Driver::displayString(const char* str) {
    uint8_t segs[4] = {0, 0, 0, 0};
    uint8_t idx = 0;

    for (size_t i = 0; str[i] != '\0' && idx < 4; i++) {
        segs[idx++] = charToSegment(str[i]);
    }
    displaySegments(segs);
}

void TM1637Driver::displayNumber(int num, bool leadingZeros) {
    uint8_t segs[4] = {0, 0, 0, 0};
    if (num < 0) num = 0;
    if (num > 9999) num = 9999;

    int d0 = (num / 1000) % 10;
    int d1 = (num / 100) % 10;
    int d2 = (num / 10) % 10;
    int d3 = num % 10;

    segs[0] = (leadingZeros || d0 > 0) ? DIGIT_MAP[d0] : 0;
    segs[1] = (leadingZeros || d0 > 0 || d1 > 0) ? DIGIT_MAP[d1] : 0;
    segs[2] = (leadingZeros || d0 > 0 || d1 > 0 || d2 > 0) ? DIGIT_MAP[d2] : 0;
    segs[3] = DIGIT_MAP[d3];

    displaySegments(segs);
}

void TM1637Driver::showMode(SystemMode mode) {
    switch (mode) {
        case MODE_ORGAN:
            displayString("OrG ");
            break;
        case MODE_JUKEBOX:
            displayString("JUKE");
            break;
        case MODE_TALKIE:
            displayString("tALk");
            break;
        case MODE_SFX:
            displayString("SFX ");
            break;
        default:
            displayString("VOI ");
            break;
    }
}

void TM1637Driver::showNote(const char* noteName, int octave) {
    uint8_t segs[4] = {0, 0, 0, 0};
    // Format: "C-4 " hoặc "CS4 "
    if (noteName != nullptr && noteName[0] != '\0') {
        segs[0] = charToSegment(noteName[0]);
        if (noteName[1] == '#' || noteName[1] == 'S' || noteName[1] == 's') {
            segs[1] = SEG_G; // Dấu gạch đại diện thăng (#)
        } else {
            segs[1] = SEG_G; // Dấu gạch ngang phân cách
        }
        segs[2] = (octave >= 0 && octave <= 9) ? DIGIT_MAP[octave] : 0;
        segs[3] = 0;
    }
    displaySegments(segs);
}

void TM1637Driver::showBpm(int bpm) {
    uint8_t segs[4] = {0, 0, 0, 0};
    if (bpm > 999) bpm = 999;
    segs[0] = (bpm >= 100) ? DIGIT_MAP[bpm / 100] : 0;
    segs[1] = (bpm >= 10) ? DIGIT_MAP[(bpm / 10) % 10] : 0;
    segs[2] = DIGIT_MAP[bpm % 10];
    segs[3] = charToSegment('b'); // Ký tự 'b' cho BPM
    displaySegments(segs);
}

void TM1637Driver::showSong(int songNum) {
    uint8_t segs[4];
    segs[0] = charToSegment('S');
    segs[1] = charToSegment('n');
    segs[2] = 0;
    segs[3] = (songNum >= 0 && songNum <= 9) ? DIGIT_MAP[songNum] : 0;
    displaySegments(segs);
}

void TM1637Driver::showVolume(int vol) {
    uint8_t segs[4];
    segs[0] = charToSegment('U');
    segs[1] = charToSegment('o');
    segs[2] = charToSegment('L');
    segs[3] = (vol >= 0 && vol <= 9) ? DIGIT_MAP[vol] : 0;
    displaySegments(segs);
}
