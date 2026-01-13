#include "PrintMatrix.h"

void PrintMatrix::writeScreen() {
    // Check whether the display needs to be sent to the serial port (do it once per second)
    if (millis() - serialOutputLastTime > 1000) {
        Serial.println("==============");
        // Draw the current view
        for (uint8_t row = 0; row < _height; row++) {
            Serial.print("|");
            for (uint8_t col = 0; col < _width; col++) {
                Serial.print(isPixelOn(col, row) ? 'X' : ' ');
            }
            Serial.println('|');
        }
        Serial.println("==============");

        serialOutputLastTime = millis();
    }
}