#ifndef NODEMCU_FIXES_PRINTWORDCLOCK_H
#define NODEMCU_FIXES_PRINTWORDCLOCK_H

#include "Matrix.h"

inline const char *MATRIX_LETTERS = "ITGISYEMAKIHKB"
                                    "LBTENHNTWENTYR"
                                    "YWQFQUARTERXIU"
                                    "ZTHIRTYGWFIVEL"
                                    "CJNDMINUTESLOA"
                                    "KATORNCPASTBNH"
                                    "IQBHONEGFLIDPG"
                                    "HTWOMETHREEWUI"
                                    "TPFOURLXUFIVEV"
                                    "JSIXJBPSEVENBS"
                                    "VHDEIGHTQONINE"
                                    "CTENUTELEVENPO"
                                    "MUHTWELVEDFMUH"
                                    "FEHJGIOCLOCKXR";

struct Printer {
    virtual void print(const char character) = 0;
    virtual void print(const char *output) = 0;
    virtual void println(const char *output) = 0;
};

class PrintMatrix : public Matrix {
    public:
    PrintMatrix(Printer &printer, uint8_t width, uint8_t height, uint16_t bufferSize) : Matrix(width, height, bufferSize), printer(printer) {}

    void handlePixel(uint8_t x, uint8_t y) override {
        // Do nothing
    }

    void writeScreen() override;

    void clearScreen() override {
        // Do nothing
    };

    private:
    Printer &printer;
    uint32_t serialOutputLastTime = 0;
};

#endif // NODEMCU_FIXES_PRINTWORDCLOCK_H
