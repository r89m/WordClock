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

#include "Matrix.h"
#include <Time.h>
#include <Timezone.h>

uint32_t lastLoopTimestamp = 0;
uint32_t serialOutputLastTime = 0;
#if defined(OUTPUT_TO_SERIAL)
  uint32_t serialOutputLastTime = 0;
#else
  #include <HT1632.h>
  HT1632 MatrixDriver = HT1632(2, 3, 4);
#endif

Matrix::Matrix(uint8_t width, uint8_t height, uint16_t bufferSize){
	
	_width = width;
	_height = height;
  _bufferSize = bufferSize;  
	_buffer = (MATRIX_BUFFER_DATA_TYPE*)calloc(bufferSize, MATRIX_BUFFER_DATA_TYPE_SIZE);
}

void Matrix::init(){   
  #if !defined(OUTPUT_TO_SERIAL)
    MatrixDriver.begin(HT1632_COMMON_16NMOS);
  #endif
}


void Matrix::buffer(uint8_t x, uint8_t y, uint8_t value){
	
	// Silently fail if buffer has not been initialised
	if(!_buffer) return;
	
	// Check whether the x and y values are valid
	if(x >= _width) return;
	if(y >= _height) return;
	
	uint8_t index;
  uint8_t bitPos;
  
  getPixelIndexAndBit(x, y, index, bitPos);
	
	// Record value in buffer
  if(value == PIXEL_ON){
    // Set the correct bits to be true
	  _buffer[index] |= (0x01 << bitPos);
  } else {
    // Set the correct bits to be false
    _buffer[index] &= ~(0x01 << bitPos);
  }    
  
  // Record that data has changed
  bufferHasNewData = true;
}

void Matrix::getPixelIndexAndBit(uint8_t x, uint8_t y, uint8_t &index, uint8_t &bitPos){
  
  uint16_t position = (y * _width) + x;
  index = (position / MATRIX_BUFFER_DATA_TYPE_SIZE_BITS);
  bitPos = (position - (index * MATRIX_BUFFER_DATA_TYPE_SIZE_BITS));
}


/*** User API ***/

void Matrix::clear(){
	
	setAllPixels(PIXEL_OFF);
}

void Matrix::update(){
	
	#if defined(OUTPUT_TO_SERIAL)
  
		// Check whether the display needs to be sent to the serial port (do it once per second)
		if(millis() - serialOutputLastTime > 1000){
      
      /*
      // London, UK Timezone
      TimeChangeRule londonDST = {"DST", Last, Sun, Mar, 1, 60};
      TimeChangeRule londonStandard = {"GMT", Last, Sun, Oct, 2, 0};
      Timezone London(londonDST, londonStandard);
			
			// Record the current time
			serialOutputLastTime = millis();
      
	     time_t timeNow = London.toLocal(now());      
      
      Serial.print(day(timeNow));
      Serial.print("/");
      Serial.print(month(timeNow));
      Serial.print("/");
      Serial.print(year(timeNow));
      Serial.print(" - ");
      Serial.print(hour(timeNow));
      Serial.print(":");
      Serial.print(minute(timeNow));
      Serial.print(":");
      Serial.print(second(timeNow));
      Serial.println();
      */
      
		  /*
			Serial.println("==============");
			// Draw the current view
			for(uint8_t row = 0; row < _height; row++){
				Serial.print("|");
				for(uint8_t col = 0; col < _width; col++){
					Serial.print(isPixelOn(col, row) ? 'X' : ' ');	
				}
				Serial.println('|');
			}
			Serial.println("==============");		
      */
      
		}
	#else
  
    enum DrawStatus {DoNothing, Redraw, ClearDisplay};
      
    DrawStatus displayDrawStatus = DoNothing;     
      
    // Check whether there is new data to draw, and if so set the display to redraw
    if(bufferHasNewData){
        displayDrawStatus = Redraw;
    }
  
    // Check whether or not flashing is enabled - this can override whether or not to redraw the display
    
    if(_flash_enabled){
      uint32_t timeNow = millis();
      uint32_t timeSinceLastFlash = timeNow - _flash_last_timestamp;
      
      // Handle switching between flash states
      // If the display is currently lit and the time for the display to be on has passed
      if(_flash_current_status == FLASH_LIT && timeSinceLastFlash > _flash_on_duration){ 
        // Record the new display status
        _flash_current_status = FLASH_UNLIT;
        // Record the time now
        _flash_last_timestamp = timeNow;
      }
      // If the display is currently unlit and the time for the display not to be on has passed
      else if (_flash_current_status == FLASH_UNLIT && timeSinceLastFlash > _flash_off_duration){
        // Ensure that the screen is updated
        displayDrawStatus = Redraw;
        // Record the new display status
        _flash_current_status = FLASH_LIT;
        // Record the time now
        _flash_last_timestamp = timeNow;
        Serial.println("yutuyt");
      }
      
      if(_flash_current_status == FLASH_UNLIT){
        // Clear the display
        displayDrawStatus = ClearDisplay;        
      }
    }
    
    
    // Check whether there's new data in the buffer - ie, we need to send new data to the driver
		if(displayDrawStatus == Redraw){
      for(uint8_t row = 0; row < _height; row++){
        for(uint8_t col = 0; col < _width; col++){
          // Should really be: (row * 16) + col, but I messed up the wiring
          uint8_t pixel = ((13 - row) * 16) + col;
          
          // More fixes my dodgy wiring!
          if(col == 12){
            pixel -= 11;
          } else if (col == 13){
            pixel -= 13;          
           // pixel += 2;  // Move the first and second columns back to the beginning
          } else {
           pixel += 2;
          }
          
          if(isPixelOn(col, row)){
            MatrixDriver.setPixel(pixel);
          } else {
            MatrixDriver.clrPixel(pixel);
          }
        }
      }
      
      // Output the new display
      MatrixDriver.writeScreen();
      
      if(displayHasNewBrightness){
        // Set the brightness
        MatrixDriver.setBrightness(_brightness);
        displayHasNewBrightness = false;
      }        
      
      // Reset the fact that we have new data
      bufferHasNewData = false;
    } else if (displayDrawStatus == ClearDisplay){
      MatrixDriver.clearScreen();
    }      
	#endif
}

void Matrix::setBrightness(uint8_t brightness){
	
  // Limit brightness to be between 0 and 15
  brightness = constrain(brightness, 0, 15);
  
  // Only carry on if the brightness has actually changed
  if(_brightness != brightness){
	  _brightness = brightness;
    displayHasNewBrightness = true;
  }    
}

void Matrix::flashOn(){
	
	_flash_enabled = true;
  _flash_current_status = FLASH_LIT;
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
	
  uint8_t index;
  uint8_t bitPos;
  
  getPixelIndexAndBit(x, y, index, bitPos);
  
	return bitRead(_buffer[index], bitPos);
}

boolean Matrix::isPixelOn(uint8_t x, uint8_t y){
	
	return (getPixel(x, y) == PIXEL_ON);
}

void Matrix::setAllPixels(uint8_t pixelValue){
	
	for(uint8_t col = 0; col < _width; col++){
		for(uint8_t row = 0; row < _height; row++){
			setPixels(col, row, pixelValue);
		}
	}
}

void Matrix::setPixels(uint8_t x, uint8_t y, uint8_t value){
	
	buffer(x, y, value);
}

void Matrix::setPixels(uint8_t x, uint8_t y, Sprite sprite){
	
	for(uint8_t sprite_x = 0; sprite_x < sprite.width(); sprite_x++){
		for(uint8_t sprite_y = 0; sprite_y < sprite.height(); sprite_y++){
			setPixels(x + sprite_x, y + sprite_y, sprite.read(sprite_x, sprite_y));
		}
	}
}

void Matrix::setPixels(ClockWord timeWord){
	
	for(uint8_t word_x = 0; word_x < timeWord.getLength(); word_x++){
		setPixels(timeWord.getX() + word_x, timeWord.getY(), timeWord.getBitAt(word_x));
	}
}

void Matrix::overwriteBuffer(uint8_t *newBuffer){
  
  for(uint16_t i = 0; i < _bufferSize; i++){
    _buffer[i] = newBuffer[i];
  }
  bufferHasNewData = true;
}