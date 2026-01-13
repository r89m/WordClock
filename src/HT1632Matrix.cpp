#include "HT1632Matrix.h"

#include "Adafruit_HT1632.h"

Adafruit_HT1632 MatrixDriver = Adafruit_HT1632(D5, D6, D7);

void HT1632Matrix::begin() { MatrixDriver.begin(ADA_HT1632_COMMON_16NMOS); }

void HT1632Matrix::handlePixel(uint8_t row, uint8_t col) {
    // Should really be: (row * 16) + col, but I messed up the wiring
    uint8_t pixel = ((13 - row) * 16) + col;

    // More fixes my dodgy wiring!
    if (col == 12) {
        pixel -= 11;
    } else if (col == 13) {
        pixel -= 13;
        // pixel += 2;  // Move the first and second columns back to the beginning
    } else {
        pixel += 2;
    }

    if (isPixelOn(col, row)) {
        MatrixDriver.setPixel(pixel);
    } else {
        MatrixDriver.clrPixel(pixel);
    }
}

void HT1632Matrix::writeScreen() {

    // Output the new display
    MatrixDriver.writeScreen();

    if (displayHasNewBrightness) {
        // Set the brightness
        MatrixDriver.setBrightness(_brightness);
        displayHasNewBrightness = false;
    }
}

void HT1632Matrix::clearScreen() { MatrixDriver.clearScreen(); }