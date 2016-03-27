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

class Matrix
{
	private: 
	MATRIX_BUFFER_DATA_TYPE* _buffer;
	uint8_t _width;
	uint8_t _height;
  uint16_t _bufferSize;
	uint8_t _brightness = 15;	// HT1632C Brightness ranges from 0 - 16
	boolean _flash_enabled;
  uint16_t _flash_on_duration;
  uint16_t _flash_off_duration;
  uint32_t _flash_last_timestamp;
  uint8_t _flash_current_status;
  boolean bufferHasNewData;
  boolean displayHasNewBrightness;
	
	void buffer(uint8_t, uint8_t, uint8_t);
  void getPixelIndexAndBit(uint8_t, uint8_t, uint8_t&, uint8_t&);
	
	public: 
	Matrix(uint8_t, uint8_t, uint16_t);
  void init();
	void clear();
	void update();
	
	void setBrightness(uint8_t);
	
	void flashOn();
	void flashOn(uint16_t);
	void flashOn(uint16_t, uint16_t);
	void flashOff();
	void setFlash(uint16_t);
	void setFlash(uint16_t, uint16_t);

	uint8_t getPixel(uint8_t, uint8_t);
	boolean isPixelOn(uint8_t, uint8_t);
	void setAllPixels(uint8_t);
	void setPixels(uint8_t, uint8_t, uint8_t);
	void setPixels(uint8_t, uint8_t, Sprite);
	void setPixels(ClockWord);
  void overwriteBuffer(uint8_t*);
	
};



#endif /* MATRIX_H_ */