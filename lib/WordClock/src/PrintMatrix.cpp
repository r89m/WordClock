#include "PrintMatrix.h"

void PrintMatrix::writeScreen() {
    // Check whether the display needs to be sent to the serial port (do it once per second)
    if (millis() - serialOutputLastTime > 1000) {
        printer.println("================");
        // Draw the current view
        for (uint8_t row = 0; row < _height; row++) {
            printer.print("|");
            for (uint8_t col = 0; col < _width; col++) {
                const char letter = MATRIX_LETTERS[row * _width + col];
                printer.print(isPixelOn(col, row) ? letter : ' ');
            }
            printer.println("|");
        }
        printer.println("================");

        serialOutputLastTime = millis();
    }
}