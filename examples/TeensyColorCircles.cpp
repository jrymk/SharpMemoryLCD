#include <SharpMemoryLCD.h>

LS013B7DH06 mlcd(10, 9);

void setup() {
    mlcd.begin();
}

void loop() {
    mlcd.fillScreen(MLCD_BLACK);
    uint16_t c[] = { MLCD_RED, MLCD_YELLOW, MLCD_GREEN, MLCD_CYAN, MLCD_BLUE, MLCD_MAGENTA };
    for (int i = 0; i < 6; i++)
        mlcd.fillCircle(64 + 44 * std::cos((millis() / 500.) - i / 3. * M_PI), 64 + 44 * std::sin((millis() / 500.) - i / 3. * M_PI), 20, c[i]);
    mlcd.fillCircle(64, 64, 20, MLCD_WHITE);

    mlcd.update();
}