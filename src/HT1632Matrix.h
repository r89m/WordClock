#ifndef NODEMCU_FIXES_HT1632WORDCLOCK_H
#define NODEMCU_FIXES_HT1632WORDCLOCK_H

#include "Matrix.h"

class HT1632Matrix : public Matrix {
    public:
    using Matrix::Matrix;

    void begin() override;

    void handlePixel(uint8_t row, uint8_t col) override;

    void writeScreen() override;

    void clearScreen() override;
};

#endif // NODEMCU_FIXES_HT1632WORDCLOCK_H
