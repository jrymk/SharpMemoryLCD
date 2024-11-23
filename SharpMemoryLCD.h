// Comment out to remove Adafruit GFX dependency. Only drawPixel will work.
#define USE_ADAFRUIT_GFX

#include <Arduino.h>
#include <SPI.h>

#ifdef USE_ADAFRUIT_GFX
#include <Adafruit_GFX.h>
#endif

#define SPI_CLASS SPI
#define SPI_CLOCK 12000000 // 12MHz

#define MLCD_WHITE   0xFFFF
#define MLCD_BLACK   0x0000
#define MLCD_RED     0b1111100000000000
#define MLCD_GREEN   0b0000011111100000
#define MLCD_BLUE    0b0000000000011111
#define MLCD_YELLOW  0b1111111111100000
#define MLCD_CYAN    0b0000011111111111
#define MLCD_MAGENTA 0b1111100000011111

/*
 * DISP pin is not used, tie it to high because it isn't really useful to turn the display off, it doesn't
 * save much power. stop updating the display does save quite some power.
 * EXTCOMIN pin is not used, tie EXTMODE to low.
 */

template<uint16_t _WIDTH, uint16_t _HEIGHT>
class SharpMemoryLCD
#ifdef USE_ADAFRUIT_GFX
    : public Adafruit_GFX
#endif
{
protected:
    int csPin = 10;                                       // defaults
    int extComInPin = 9;                                  // defaults, set to negative value to use COM signal serial input (pull EXTMODE low)
    const int vcomInvertMs = 100;                         // minimum 0.5hz, which means a cycle every 2 seconds, so invert every second at the very least?
    const int invertOnUpdateTolerance = vcomInvertMs / 3; // prefer inverting on display data update to save time, only when there are no data updates after this time will the checkInvert function explicitly invert
    uint32_t lastInversion = 0;
    SPISettings spiSettings;
    bool vcomStatus = false;

public:
    uint8_t* frameBuffer = nullptr;

    SharpMemoryLCD(int cs, int extComIn) :
#ifdef USE_ADAFRUIT_GFX
            Adafruit_GFX(_WIDTH, _HEIGHT),
#endif
            csPin(cs), extComInPin(extComIn),
            spiSettings(SPI_CLOCK, LSBFIRST, SPI_MODE0) {
        frameBuffer = (uint8_t*)malloc(_WIDTH * _HEIGHT / 8);
    }

    void begin() {
        pinMode(csPin, OUTPUT);
        digitalWriteFast(csPin, LOW);
        pinMode(extComInPin, OUTPUT);
        digitalWriteFast(extComInPin, LOW);
        SPI_CLASS.begin();
    }

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (x < 0 || y < 0 || x >= _WIDTH || y >= _HEIGHT) return;
        if (color & 0x8000) // if the final bit is 1, or white (yes, this won't work with RGB luminances)
            frameBuffer[(y * _WIDTH + x) / 8] |= (1 << (x & 0b111));
        else
            frameBuffer[(y * _WIDTH + x) / 8] &= ~(1 << (x & 0b111));
    }

    virtual void update(uint16_t lineStart = 0, uint16_t lineEnd = _HEIGHT) {
        if (millis() - lastInversion >= vcomInvertMs) {
            lastInversion = millis();
            vcomStatus = !vcomStatus;
        }
        uint8_t mode = 0b001 | (vcomStatus ? 0b010 : 0b000); // data update mode with current VCOM status
        auto data = frameBuffer + (lineStart * _WIDTH / 8);
        SPI_CLASS.beginTransaction(spiSettings);
        digitalWriteFast(csPin, HIGH);
        SPI_CLASS.transfer(mode);
        for (auto l = lineStart + 1; l <= lineEnd; l++) {
            SPI_CLASS.transfer(l); // SHARP starts its line index at 1, yeww!
            auto i = _WIDTH / 8;
            while (i--)
                SPI_CLASS.transfer(*data++);
            SPI_CLASS.transfer(0x00); // dummy
        }
        SPI_CLASS.transfer(0x00); // dummy
        digitalWriteFast(csPin, LOW);
        SPI_CLASS.endTransaction();
    }

    // this routine does not take much time, try calling it in the main loop
    void checkInvert() {
        if (millis() - lastInversion >= vcomInvertMs + invertOnUpdateTolerance)
            invertVCOM();
    }

    // call this periodically (minimum 0.5hz)
    void invertVCOM() {
        if (extComInPin < 0) {
            SPI_CLASS.beginTransaction(spiSettings);
            digitalWriteFast(csPin, HIGH);
            SPI_CLASS.transfer(vcomStatus ? 0b010 : 0b000);
            SPI_CLASS.transfer(0x00); // dummy
            digitalWriteFast(csPin, LOW);
            SPI_CLASS.endTransaction();
        } else {
            digitalWrite(extComInPin, HIGH);
            delayMicroseconds(1);
            digitalWrite(extComInPin, LOW);
        }
        lastInversion = millis();
        vcomStatus = !vcomStatus;
    }
};


template<uint16_t _WIDTH, uint16_t _HEIGHT>
class SharpColorMemoryLCD : public SharpMemoryLCD<_WIDTH, _HEIGHT> {
public:
    SharpColorMemoryLCD(int cs, int extComIn) : SharpMemoryLCD<_WIDTH, _HEIGHT>(cs, extComIn) {
        free(SharpMemoryLCD<_WIDTH, _HEIGHT>::frameBuffer); // constructor should allocated a 1bpp frame buffer
        SharpMemoryLCD<_WIDTH, _HEIGHT>::frameBuffer = (byte*)malloc(_WIDTH * _HEIGHT * 3 / 8);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (x < 0 || y < 0 || x >= _WIDTH || y >= _HEIGHT) return;
        uint16_t c = (color & 0b1000000000000000 ? 0b001 : 0) // red
            | (color & 0b0000010000000000 ? 0b010 : 0) // green
            | (color & 0b0000000000010000 ? 0b100 : 0); // blue

        // is this evil? to read uint8 as uint16 to solve "groups of 3 bits" crossing byte boundary
        (*(uint16_t*)(SharpMemoryLCD<_WIDTH, _HEIGHT>::frameBuffer + (y * _WIDTH + x) * 3 / 8)) &= ~(0b111 << ((x * 3) & 0b111));
        (*(uint16_t*)(SharpMemoryLCD<_WIDTH, _HEIGHT>::frameBuffer + (y * _WIDTH + x) * 3 / 8)) |= c << ((x * 3) & 0b111);
    }

    void update(uint16_t lineStart = 0, uint16_t lineEnd = _HEIGHT) override {
        auto data = SharpMemoryLCD<_WIDTH, _HEIGHT>::frameBuffer + (lineStart * _WIDTH / 8);
        SPI_CLASS.beginTransaction(SharpMemoryLCD<_WIDTH, _HEIGHT>::spiSettings);
        digitalWriteFast(SharpMemoryLCD<_WIDTH, _HEIGHT>::csPin, HIGH);
        SPI_CLASS.transfer(0b001);
        for (auto l = lineStart + 1; l <= lineEnd; l++) {
            SPI_CLASS.transfer(l);
            auto i = _WIDTH * 3 / 8;
            while (i--)
                SPI_CLASS.transfer(*data++);
            SPI_CLASS.transfer(0x00); // dummy
        }
        SPI_CLASS.transfer(0x00); // dummy
        digitalWriteFast(SharpMemoryLCD<_WIDTH, _HEIGHT>::csPin, LOW);
        SPI_CLASS.endTransaction();
    }
};

template<uint16_t _WIDTH, uint16_t _HEIGHT>
class SharpMemoryLCD10bitAddr : public SharpMemoryLCD<_WIDTH, _HEIGHT> { // untested, as I don't own one
public:
    SharpMemoryLCD10bitAddr(int cs, int extComIn) : SharpMemoryLCD<_WIDTH, _HEIGHT>(cs, extComIn) {}

    void update(uint16_t lineStart = 0, uint16_t lineEnd = _HEIGHT) override {
        auto data = SharpMemoryLCD<_WIDTH, _HEIGHT>::frameBuffer + (lineStart * _WIDTH / 8);
        SPI_CLASS.beginTransaction(SharpMemoryLCD<_WIDTH, _HEIGHT>::spiSettings);
        digitalWriteFast(SharpMemoryLCD<_WIDTH, _HEIGHT>::csPin, HIGH);
        SPI_CLASS.transfer(0b001 | ((lineStart + 1) << 6)); // last 2 bits of address
        for (auto l = lineStart + 1; l <= lineEnd; l++) {
            SPI_CLASS.transfer(l >> 2);
            auto i = _WIDTH / 8;
            while (i--)
                SPI_CLASS.transfer(*data++);
            SPI_CLASS.transfer((l + 1) << 6); // last 2 bits of next address
        }
        SPI_CLASS.transfer(0x00); // dummy
        digitalWriteFast(SharpMemoryLCD<_WIDTH, _HEIGHT>::csPin, LOW);
        SPI_CLASS.endTransaction();
    }
};

using LS027B7DH01 = SharpMemoryLCD<400, 240>;
using LS032B7DD02 = SharpMemoryLCD10bitAddr<336, 536>;
using LS044Q7DH01 = SharpMemoryLCD<320, 240>;
using LS006B7DH03 = SharpMemoryLCD<64, 64>;
using LS011B7DH03 = SharpMemoryLCD<160, 68>;
using LS013B7DH06 = SharpColorMemoryLCD<128, 128>;