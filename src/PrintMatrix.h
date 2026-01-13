#ifndef NODEMCU_FIXES_PRINTWORDCLOCK_H
#define NODEMCU_FIXES_PRINTWORDCLOCK_H

#include "Matrix.h"

inline const char *MATRIX_LETTERS = "ABCDEF"
                                    "HIJKL";

class PrintMatrix : public Matrix {
    public:
    using Matrix::Matrix;

    void handlePixel(uint8_t x, uint8_t y) override {
        // Do nothing
    }

    void writeScreen() override;

    void clearScreen() override {
        // Do nothing
    };

    private:
    uint32_t serialOutputLastTime = 0;
};

#endif // NODEMCU_FIXES_PRINTWORDCLOCK_H
