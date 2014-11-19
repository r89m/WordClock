/*
 * TimeWord.h
 *
 * Created: 18/11/2014 00:44:13
 *  Author: Richard
 */ 


#ifndef TIMEWORD_H_
#define TIMEWORD_H_

#include <inttypes.h>

class TimeWord{
	
	private:
		uint8_t _x;
		uint8_t _y;
		uint8_t _length;
		uint16_t _mask;
		void init(uint8_t, uint8_t, uint8_t);
		void setCustomMask(uint16_t);
		
	public:
		TimeWord(uint8_t, uint8_t, uint8_t);	
		TimeWord(uint8_t, uint8_t, uint8_t, uint16_t);
		uint8_t getLength() const;
		uint8_t getBitAt(uint8_t) const;
		uint8_t getX() const;
		uint8_t getY() const;
};



#endif /* TIMEWORD_H_ */