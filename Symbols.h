#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "Sprite.h"
#include "ClockWord.h"

const Sprite TIME_INTRO = Sprite(5, 1,		B11011);
const Sprite NUMBER_NOUGHT = Sprite(5, 7,		B01110,
										B10001,
										B10011,
										B10101,
										B11001,
										B10001,
										B01110);
										
const Sprite NUMBER_ONE = Sprite(5, 7,		B00100,
										B01100,
										B00100,
										B00100,
										B00100,
										B00100,
										B01110);
										
const Sprite NUMBER_TWO = Sprite(5, 7,		B01110,
										B10001,
										B00001,
										B00110,
										B01000,
										B10000,
										B11111);
										
const Sprite NUMBER_THREE = Sprite(5, 7,		B01110,
										B10001,
										B00001,
										B01110,
										B00001,
										B10001,
										B01110);
										
const Sprite NUMBER_FOUR = Sprite(5, 7,		B00010,
										B00110,
										B01010,
										B10010,
										B11111,
										B00010,
										B00010);
										
const Sprite NUMBER_FIVE = Sprite(5, 7,		B11111,
										B10000,
										B11110,
										B00001,
										B00001,
										B00001,
										B11110);
										
const Sprite NUMBER_SIX = Sprite(5, 7,		B00110,
										B01000,
										B10000,
										B11110,
										B10001,
										B10001,
										B01110);
										
const Sprite NUMBER_SEVEN = Sprite(5, 7,		B11111,
										B00001,
										B00010,
										B00100,
										B01000,
										B01000,
										B01000);
										
const Sprite NUMBER_EIGHT = Sprite(5, 7,		B01110,
										B10001,
										B10001,
										B01110,
										B10001,
										B10001,
										B01110);

const Sprite NUMBER_NINE = Sprite(5, 7,		B01110,
										B10001,
										B10001,
										B01111,
										B00001,
										B00010,
										B01100);
										
const Sprite SYMBOL_DEGREES = Sprite(2, 2,	B11,
										B11);
										
const ClockWord TIME_WORD_ITIS = ClockWord(0, 0, 5, B11011);
const ClockWord TIME_WORD_MINUTE_ONE = ClockWord(0, 5, 3);

#endif // SYMBOLS_H
