# Sharp Memory LCD
Single header Arduino framework library for SHARP Memory LCD Displays, with Adafruit_GFX compatibility

![IMG_0026](https://github.com/jrymk/SharpMemoryLCD/assets/39593345/5379979d-34dd-4bc9-a96e-de3c043a130e)
![IMG_0033](https://github.com/jrymk/SharpMemoryLCD/assets/39593345/bb936a27-98ff-4d61-85b8-a9f5a0ae288e)

### Example
```cpp
#include <SharpMemoryLCD.h>
#include <SoftwareSerial.h>
#include <TeensyTimerTool.h>

LS013B7DH06 mlcd(10, 9);
TeensyTimerTool::PeriodicTimer extComUpdate;

void setup() {
    Serial.begin(115200);
    mlcd.begin();
    extComUpdate.begin([] { mlcd.VCOMInvert(); }, 500ms);
}

void loop() {
    mlcd.fillScreen(MLCD_BLACK);
    uint16_t c[] = { MLCD_RED, MLCD_YELLOW, MLCD_GREEN, MLCD_CYAN, MLCD_BLUE, MLCD_MAGENTA };
    for (int i = 0; i < 6; i++)
        mlcd.fillCircle(64 + 44 * std::cos((millis() / 500.) - i / 3. * M_PI), 64 + 44 * std::sin((millis() / 500.) - i / 3. * M_PI), 20, c[i]);
    mlcd.fillCircle(64, 64, 20, MLCD_WHITE);
    mlcd.update();
}
```

![GIF 2023-6-28 上午 01-40-58](https://github.com/jrymk/SharpMemoryLCD/assets/39593345/dd95ae00-6a58-400c-abb7-d2148f4c5a20)
![GIF 2023-6-28 上午 01-42-09](https://github.com/jrymk/SharpMemoryLCD/assets/39593345/8adb8c76-9d15-49b6-b928-3d876637092f)
