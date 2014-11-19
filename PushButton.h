/*
 * PushButton.h
 *
 * Created: 18/11/2014 19:33:33
 *  Author: Richard
 */ 


#ifndef PUSHBUTTON_H_
#define PUSHBUTTON_H_

#include "Button.h";
#include "Bounce2.h"

class PushButton : public Button {
	
	private:
		Bounce bouncer;
		void init(uint8_t, boolean);
		
	protected:
		boolean _update_button_state();
	
	public:
		PushButton(uint8_t);
		PushButton(uint8_t, boolean);
	
};





#endif /* PUSHBUTTON_H_ */