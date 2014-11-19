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

#include "Matrix.h"

Matrix::Matrix(uint8_t width, uint8_t height){
	
	_width = width;
	_height = height;
	_buffer = (uint8_t*)calloc(_width, _height);
}


void Matrix::buffer(uint8_t x, uint8_t y, uint8_t value){
	
	// Silently fail if buffer has not been initialised
	if(!_buffer) return;
	
	// Check whether the x and y values are valid
	if(x >= _width) return;
	if(y >= _height) return;
	
	// Record value in buffer
	if(value){
		_buffer[x] |= 0x01 << y;
		}else{
		_buffer[x] &= ~(0x01 << y);
	}
}


/*** User API ***/

void Matrix::clear(){
	
	for(uint8_t i = 0; i < _width; i++){
		for(uint8_t j = 0; j < _height; j++){
			buffer(i, j, 0x00);
		}
	}
	
}

void Matrix::loop(){
	
	// Check whether multiplexing needs to be moved along one column
	if(millis() - _multiplexLastTimestamp > _multiplexPeriod){
		// Record the current time
		_multiplexLastTimestamp = millis();
		
		// Move on to the next column
		_multiplexCurrentColumn++;
		
		// Wrap around if we've reached the end
		if(_multiplexCurrentColumn == _width){
			_multiplexCurrentColumn = 0;
		}
		
	}
}

void Matrix::setBrightness(uint8_t brightness){
	
}

void Matrix::flashOn(){
	
	_flash_enabled = true;
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

