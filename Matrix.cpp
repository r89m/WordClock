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

//#define OUTPUT_TO_SERIAL

// Row selection shift register pins. Do not use 3, 9, 10, 11 or 13!!
#define PIN_SHIFTREGISTER_LATCH 6
#define PIN_SHIFTREGISTER_DATA 8
#define PIN_SHIFTREGISTER_CLOCK 7

#include "Matrix.h"
#include <Tlc5940.h>

// Define function prototype
void pulseClockPin();

Matrix::Matrix(uint8_t width, uint8_t height){
	
	_width = width;
	_height = height;
	_buffer = (uint8_t*)calloc(_width, (_height / sizeof(uint8_t)) + 1);
	
	// Initialise TLC5940 Driver
	Tlc.init();
	
	// Setup the shift register pins
	pinMode(PIN_SHIFTREGISTER_LATCH, OUTPUT);
	pinMode(PIN_SHIFTREGISTER_CLOCK, OUTPUT);
	pinMode(PIN_SHIFTREGISTER_DATA, OUTPUT);
}


void Matrix::buffer(uint8_t x, uint8_t y, uint8_t value){
	
	// Silently fail if buffer has not been initialised
	if(!_buffer) return;
	
	// Check whether the x and y values are valid
	if(x >= _width) return;
	if(y >= _height) return;
	
	uint8_t pos = x + (_width * y);
	
	// Record value in buffer
	_buffer[pos] = value;
}


/*** User API ***/

void Matrix::clear(){
	
	for(uint8_t i = 0; i < _width; i++){
		for(uint8_t j = 0; j < _height; j++){
			buffer(i, j, PIXEL_OFF);
		}
	}
	
}

void Matrix::loop(){
	
	#if defined(OUTPUT_TO_SERIAL)
		// Check whether multiplexing needs to be moved along one column
		if(millis() - _multiplexLastTimestamp > 1000){
			
			Serial.println(millis() - _multiplexLastTimestamp, DEC);
			
			// Record the current time
			_multiplexLastTimestamp = millis();
		
			Serial.println("==============");
			// Draw the current view
			for(uint8_t col = 0; col < _height; col++){
				Serial.print("|");
				for(uint8_t j = 0; j < _width; j++){
					Serial.print(_buffer[j + (_width * col)] == PIXEL_ON ? 'X' : ' ');	
				}
				Serial.println('|');
			}
			Serial.println("==============");
		
			// Move on to the next column
			_multiplexCurrentIndex++;
		
			// Wrap around if we've reached the end
			if(_multiplexCurrentIndex == _width){
				_multiplexCurrentIndex = 0;
			}
		
		}
	#else
		// Check whether multiplexing needs to be moved along one column
		if(micros() - _multiplexLastTimestamp > _multiplexPeriod){
			// Record the current time
			_multiplexLastTimestamp = micros();
			
			Tlc.clear();
			
			// Draw the current view
			for(uint8_t col = 0; col < _width; col++){
				/*
				Serial.print(_multiplexCurrentIndex);
				Serial.print(":");
				Serial.print(col, DEC);
				Serial.print(" - ");
				*/
				
				if(isPixelOn(col, _multiplexCurrentIndex)){
					Tlc.set(col + 1, _brightness); // Add 1 so that we use all the outputs on one side
					//Serial.println(_brightness, DEC);	
				} else {
					//Serial.println(0, DEC);
				}
			}
			
			digitalWrite(PIN_SHIFTREGISTER_LATCH, LOW);
			uint16_t shiftBit = 0xFFFF;
			bitClear(shiftBit, _multiplexCurrentIndex + 1);
			//Serial.print("Shift value: ");
			//Serial.println(shiftBit, DEC);
			shiftOut(PIN_SHIFTREGISTER_DATA, PIN_SHIFTREGISTER_CLOCK, MSBFIRST, shiftBit);
			// For the first row, send a one to the shift registers
			/*
			if(_multiplexCurrentIndex == 0){
				digitalWrite(PIN_SHIFTREGISTER_DATA, true);
				pulseClockPin();
				digitalWrite(PIN_SHIFTREGISTER_DATA, false);
			} else {
				// For any others, just pulse the clock pin to shift the bit along
				pulseClockPin();
			}
			*/
			
			
			// Update the LED driver and latch the shift register to display this row
			Tlc.update();
			digitalWrite(PIN_SHIFTREGISTER_LATCH, HIGH);
				
			// Move on to the next row
			_multiplexCurrentIndex++;
			
			// Wrap around if we've reached the end
			if(_multiplexCurrentIndex == _height){
				_multiplexCurrentIndex = 0;
			}
			
		}
	#endif
}

void Matrix::setBrightness(uint8_t brightness){
	
	// Make the 0 - 255 brightness to 0 - 4095
	_brightness = map(brightness, 0, 255, 0, 4095);	
}

void Matrix::flashOn(){
	
	_flash_enabled = true;
}

void Matrix::flashOn(uint16_t duration){
	
	setFlash(duration);
	flashOn();
}

void Matrix::flashOn(uint16_t on_duration, uint16_t off_duration){
	
	setFlash(on_duration, off_duration);
	flashOn();
}

void Matrix::flashOff(){
	
	_flash_enabled = false;
}

void Matrix::setFlash(uint16_t duration){
	
	_flash_on_duration = duration;
	_flash_off_duration = duration;
}

void Matrix::setFlash(uint16_t on_duration, uint16_t off_duration){
	
	_flash_on_duration = on_duration;
	_flash_off_duration = off_duration;
}


uint8_t Matrix::getPixel(uint8_t x, uint8_t y){
	
	return _buffer[x + (_width * y)];
}

boolean Matrix::isPixelOn(uint8_t x, uint8_t y){
	
	return (getPixel(x, y) == PIXEL_ON);
}

void Matrix::setPixels(uint8_t x, uint8_t y, uint8_t value){
	
	buffer(x, y, value);
}

void Matrix::setPixels(uint8_t x, uint8_t y, Sprite sprite){
	
	for(uint8_t i = 0; i < sprite.width(); i++){
		for(uint8_t j = 0; j < sprite.height(); j++){
			buffer(x + i, y + j, sprite.read(i, j));
		}
	}
}

void Matrix::setPixels(ClockWord timeWord){
	
	for(uint8_t i = 0; i < timeWord.getLength(); i++){
		buffer(timeWord.getX() + i, timeWord.getY(), timeWord.getBitAt(i));
	}
}

void pulseClockPin(){

	digitalWrite(PIN_SHIFTREGISTER_CLOCK, HIGH);
	digitalWrite(PIN_SHIFTREGISTER_CLOCK, LOW);	
}