/*
 * TimeWord.cpp
 *
 * Created: 18/11/2014 00:44:22
 *  Author: Richard
 */ 

#include "ClockWord.h"

ClockWord::ClockWord(uint8_t x, uint8_t y, uint8_t length){
	
	init(x, y, length);
}

ClockWord::ClockWord(uint8_t x, uint8_t y, uint8_t length, uint16_t customMask){
	
	init(x, y, length);
	// Overwrite the pixel mask with a custom one
	_mask = customMask;
}

void ClockWord::init(uint8_t x, uint8_t y, uint8_t length){
	
	_x = x;
	_y = y;
	_length = length;
	_mask = (0x01 << length) - 1;	// Create a number length bits long, set to all 1s
}

/** Public API **/

uint8_t ClockWord::getBitAt(uint8_t position) const{
	
	return bitRead(_mask, position);
}

uint8_t ClockWord::getLength() const{
	
	return _length;
}

uint8_t ClockWord::getX() const{
	
	return _x;
}

uint8_t ClockWord::getY() const{
	
	return _y;
}