#ifndef SHARPMLCD_DMA_H
#define SHARPMLCD_DMA_H

// A DMA version of the SharpMemoryLCD library

// Auto updating the display at a fixed framerate is implemented for Teensies only for now.

#include <Arduino.h>
#include <DMAChannel.h>
#include <SPI.h>

#define MLCD_WIDTH 400
#define MLCD_HEIGHT 240
#define MLCD_TXBUFFER_WORDS 6241      // (((MLCD_WIDTH / 8 + 2) * MLCD_HEIGHT + 2) / 2)
#define MLCD_MAX_FPS 120              // based on the SPI clock speed
#define MLCD_VCOM_INVERT_DURATION 100 // 500ms = 1Hz (recommended 0.5Hz ~ 10Hz)

#ifdef __IMXRT1062__
#define MLCD_ENABLE_AUTO_UPDATE
#endif

#ifdef MLCD_ENABLE_AUTO_UPDATE
// auto update with TeensyTimerTool is implemented for Teensies only, obviously.
// to update the frameBuffer, call pauseUpdate(), and then call sendFrame(), it will resume auto updating automatically.
#include <TeensyTimerTool.h>
#endif

/*
 * DISP pin is not used, tie it to high because it isn't really useful to turn the display off, it doesn't
 * save much power. stop updating the display does save quite some power.
 * EXTCOMIN pin is not used, tie EXTMODE to low.
 */

namespace MemoryLCD {

// at 14MHz, the maximum framerate is 123.32Hz (measured)
uint16_t DMAMEM txBuffer[MLCD_TXBUFFER_WORDS] __attribute__((aligned(32)));
SPISettings spiSettings(14000000, LSBFIRST, SPI_MODE0);
EventResponder callbackHandler;
volatile bool dmaBusy = false;

uint8_t csPin;

uint16_t *frameBuffer = nullptr; // 400 divides 16 and also is the unit for the tx buffer

#ifdef MLCD_ENABLE_AUTO_UPDATE
TeensyTimerTool::OneShotTimer nextFrameTrigger;
uint8_t targetFramerate = 60;
#endif

void sendFrame() {
    uint16_t *d = txBuffer;
    uint16_t *s = frameBuffer;
    uint16_t line = 0;
    uint8_t mode = ((millis() / MLCD_VCOM_INVERT_DURATION) & 0b1) ? 0b011 : 0b001;

    while (dmaBusy)
        yield();

    dmaBusy = true;

    digitalWriteFast(10, HIGH);

    // output buffer processing takes 47 microseconds
    while (line < (MLCD_HEIGHT << 8)) {
        *(d++) = line += (1 << 8);
        if (line == (1 << 8))
            *(d - 1) |= mode;
        memcpy(d, s, MLCD_WIDTH / 8);
        d += (MLCD_WIDTH / 16);
        s += (MLCD_WIDTH / 16);
    }
    *(d++) = 0;

    SPI.beginTransaction(spiSettings);
    SPI.transfer((void *)txBuffer, nullptr, MLCD_TXBUFFER_WORDS * 2, callbackHandler);
}

void callback(EventResponderRef eventResponder) {
    SPI.endTransaction();
    digitalWriteFast(10, LOW);
    dmaBusy = false;

#ifdef MLCD_ENABLE_AUTO_UPDATE
    nextFrameTrigger.trigger(1000000 / targetFramerate);
#endif
}

#ifdef MLCD_ENABLE_AUTO_UPDATE
void pauseUpdate() {
    nextFrameTrigger.stop();
}
#endif

void begin(uint8_t cs) {
    pinMode(cs, OUTPUT);
    csPin = cs;
    callbackHandler.attachImmediate(&callback);
    SPI.begin();

    frameBuffer = new uint16_t[MLCD_WIDTH * MLCD_HEIGHT / 16];

#ifdef MLCD_ENABLE_AUTO_UPDATE
    nextFrameTrigger.begin(sendFrame);
    nextFrameTrigger.trigger(1000000 / targetFramerate);
#endif
}

} // namespace MemoryLCD

#endif // SHARPMLCD_DMA_H