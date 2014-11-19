/*
 * Button.h
 *
 * Created: 18/11/2014 19:33:02
 *  Author: Richard
 */ 


#ifndef BUTTON_H_
#define BUTTON_H_

#include <inttypes.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

class Button;
class ButtonReleaseCallback;
class ButtonHoldCallback;

// Define callback types
typedef void (*ButtonOnPressCallback)(Button*);
typedef void (*ButtonOnEventCallback)(Button*, uint16_t);

class Button{
	
	private:
		uint32_t _button_pressed_timestamp;
		boolean _is_pressed;
		void (*_on_press_callback)(Button*);
		void (*_on_release_callback)(Button*, uint16_t);
		ButtonReleaseCallback **_on_release_callbacks;
		ButtonHoldCallback **_on_hold_callbacks;
		uint8_t _on_release_callbacks_count = 0;
		uint8_t _on_hold_callbacks_count = 0;
		
	protected:
		virtual boolean _update_button_state()=0;
		void _button_pressed();
		void _button_released();
		void _button_held();
		uint16_t _button_time_elapsed();
		void _add_on_release_callback(ButtonReleaseCallback);
		void _add_on_hold_callback(ButtonHoldCallback);
	
	public:
		void onPress(ButtonOnPressCallback);
		void onRelease(ButtonOnEventCallback);
		void onRelease(uint16_t, ButtonOnEventCallback);
		void onRelease(uint16_t, uint16_t, ButtonOnEventCallback);
		void onHold(uint16_t, ButtonOnEventCallback);
		void onHold(uint16_t, uint16_t, ButtonOnEventCallback);
		void update();
		boolean isPressed();
};

class ButtonDelayedCallback{
	
	protected:
		uint16_t _delay;
		void (*_callback)(Button*,uint16_t);
		virtual boolean _should_perform_callback(uint16_t)=0;
		
	public:
		boolean performCallbackIfTime(Button*, uint16_t);
};

class ButtonReleaseCallback : public ButtonDelayedCallback{
	
	private:
		uint16_t _max_delay;
		boolean _should_perform_callback(uint16_t);
	
	public:
		ButtonReleaseCallback(uint16_t, ButtonOnEventCallback);
		ButtonReleaseCallback(uint16_t, uint16_t, ButtonOnEventCallback);
	
};

class ButtonHoldCallback : public ButtonDelayedCallback{
	
	private:
		uint16_t _repeat_delay = 0;
		uint32_t _next_callback_execution_delay;
		boolean _should_perform_callback(uint16_t);
		
	public:
		ButtonHoldCallback(uint16_t, ButtonOnEventCallback);
		ButtonHoldCallback(uint16_t, uint16_t, ButtonOnEventCallback);
		void reset();
	};

#endif /* BUTTON_H_ */