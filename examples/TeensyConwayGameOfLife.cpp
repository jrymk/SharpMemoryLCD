#include <SharpMemoryLCD.h>
#include <TeensyTimerTool.h>
#include <Fonts/FreeMonoBold12pt7b.h>

LS027B7DH01 mlcd(10, 9);
TeensyTimerTool::PeriodicTimer extComUpdate;

bool game[12][16] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
bool temp[12][16];


void iterate() {
    for (int i = 1; i < 15; i++) {
        for (int j = 1; j < 11; j++) {
            int neighbors = game[j - 1][i - 1] + game[j][i - 1] + game[j + 1][i - 1];
            neighbors += game[j - 1][i] + game[j + 1][i];
            neighbors += game[j - 1][i + 1] + game[j][i + 1] + game[j + 1][i + 1];
            if (game[j][i]) temp[j][i] = (neighbors == 2 || neighbors == 3);
            else temp[j][i] = (neighbors == 3);
        }
    }
    std::swap(temp, game);
}

#define SPEED 4
#define CELL_SIZE 20
int iterations = 0;
uint64_t lastIteration = 0;
uint64_t timer;

void setup() {
    mlcd.begin();
    extComUpdate.begin([] { mlcd.VCOMInvert(); }, 500ms);
    lastIteration = millis();
    mlcd.setFont(&FreeMonoBold12pt7b);
}

void loop() {
    mlcd.fillScreen(MLCD_WHITE);

    int offset = (millis() / SPEED / 10) - iterations / 10 * CELL_SIZE;

    for (int i = offset % CELL_SIZE; i < mlcd.width(); i += CELL_SIZE)
        mlcd.drawFastVLine(i, 0, mlcd.height(), MLCD_BLACK);
    for (int i = CELL_SIZE - 1; i < mlcd.height(); i += CELL_SIZE)
        mlcd.drawFastHLine(0, i, mlcd.width(), MLCD_BLACK);

    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 12; j++)
            if (game[j][i])
                mlcd.fillRect(20 + offset + CELL_SIZE * i, j * CELL_SIZE, CELL_SIZE, CELL_SIZE, MLCD_BLACK);

    if (millis() - lastIteration >= SPEED * CELL_SIZE) {
        lastIteration += SPEED * CELL_SIZE;
        iterate();
        iterations++;
        if (iterations % 10 == 0) {
            for (int i = 15; i > 0; i--)
                for (int j = 0; j < 12; j++)
                    game[j][i] = game[j][i - 1];
        }
    }

    mlcd.setCursor(3, 17);
    mlcd.setTextColor(MLCD_BLACK);
    mlcd.printf("%dfps", int(1000000. / (micros() - timer) + .5));
    timer = micros();
    mlcd.update();
}