#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "Sprite.h"
#include "ClockWord.h"

const Sprite NUMBERS[10] = {
	Sprite(5, 7,B01110,
				B10001,
				B10011,
				B10101,
				B11001,
				B10001,
				B01110),
										
	Sprite(5, 7,B00100,
				B01100,
				B00100,
				B00100,
				B00100,
				B00100,
				B01110),
										
	Sprite(5, 7,B01110,
				B10001,
				B00001,
				B00110,
				B01000,
				B10000,
				B11111),
										
	Sprite(5, 7,B01110,
				B10001,
				B00001,
				B01110,
				B00001,
				B10001,
				B01110),
										
	Sprite(5, 7,B00010,
				B00110,
				B01010,
				B10010,
				B11111,
				B00010,
				B00010),
										
	Sprite(5, 7,B11111,
				B10000,
				B11110,
				B00001,
				B00001,
				B00001,
				B11110),
										
	Sprite(5, 7,B00110,
				B01000,
				B10000,
				B11110,
				B10001,
				B10001,
				B01110),
										
	Sprite(5, 7,B11111,
				B00001,
				B00010,
				B00100,
				B01000,
				B01000,
				B01000),
										
	Sprite(5, 7,B01110,
				B10001,
				B10001,
				B01110,
				B10001,
				B10001,
				B01110),

	Sprite(5, 7,B01110,
				B10001,
				B10001,
				B01111,
				B00001,
				B00010,
				B01100)};
										
const Sprite SYMBOL_DEGREES = Sprite(2, 2,	B11,
											B11);
										
const ClockWord TIME_WORDS_HOURS[12] = {
	ClockWord(4, 6, 3),		// One
	ClockWord(1, 7, 3),		// Two
	ClockWord(6, 7, 5),		// Three
	ClockWord(2, 8, 4),		// Four
	ClockWord(9, 8, 4),		// Five
	ClockWord(1, 9, 3),		// Six
	ClockWord(7, 9, 5),		// Seven
	ClockWord(3, 10, 5),	// Eight
	ClockWord(10, 10, 4),	// Nine
	ClockWord(1, 11, 3),	// Ten
	ClockWord(6, 11, 6),	// Eleven
	ClockWord(3, 12, 6)		// Twelve
};

const ClockWord TIME_WORDS_MINUTES[6] = {
	ClockWord(9, 3, 4),		// Five
	ClockWord(2, 1, 3),		// Ten
	ClockWord(4, 2, 7),		// Quarter
	ClockWord(7, 1, 6),		// Twenty
	ClockWord(0, 0, 0),		// Twenty Five - Empty, but makes the array numbering better
	ClockWord(1, 3, 6),		// Thirty
};

#define MINUTE_INDEX_FIVE 0
#define MINUTE_INDEX_TWENTY 3
																				
const ClockWord TIME_WORD_ITIS = ClockWord(0, 0, 5, B11011);
const ClockWord TIME_WORD_MINS = ClockWord(4, 4, 7);
const ClockWord TIME_WORD_TO = ClockWord(2, 5, 2);
const ClockWord TIME_WORD_PAST = ClockWord(7, 5, 4);
const ClockWord TIME_WORD_OCLOCK = ClockWord(16, 13, 6);

#endif // SYMBOLS_H
