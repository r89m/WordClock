/*
 * Button.cpp
 *
 * Created: 18/11/2014 19:33:09
 *  Author: Richard
 */ 

#include "Button.h"

void Button::_button_pressed(){
	
	// Set the button pressed state to true
	_is_pressed = true;
	
	// Record when the button was originally pressed
	_button_pressed_timestamp = millis();
	
	// Fire the onPress callback if one has been specified
	if(_on_press_callback){
		_on_press_callback(*this);
	}
	
	// Reset all hold callbacks
	for(uint8_t i = 0; i < _on_hold_callbacks_count; i++){
		_on_hold_callbacks[i]->reset();
	}
}

void Button::_button_released(){

	// Set the button pressed state to false
	_is_pressed = false;
	
	// Fire the onRelease callback if one has been specified
	if(_on_release_callback){
		_on_release_callback(*this, _button_time_elapsed());
	}
	
	// Search for the most appropriate onRelease callback with wait
	// Iterate from the end of the array, as we want the first callback who's "wait" has elapsed
	for(uint8_t i = _on_release_callbacks_count - 1; i >= 0; i--){
		if(_on_release_callbacks[i]->performCallbackIfTime(*this, _button_time_elapsed())){
			break;
		}
	}
}

void Button::_button_held(){
	
	// Search for the most appropriate onHold callback
	for(uint8_t i = 0; i < _on_hold_callbacks_count; i++){
		_on_hold_callbacks[i]->performCallbackIfTime(*this, _button_time_elapsed());
	}
}

uint16_t Button::_button_time_elapsed(){
	
	return millis() - _button_pressed_timestamp;
}


void Button::update(){
	
	// Record the previous and new state of the button
	boolean _previous_button_state = isPressed();
	boolean _new_button_state = _update_button_state();
	
	// If the state of the button has changed
	if(_previous_button_state != _new_button_state){
		// If the button is now pressed
		if(_new_button_state){
			_button_pressed();
		} else {
			// Otherwise if it has just been let go
			_button_released();
		}
	// If the state hasn't changed but the button is pressed - ie it is being held
	} else if(_new_button_state){
		_button_held();
	}
	
	
}

void Button::onPress(ButtonOnPressCallback callback){
	
	_on_press_callback = callback;
}

void Button::onRelease(ButtonOnEventCallback callback){
	
	_on_release_callback = callback;
}

void Button::onRelease(uint16_t wait, ButtonOnEventCallback callback){
	
	_add_on_release_callback(ButtonReleaseCallback(wait, callback));
}

void Button::onRelease(uint16_t wait, uint16_t max_wait, ButtonOnEventCallback callback){
	
	_add_on_release_callback(ButtonReleaseCallback(wait, max_wait, callback));
}

void Button::_add_on_release_callback(ButtonReleaseCallback callbackObj){
	
	// Resize onRelease callback list, keeping existing items.
	// If it fails, there the callback is not added and the function returns.
	_on_release_callbacks = (ButtonReleaseCallback **) realloc(_on_release_callbacks,
								(_on_release_callbacks_count + 1)
								* sizeof(ButtonReleaseCallback*));
	
	if (_on_release_callbacks == NULL){
		return;
	}

	_on_release_callbacks[_on_release_callbacks_count] = & callbackObj;

	_on_release_callbacks_count++;
}

void Button::onHold(uint16_t duration, ButtonOnEventCallback callback){
	
	_add_on_hold_callback(ButtonHoldCallback(duration, callback));
}

void Button::onHold(uint16_t duration, uint16_t repeat_every, ButtonOnEventCallback callback){
	
	_add_on_hold_callback(ButtonHoldCallback(duration, repeat_every, callback));
}

void Button::_add_on_hold_callback(ButtonHoldCallback callbackObj){
		
	// Resize onHold callback list, keeping existing items.
	// If it fails, there the callback is not added and the function returns.
	_on_hold_callbacks = (ButtonHoldCallback **) realloc(_on_hold_callbacks,
	(_on_hold_callbacks_count + 1)
	* sizeof(ButtonHoldCallback*));
	
	if (_on_hold_callbacks == NULL){
		return;
	}

	_on_hold_callbacks[_on_hold_callbacks_count] = & callbackObj;

	_on_hold_callbacks_count++;
}

boolean Button::isPressed(){
	
	return _is_pressed;
}

boolean ButtonDelayedCallback::performCallbackIfTime(Button& button, uint16_t elapsed_time){
	
	if(_should_perform_callback(elapsed_time)){
		_callback(button, elapsed_time);
		return true;
	} else {
		return false;
	}
}

ButtonReleaseCallback::ButtonReleaseCallback(uint16_t delay, ButtonOnEventCallback callback){
	
	_delay = delay;
	_callback = callback;
}

ButtonReleaseCallback::ButtonReleaseCallback(uint16_t delay, uint16_t _max_delay, ButtonOnEventCallback callback){
	
	_delay = delay;
	_max_delay = _max_delay;
	_callback = callback;
}

boolean ButtonReleaseCallback::_should_perform_callback(uint16_t elapsed_time){
	
	return ((elapsed_time >= _delay) && (elapsed_time <= _max_delay));
}

ButtonHoldCallback::ButtonHoldCallback(uint16_t duration, ButtonOnEventCallback callback){
	
	_delay = duration;
	_next_callback_execution_delay = duration;
	_callback = callback;
}

ButtonHoldCallback::ButtonHoldCallback(uint16_t duration, uint16_t _repeat_delay, ButtonOnEventCallback callback){
	
	_delay = duration;
	_repeat_delay = _repeat_delay;
	_callback = callback;
	
	reset();
}

boolean ButtonHoldCallback::_should_perform_callback(uint16_t elapsed_time){
	
	if(elapsed_time >= _next_callback_execution_delay){
		_next_callback_execution_delay += _repeat_delay;
		return true;
	} else {
		return false;
	}
}

void ButtonHoldCallback::reset(){
	
	_next_callback_execution_delay = _delay;
}