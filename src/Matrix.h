/*
 * Matrix.cpp
 *
 * Derived from Nicholas Zambetti's Matrix library
 *
   Matrix.h - Max7219 LED Matrix library for Arduino & Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MATRIX_H_
#define MATRIX_H_

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "Sprite.h"
#include "ClockWord.h"

#define PIXEL_ON true
#define PIXEL_OFF false

#define FLASH_LIT 1
#define FLASH_UNLIT 2

#define MATRIX_BUFFER_DATA_TYPE uint8_t
#define MATRIX_BUFFER_DATA_TYPE_SIZE sizeof(MATRIX_BUFFER_DATA_TYPE)
#define MATRIX_BUFFER_DATA_TYPE_SIZE_BITS 8 * MATRIX_BUFFER_DATA_TYPE_SIZE

class IMatrix {
    public:
    virtual ~IMatrix() = default;

    virtual void begin() = 0;

    virtual void clear() = 0;

    virtual void setAllPixels(uint8_t) = 0;
    virtual void setPixels(uint8_t, uint8_t, uint8_t) = 0;
    virtual void setPixels(uint8_t, uint8_t, Sprite) = 0;
    virtual void setPixels(ClockWord) = 0;
    virtual void overwriteBuffer(uint8_t *) = 0;

    virtual void setBrightness(uint8_t brightness) = 0;

    virtual void flashOff() = 0;

    virtual void update() = 0;
};

class MatrixCollection : public IMatrix {
    public:
    MatrixCollection(const std::vector<IMatrix *> &matrices) : matrices(matrices) {};

    void begin() override {
        for (const auto matrix : matrices) {
            matrix->begin();
        }
    }

    void clear() override {
        for (const auto matrix : matrices) {
            matrix->clear();
        }
    }

    void update() override {
        for (const auto matrix : matrices) {
            matrix->update();
        }
    }

    void setAllPixels(uint8_t pixelValue) override {
        for (const auto matrix : matrices) {
            matrix->setAllPixels(pixelValue);
        }
    }

    void setPixels(uint8_t x, uint8_t y, uint8_t value) override {
        for (const auto matrix : matrices) {
            matrix->setPixels(x, y, value);
        }
    }

    void setPixels(uint8_t x, uint8_t y, Sprite sprite) override {
        for (const auto matrix : matrices) {
            matrix->setPixels(x, y, sprite);
        }
    }
    void setPixels(ClockWord clockWord) override {
        for (const auto matrix : matrices) {
            matrix->setPixels(clockWord);
        }
    }

    void overwriteBuffer(uint8_t *buffer) override {
        for (const auto matrix : matrices) {
            matrix->overwriteBuffer(buffer);
        }
    }

    void setBrightness(uint8_t brightness) override {
        for (const auto matrix : matrices) {
            matrix->setBrightness(brightness);
        }
    }

    void flashOff() override {
        for (const auto matrix : matrices) {
            matrix->flashOff();
        }
    }

    private:
    std::vector<IMatrix *> matrices;
};

class Matrix : public IMatrix {
    private:
    MATRIX_BUFFER_DATA_TYPE *_buffer;
    uint16_t _bufferSize;
    boolean _flash_enabled;
    uint16_t _flash_on_duration;
    uint16_t _flash_off_duration;
    uint32_t _flash_last_timestamp;
    uint8_t _flash_current_status;
    boolean bufferHasNewData;

    void buffer(uint8_t, uint8_t, uint8_t);
    void getPixelIndexAndBit(uint8_t, uint8_t, uint8_t &, uint8_t &);

    protected:
    uint8_t _width;
    uint8_t _height;
    uint8_t _brightness = 15; // HT1632C Brightness ranges from 0 - 16
    boolean displayHasNewBrightness;

    public:
    Matrix(uint8_t, uint8_t, uint16_t);

    void begin() override {
        // Do nothing by default
    };
    void clear() override;
    void update() override;

    virtual void handlePixel(uint8_t x, uint8_t y) = 0;

    virtual void writeScreen() = 0;
    virtual void clearScreen() = 0;

    void setBrightness(uint8_t) override;

    void flashOn();
    void flashOn(uint16_t);
    void flashOn(uint16_t, uint16_t);
    void flashOff() override;
    void setFlash(uint16_t);
    void setFlash(uint16_t, uint16_t);

    uint8_t getPixel(uint8_t, uint8_t);
    boolean isPixelOn(uint8_t, uint8_t);
    void setAllPixels(uint8_t) override;
    void setPixels(uint8_t, uint8_t, uint8_t) override;
    void setPixels(uint8_t, uint8_t, Sprite) override;
    void setPixels(ClockWord) override;
    void overwriteBuffer(uint8_t *) override;
};

#endif /* MATRIX_H_ */