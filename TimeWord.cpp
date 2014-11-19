/*
 * TimeWord.cpp
 *
 * Created: 18/11/2014 00:44:22
 *  Author: Richard
 */ 

#include "TimeWord.h"

TimeWord::TimeWord(uint8_t x, uint8_t y, uint8_t length){
	
	init(x, y, length);
}

TimeWord::TimeWord(uint8_t x, uint8_t y, uint8_t length, uint16_t customMask){
	
	init(x, y, length);
	setCustomMask(customMask);
}

void TimeWord::init(uint8_t x, uint8_t y, uint8_t length){
	
	_x = x;
	_y = y;
	_length = length;
	_mask = (0x01 << (length - 1));
}

void TimeWord::setCustomMask(uint16_t mask){
	
	_mask = mask;
}

/** Public API **/

uint8_t TimeWord::getBitAt(uint8_t position) const{
	
	return _mask & (0x01 << position);
}

uint8_t TimeWord::getLength() const{
	
	return _length;
}

uint8_t TimeWord::getX() const{
	
	return _x;
}

uint8_t TimeWord::getY() const{
	
	return _y;
}