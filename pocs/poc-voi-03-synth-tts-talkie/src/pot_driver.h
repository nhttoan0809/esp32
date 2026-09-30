#ifndef VOI03_POT_DRIVER_H
#define VOI03_POT_DRIVER_H

#include <Arduino.h>
#include "config.h"

class PotentiometerDriver {
public:
    PotentiometerDriver(uint8_t pin = PIN_POTENTIOMETER);

    void begin();
    void update(); // Gọi định kỳ trong loop (non-blocking)

    uint16_t getRaw() const;
    uint16_t getFiltered() const;
    int16_t getPitchBendHz() const;
    uint16_t getTempoBpm() const;
    bool hasChanged() const;

private:
    uint8_t _pin;
    uint16_t _samples[POT_SMA_SAMPLES];
    uint8_t _sampleIndex;
    uint32_t _sampleSum;
    uint16_t _filteredValue;
    uint16_t _lastReportedValue;
    bool _changed;
    uint32_t _lastReadTime;
};

#endif // VOI03_POT_DRIVER_H
