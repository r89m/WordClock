/*
 * PushButton.cpp
 *
 * Created: 18/11/2014 19:33:23
 *  Author: Richard
 */ 

#include "PushButton.h"
#include "Bounce2.h"

PushButton::PushButton(uint8_t pin){
	
	init(pin, true);
}

PushButton::PushButton(uint8_t pin, boolean enable_pullup){
	
	init(pin, enable_pullup);
}

void PushButton::init(uint8_t pin, boolean enable_pullup){
	
	if(enable_pullup){
		pinMode(pin, INPUT_PULLUP);
	} else {
		pinMode(pin, INPUT);
	}
	
	bouncer = Bounce();
	bouncer.attach(pin);
	bouncer.interval(5);
}

boolean PushButton::_update_button_state(){
	
	// Update the button
	bouncer.update();
	// Return whether it is pressed or not
	return (bouncer.read() == HIGH);
}