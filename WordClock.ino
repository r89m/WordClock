#include <TimeAlarms.h>
#include <Adafruit_MPR121.h>
#include <MPR121Button.h>
#include <Tlc5940.h>
#include <MemoryFree.h>
#include <Sprite.h>
#include <binary.h>
#include <CapacitiveSensor.h>
#include <PushButton.h>
#include <CapacitiveButton.h>
#include <Button.h>
#include <ButtonEventCallback.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <Time.h>
#include <DS3232RTC.h>
#include <Wire.h>

#include "Symbols.h"
#include "Matrix.h"

#define PIN_TOUCH_IRQ 12
#define I2C_ADDR_TOUCH 0x5C
#define PIN_LDR A0

#define MATRIX_WIDTH 14
#define MATRIX_HEIGHT 14

// Define MODE constants - integers should be continuous
#define MODE_DISPLAY_WORD 1
#define MODE_DISPLAY_DAYOFMONTH 2
#define MODE_DISPLAY_SECONDS 3
#define MODE_DISPLAY_TEMP 4
#define MODE_SETUP_BRIGHTNESS_MIN 5
#define MODE_SETUP_BRIGHTNESS_MAX 6
#define MODE_ALARM 7

// Define update frequencies in ms
#define FREQ_1HZ 1000
#define FREQ_3HZ 333
#define FREQ_15HZ 67

// EEPROM Addresses
#define ADDRESS_SCHEMA 0x00
#define ADDRESS_BRIGHTNESS_MIN 0x01
#define ADDRESS_BRIGHTNESS_MAX 0x02

#define BRIGHTNESS_DELTA 25;

// Time constants
#define MS_IN_SECOND 1000
#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_5MINUTES (SECONDS_IN_MINUTE * 5)
#define MINUTES_IN_HOUR 60

// Serial Commands
#define SERIAL_COMMAND_SETTIME 't'
#define SERIAL_COMMAND_SETMODE 'm'
#define SERIAL_COMMAND_SETALARM 'a'

// LDR Reading Limits
#define LDR_READING_VERYDARK 300
#define LDR_READING_VERYLIGHT 800

// Alarm variables
#define ALARM_BRIGHTNESS_RAMP_PERIOD_MINS 10 

#define MIN_TIMESTAMP 1357041600 // Jan 1 2013

// Schema Version & Defaults
const uint8_t EEPROMSchemaVersion = 0x01;
const uintmax_t EEPROMDefaultsBrightnessMin = 0x00;
const uint8_t EEPROMDefaultsBrightnessMax = 0xFF;

Matrix display = Matrix(MATRIX_WIDTH, MATRIX_HEIGHT);

// Button Variables
//PushButton btnUp = PushButton(2, ENABLE_INTERNAL_PULLUP);
//PushButton btnMain = PushButton(4, ENABLE_INTERNAL_PULLUP);
//PushButton btnDown = PushButton(5, ENABLE_INTERNAL_PULLUP);

//CapacitiveButton btnCapUp = CapacitiveButton(2, 3);

Adafruit_MPR121 TouchSensor = Adafruit_MPR121();

MPR121Button btn = MPR121Button(TouchSensor, 2);
MPR121Button btn1 = MPR121Button(TouchSensor, 4);
MPR121Button btn2 = MPR121Button(TouchSensor, 6);

// Default mode is display word
uint8_t CurrentMode = MODE_DISPLAY_WORD;

// Brightness Settings
uint8_t ClockBrightness_Min;
uint8_t ClockBrightness_Max;
uint8_t ClockBrightness_Config;

uint32_t lastViewUpdateTimestamp = 0;

void setup()
{
	Tlc.init();
	
	Serial.begin(115200);
	
	Serial.print("Memory free at boot start = ");
	Serial.println(freeMemory());

	// Read the previous settings from EEPROM
	// If the stored schema matches the current schema, use the stored values
	if (EEPROM.read(ADDRESS_SCHEMA) == EEPROMSchemaVersion){
		ClockBrightness_Min = EEPROM.read(ADDRESS_BRIGHTNESS_MIN);
		ClockBrightness_Max = EEPROM.read(ADDRESS_BRIGHTNESS_MAX);
	}
	// Otherwise, save the current EEPROM version and then use the default values
	else {
		EEPROM.write(ADDRESS_SCHEMA, EEPROMSchemaVersion);
		ClockBrightness_Min = EEPROMDefaultsBrightnessMin;
		ClockBrightness_Max = EEPROMDefaultsBrightnessMax;
	}
	
	btn.onPress(onBtnPress);
	//btn.onRelease(onValueChangeButtonPress);
	btn.onRelease(400, onValueChangeButtonPress);

	// Setup the time library
	setSyncProvider(RTC.get);

	// If the time is not set correctly
	if (timeStatus() != timeSet){
		// Make the whole face flash for 5 seconds, then boot as normal
		// Set all pixels to be on
		for (uint8_t i = 0; i < MATRIX_WIDTH; i++){
			for (uint8_t j = 0; j < MATRIX_HEIGHT; j++){
				display.setPixels(i, j, PIXEL_ON);
			}
		}
		// Max display flash
		display.flashOn(750);
		
		uint32_t startTime = millis();
		
		display.setBrightness(250);
		
		// Loop for 5 seconds
		while(millis() - startTime < 1){		// Change to 5000
			// Update the display
			display.loop();
		}
	}
	
	// Setup the buttons
	/*
	btnMain.configureButton(configurePushButtons);
	btnMain.onHold(1000, onMainButtonHeld);
	btnMain.onRelease(0, 400, onValueChangeButtonPress);
	Serial.println("hello");

	btnUp.onRelease(0, 500, onValueChangeButtonPress);
	btnUp.onHoldRepeat(750, 250, onValueChangeButtonHold);
	btnDown.onRelease(0, 500, onValueChangeButtonPress);
	btnDown.onHoldRepeat(750, 250, onValueChangeButtonHold);
	Serial.println("bye");
	*/
	
	//btnCapUp.setThreshold(500);
	//btnCapUp.configureButton(configureCapacitiveButtons);
	//btnCapUp.onHoldRepeat(1000, 250, onCapHeldRepeat);
	
	
	Serial.println("why");
	delay(500);
	

	//Serial.println("SC");
	
	Serial.print("Memory free at boot end = ");
	Serial.println(freeMemory());
	Serial.println("===========================================");
	
	// Set the default time
	setTime(1423397887);
	setCurrentMode(MODE_DISPLAY_SECONDS);
}

void loop()
{
	// Update all the buttons -  do this at the beginning, as a pressed button may cause the CurrentMode to change
	//btnMain.update();
	//btnUp.update();
	//btnDown.update();
	
	// If there is new data from the touch sensor
	if(digitalRead(PIN_TOUCH_IRQ) == LOW){
		uint16_t latestTouchReadings = TouchSensor.touched();
		btn.update(latestTouchReadings);
		btn1.update(latestTouchReadings);
		btn2.update(latestTouchReadings);
	}
	
	// Check for anything from the serial port
	if(Serial.available()){
		byte command = Serial.read();
		
		if(command == SERIAL_COMMAND_SETTIME){
			serialProcessSetTime();
		} else if (command == SERIAL_COMMAND_SETMODE){
			serialProcessMode();
		} else if (command == SERIAL_COMMAND_SETALARM){
			serialProcessAlarm();
		}
	}
	
	// If we are on a brightness setting screen, update the brightness to reflect the setting
	if (CurrentMode == MODE_SETUP_BRIGHTNESS_MIN || CurrentMode == MODE_SETUP_BRIGHTNESS_MAX){
		display.setBrightness(ClockBrightness_Config);
	} else if (CurrentMode == MODE_ALARM){
		// If we're in alarm mode, set the brightness based on the alarm brightness ramp period
		
	} else {
		// Otherwise, update brightness to reflect LDR reading - mapped between it's readings for light / dark and the min /max brightness settings
		display.setBrightness(map(analogRead(PIN_LDR), LDR_READING_VERYDARK, LDR_READING_VERYLIGHT, ClockBrightness_Min, ClockBrightness_Max));
		display.setBrightness(150);
	}
	
	// Check whether we should be updating the view
	if(shouldUpdateView()){		
		// If we should, update the display for the given mode
		switch(CurrentMode){
		
			case MODE_DISPLAY_WORD:
				displayWordClock();
				break;
		
			case MODE_DISPLAY_SECONDS:
				displaySeconds();
				break;
				
			case MODE_DISPLAY_DAYOFMONTH:
				displayDayOfMonth();
				break;
				
			case MODE_DISPLAY_TEMP:
				displayTemperature();
				break;
				
			case MODE_ALARM:
				displayWordClock();
				displayOverlayAlarm();
				break;
				
			case MODE_SETUP_BRIGHTNESS_MAX:
			case MODE_SETUP_BRIGHTNESS_MIN:
				displayBrightnessSettings();
				break;
		}
	}

	// Update the display; handle multiplexing etc.
	display.loop();
}

void displayWordClock(){
	
	uint8_t nowHour = hour();
	uint8_t nowMinute = minute();
	uint8_t nowSecond = second();
	
	uint8_t roundedMinute = roundMinutesToNearestFive(nowMinute, nowSecond);
	boolean isAfterHalfPast = true;
	
	// Point the index to the next hour (this can be used for values after half past)
	uint8_t hourIndex = nowHour;
	uint8_t minuteIndex = roundedMinute;
	
	// If the time is half past or earlier, decrement the index value by one to point the current hour.
	if(roundedMinute <= 30){
		isAfterHalfPast = false;
		hourIndex--;
		} else {
		// For times after half past, we need to take their value away from 60
		minuteIndex = MINUTES_IN_HOUR - roundedMinute;
	}
	
	// Map the minute index to the array indices
	minuteIndex = (minuteIndex / 5) - 1;
	
	// Clear the canvas
	display.clear();
	
	// Light the correct words
	// It is
	display.setPixels(TIME_WORD_ITIS);
	
	// Minute
	if(roundedMinute == 0){
		display.setPixels(TIME_WORD_OCLOCK);
		} else {
		// Display Minute
		// 25 is a special case
		if(roundedMinute == 25){
			display.setPixels(TIME_WORDS_MINUTES[MINUTE_INDEX_TWENTY]);
			display.setPixels(TIME_WORDS_MINUTES[MINUTE_INDEX_FIVE]);
		} else {
			display.setPixels(TIME_WORDS_MINUTES[minuteIndex]);
		}
		
		// Don't display "minutes" for quarter past
		if(roundedMinute != 15){
			display.setPixels(TIME_WORD_MINS);
		}
		
		// To / Past
		if(isAfterHalfPast){
			display.setPixels(TIME_WORD_TO);
			} else {
			display.setPixels(TIME_WORD_PAST);
		}
	}
	
	// Hour
	display.setPixels(TIME_WORDS_HOURS[hourIndex]);	
}

void displaySeconds(){
	
	uint8_t secs = second();
	uint8_t secsSmall = secs % 10;
	uint8_t secsBig = (secs - secsSmall) / 10;
	
	// Clear the display buffer
	display.clear();
	
	// Draw the numbers to the canvas
	display.setPixels(1, 3, NUMBERS[secsBig]);
	display.setPixels(8, 3, NUMBERS[secsSmall]);
}

void displayDayOfMonth(){
	
	uint8_t dom = day();
	uint8_t domSmall = dom % 10;
	uint8_t domBig = (dom - domSmall) / 10;
	
	// Clear the display buffer
	display.clear();
	
	// Draw the numbers to the canvas
	display.setPixels(1, 3, NUMBERS[domBig]);
	display.setPixels(8, 3, NUMBERS[domSmall]);
}

void displayTemperature(){
	
	// Don't bother handling negatives - hopefully it'll never get that cold!
	uint8_t rawTemp = RTC.temperature();
	uint8_t temp = max(0, rawTemp / 4);	// temperature returns the temp in C * 4
	uint8_t tempSmall = temp % 10;
	uint8_t tempBig = (temp - tempSmall) / 10;
	
	// Clear the display buffer
	display.clear();
	
	// Draw the numbers and symbol to the buffer
	display.setPixels(1, 3, NUMBERS[tempSmall]);
	display.setPixels(7, 3, NUMBERS[tempBig]);
	display.setPixels(12, 3, SYMBOL_DEGREES);
}

void displayBrightnessSettings(){
	
}

void displayOverlayAlarm(){
	
	// Do something to make the alarm mode a bit more interesting
	
}

void onMainButtonPressed(Button& button, uint16_t duration){

	// Go to the next view
	switch(CurrentMode){
		
		// For most of the modes, we can just increment the mode
		case MODE_DISPLAY_WORD:
		case MODE_DISPLAY_SECONDS:
		case MODE_DISPLAY_DAYOFMONTH:
			setCurrentMode(CurrentMode + 1);
			break;
			
		// For the temperature, we loop back to the beginning
		case MODE_DISPLAY_TEMP:
			setCurrentMode(MODE_DISPLAY_WORD);
			break;
		
		// For the alarm, we go back to the word clock mode
		case MODE_ALARM:
			setCurrentMode(MODE_DISPLAY_WORD);
			break;
			
		// For brightness settings, loop between the two
		case MODE_SETUP_BRIGHTNESS_MAX:
			saveNewValue();
			setCurrentMode(MODE_SETUP_BRIGHTNESS_MIN);
			break;
			
		case MODE_SETUP_BRIGHTNESS_MIN:
			saveNewValue();
			setCurrentMode(MODE_SETUP_BRIGHTNESS_MAX);
			break;
	}

}

void onMainButtonHeld(Button& button, uint16_t duration){

	// If we're currently in a setup mode, save the values and then go to the default display mode
	if (CurrentMode == MODE_SETUP_BRIGHTNESS_MAX || CurrentMode == MODE_SETUP_BRIGHTNESS_MIN){
		// Save the current value
		saveNewValue();
		
		// Go to the default display mode
		CurrentMode = MODE_DISPLAY_WORD;
	}
	else{
		// Go to max brightness setup
		CurrentMode = MODE_SETUP_BRIGHTNESS_MAX;
	}
}

void configurePushButtons(Bounce& btn){

	btn.interval(15);
}

void onBtnPress(Button& btn){
	
}

void onValueChangeButtonPress(Button& btn, uint16_t duration){
	
	// Only listen to the up / down buttons if we're on a settings screen
	if(CurrentMode == MODE_SETUP_BRIGHTNESS_MAX || CurrentMode == MODE_SETUP_BRIGHTNESS_MIN){
		// Check whether the up or down button was pressed by comparing the addresses of the Button parameter
		//changeBrightnessConfig(&btn == &btnUp);	
	}
}

void onValueChangeButtonHold(Button& btn, uint16_t duration, uint16_t repeat_count){
	
	// Only listen to the up / down buttons if we're on a settings screen
	if(CurrentMode == MODE_SETUP_BRIGHTNESS_MAX || CurrentMode == MODE_SETUP_BRIGHTNESS_MIN){
		//changeBrightnessConfig(&btn == &btnUp);
	}
}

void saveNewValue(){
	
	// See which value we were setting, and store it in the relevant variable
	switch(CurrentMode){
		
		case MODE_SETUP_BRIGHTNESS_MAX:
			ClockBrightness_Max = ClockBrightness_Config;
			break;
			
		case MODE_SETUP_BRIGHTNESS_MIN:
			ClockBrightness_Min = ClockBrightness_Config;
			break;
	}
	
	// Save the variables to EEPROM so that the settings persist
	saveToEEPROM();	
}

void saveToEEPROM(){
	
	EEPROM.write(ADDRESS_BRIGHTNESS_MAX, ClockBrightness_Max);
	EEPROM.write(ADDRESS_BRIGHTNESS_MIN, ClockBrightness_Min);
}

void changeBrightnessConfig(boolean increment){
	
	// If we want to increment the value and it won't become too large
	if(increment && ClockBrightness_Config < 230){
		ClockBrightness_Config += BRIGHTNESS_DELTA;
	} else if (!increment && ClockBrightness_Config > 25){
		// Or, if we're decrementing and it'll stay above 0
		ClockBrightness_Config -= BRIGHTNESS_DELTA;
	}
}

boolean shouldUpdateView(){
	
	// The frequency of the view being updated depends on which mode we are on:
	//	- date / time modes should be updated	3Hz
	//	- config modes							15Hz
	//	- temp mode								1Hz
	
	uint32_t timeSinceLastUpdate = millis() - lastViewUpdateTimestamp;
	boolean updateView = false;
	
	switch(CurrentMode){
		
		case MODE_DISPLAY_WORD:
		case MODE_DISPLAY_SECONDS:
		case MODE_DISPLAY_DAYOFMONTH:
		case MODE_ALARM:
			updateView = (timeSinceLastUpdate > FREQ_3HZ);
			break;
			
		case MODE_DISPLAY_TEMP:
			updateView = (timeSinceLastUpdate > FREQ_1HZ);
			break;
			
		case MODE_SETUP_BRIGHTNESS_MAX:
		case MODE_SETUP_BRIGHTNESS_MIN:
			updateView = (timeSinceLastUpdate > FREQ_15HZ);
			break;		
	}
	
	if(updateView){
		// If we're updating the view, update the timestamp
		lastViewUpdateTimestamp = millis();		
	}
	
	return updateView;
}

uint8_t roundMinutesToNearestFive(uint8_t currentMinute, uint8_t currentSecond){
	
	// Convert minutes and seconds into seconds
	uint16_t seconds = (currentMinute * SECONDS_IN_MINUTE) + currentSecond;
	
	return round(seconds / SECONDS_IN_5MINUTES) * 5;	// Round to nearest 5
}

void setCurrentMode(uint8_t newMode){
	
	// Save the new mode
	CurrentMode = newMode;
	// Reset the display counter
	lastViewUpdateTimestamp = 0;
}

void serialProcessSetTime() {
	
	uint32_t newTime = Serial.parseInt();
	
	if( newTime >= MIN_TIMESTAMP) { // check the integer is a valid time (greater than Jan 1 2013)
		setTime(newTime); // Sync Arduino clock to the time received on the serial port
	}
}

void serialProcessMode(){

	// Set the Mode to the given integer
	setCurrentMode(Serial.parseInt());
}

void serialProcessAlarm(){
	
	// Set the alarm mode to the given parameters (time / days of week)
	uint16_t alarmTime = Serial.parseInt();	
	alarmTime -= ALARM_BRIGHTNESS_RAMP_PERIOD_MINS * SECONDS_IN_MINUTE * MS_IN_SECOND;
	Alarm.alarmOnce(alarmTime, alarm);
}

void alarm(){
	
	// Do some stuff!
}