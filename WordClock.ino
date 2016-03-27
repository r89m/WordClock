#include <Timezone.h>
#include <MPR121ProximityButton2.h>
#include <RunningAverage.h>
#include <HT1632.h>
#include <DS3232RTC.h>
#include <TimeAlarms.h>
#include <Adafruit_MPR121.h>
#include <Sprite.h>
#include <binary.h>
#include <PushButton.h>
#include <Button.h>
#include <ButtonEventCallback.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <Time.h>
#include <Wire.h>

#include "Symbols.h"
#include "Matrix.h"

#define _PRINT(x) Serial.print(x)
#define _PRINTLN(x) Serial.println(x)
//#define _PRINT(x)
//#define _PRINTLN(x)

#define I2C_ADDR_TOUCH 0x5A
#define PIN_LDR A0

#define MATRIX_WIDTH 14
#define MATRIX_HEIGHT 14
#define MATRIX_BUFFER_SIZE 25     // Buffer size should be (height * width) / 8 rounded up to the nearest integer

// Define MODE constants - integers should be continuous
#define MODE_DISPLAY_WORD 1
#define MODE_DISPLAY_LIVETIME 2
#define MODE_DISPLAY_DATE 3
#define MODE_DISPLAY_SECONDS 4
#define MODE_SETUP_BRIGHTNESS_MIN 5
#define MODE_SETUP_BRIGHTNESS_MAX 6
#define MODE_ALARM 7
#define MODE_NIGHTLIGHT 8
#define MODE_EXTERNAL_FRAMES 9

#define MODE_DEFAULT MODE_DISPLAY_WORD

// Define update frequencies in ms
#define FREQ_1HZ 1000
#define FREQ_3HZ 333
#define FREQ_15HZ 67

// EEPROM Addresses
#define ADDRESS_SCHEMA 0x00
#define ADDRESS_BRIGHTNESS_MIN 0x01
#define ADDRESS_BRIGHTNESS_MAX 0x02
#define ADDRESS_SENSITIVITY_TOUCH 0x03
#define ADDRESS_SENSITIVITY_RELEASE 0x04

// Time constants
#define MS_IN_SECOND 1000
#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_2ANDAHALFMINUTES (SECONDS_IN_MINUTE * 2.5)
#define SECONDS_IN_5MINUTES (SECONDS_IN_MINUTE * 5)
#define MINUTES_IN_HOUR 60

// Serial Commands
#define SERIAL_COMMAND_SETTIME 't'
#define SERIAL_COMMAND_SETMODE 'm'
#define SERIAL_COMMAND_SETALARM 'a'
#define SERIAL_COMMAND_DISMISSALARM 'd'
#define SERIAL_COMMAND_NIGHTLIGHTBRIGHTNESS 'n'
#define SERIAL_COMMAND_CAPACITIVETHRESHOLD 'c'
#define SERIAL_COMMAND_FRAME 'f'

// LDR Reading Limits
#define LDR_READING_VERYDARK 150
#define LDR_READING_VERYLIGHT 700

// Alarm variables
#define ALARM_BRIGHTNESS_RAMP_PERIOD_MINS 10 
#define ALARM_FLASH_ONSET_DELAY_MINS 3
#define ALARM_MAX_DURATION_MINS 5

#define MIN_TIMESTAMP 1357041600 // Jan 1 2013

// Schema Version & Defaults
const uint8_t EEPROMSchemaVersion = 0x01;
const uintmax_t EEPROMDefaultsBrightnessMin = 0;
const uint8_t EEPROMDefaultsBrightnessMax = 15;
const uint8_t EEPROMDefaultsSensitivityTouch = 40;
const uint8_t EEPROMDefaultsSensitivityRelease = 5;

Matrix display = Matrix(MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_BUFFER_SIZE);

// Button Variables
Adafruit_MPR121 TouchSensor = Adafruit_MPR121();
MPR121ProximityButton2 mainButton = MPR121ProximityButton2(TouchSensor, 6);

// Default mode is display word
uint8_t CurrentMode = MODE_DISPLAY_WORD;

// Store the time that the alarm will go off - default to disabled
uint32_t alarmTime = 0;

// Nightlight variables
uint8_t NightlightBrightness = 0;

// London, UK Timezone
TimeChangeRule londonDST = {"DST", Last, Sun, Mar, 1, 60};
TimeChangeRule londonStandard = {"GMT", Last, Sun, Oct, 2, 0};
Timezone London(londonDST, londonStandard);

// Brightness Settings
uint8_t ClockBrightness_Min;
uint8_t ClockBrightness_Max;
uint8_t ClockBrightness_Config;

// Touch Button Sensitivity
uint8_t TouchSensitivity_Touch;
uint8_t TouchSensitivity_Release;

// Settings timeout timer
#define CONFIG_TIMEOUT_SECS 15
const AlarmID_t configTimer = Alarm.timerRepeat(CONFIG_TIMEOUT_SECS, configTimedout);

// Serial buffer
const uint8_t serialBufferLength = (MATRIX_BUFFER_SIZE * 2) + 5;
uint8_t serialBufferIndex = 0;
char serialCommandChar = 0;
char serialBuffer[serialBufferLength];
boolean serialSendComplete = false;

// LDR Readings
RunningAverage ambientBrightness(25);
uint8_t brightnessMappedPrevious = -1;
uint32_t brightnessChangedLastTimestamp = 0;

uint32_t lastViewUpdateTimestamp = 0;

void setup()
{
  // This baud rate must match that of the HC-06 module
	Serial.begin(115200);
  
  Serial.println("Boot Start");
  
  // Prepare the touch sensor
  if(!TouchSensor.begin(I2C_ADDR_TOUCH)){
    Serial.println("Couldn't connect to touch sensor");
  }    
  
	// Initialise the display
	display.init();
	
	// Read the previous settings from EEPROM
	// If the stored schema matches the current schema, use the stored values
  // Disabled for now
	if (EEPROM.read(ADDRESS_SCHEMA) == EEPROMSchemaVersion){
		ClockBrightness_Min = EEPROM.read(ADDRESS_BRIGHTNESS_MIN);
		ClockBrightness_Max = EEPROM.read(ADDRESS_BRIGHTNESS_MAX);
		TouchSensitivity_Touch = EEPROM.read(ADDRESS_SENSITIVITY_TOUCH);
		TouchSensitivity_Release = EEPROM.read(ADDRESS_SENSITIVITY_RELEASE);
	}
	// Otherwise, save the current EEPROM version and then use the default values
	else {
		EEPROM.write(ADDRESS_SCHEMA, EEPROMSchemaVersion);
		ClockBrightness_Min = EEPROMDefaultsBrightnessMin;
		ClockBrightness_Max = EEPROMDefaultsBrightnessMax;
		TouchSensitivity_Touch = EEPROMDefaultsSensitivityTouch;
		TouchSensitivity_Release = EEPROMDefaultsSensitivityRelease;
	}
	
	_PRINTLN(TouchSensitivity_Touch);
	_PRINTLN(TouchSensitivity_Release);
  
	ClockBrightness_Min = 0;
	ClockBrightness_Max = 15;
	
	// Disable the config timeout
	cancelConfigTimeout();
	
	//btn.onRelease(onValueChangeButtonPress);
  mainButton.setThresholds(TouchSensitivity_Touch, TouchSensitivity_Release);
	mainButton.onRelease(0, 500, onMainButtonPressed);
	mainButton.onHold(1000, onMainButtonHeld);
	
	//btn2.onRelease(0, 500, onValueChangeButtonPress);
	//btn3.onRelease(0, 500, onValueChangeButtonPress);
	
	//btn2.onHoldRepeat(1000, 500, onValueChangeButtonHold);
	//btn3.onHoldRepeat(1000, 500, onValueChangeButtonHold);

	// Setup the time library
	setSyncProvider(getCurrentTimeFromRTC);
	
  setCurrentMode(MODE_DEFAULT);
  
  _PRINTLN("Boot animation start");
  
  // Boot animation
  for(int col=0; col<MATRIX_WIDTH; col++){
    for(int row=0; row<MATRIX_HEIGHT; row++){
      display.setPixels(col, row, PIXEL_ON);
    }
    display.update();
    delay(100);
    for(int row=0; row<MATRIX_HEIGHT; row++){
      display.setPixels(col, row, PIXEL_OFF);
    }
    display.update();
  }
  
  display.setAllPixels(PIXEL_ON);
  display.update();
  delay(500);
  display.setAllPixels(PIXEL_OFF);
  display.update();
  
  _PRINTLN("Boot complete");
}

void loop()
{
	// Update all the buttons -  do this at the beginning, as a pressed button may cause the CurrentMode to change
  mainButton.update();
	
	// Check if the alarm time has arrived
	Alarm.delay(0);
	
	// Check for anything from the serial port
	if(serialSendComplete){
    switch(serialCommandChar){
      
      case SERIAL_COMMAND_SETTIME:
        serialProcessSetTime();
        break;
        
      case SERIAL_COMMAND_SETMODE:
        serialProcessMode();
        break;
      
      case SERIAL_COMMAND_SETALARM:
        serialProcessAlarm();
        break;
        
      case SERIAL_COMMAND_DISMISSALARM:
        serialProcessDismissAlarm();
        break;
        
      case SERIAL_COMMAND_NIGHTLIGHTBRIGHTNESS:
        serialProcessNightlight();
        break;
        
      case SERIAL_COMMAND_CAPACITIVETHRESHOLD:
        serialProcessCapacitiveThreshold();
        break;
        
      case SERIAL_COMMAND_FRAME:
        serialProcessFrame();
        break;
        
      default:
        _PRINTLN("Unrecognised command.");
        break;
    }      
		
    // Reset serial buffer variables
    resetSerialCommand();
	}
	
	// If we are on a brightness setting screen, update the brightness to reflect the setting
	if (CurrentMode == MODE_SETUP_BRIGHTNESS_MIN || CurrentMode == MODE_SETUP_BRIGHTNESS_MAX){
		display.setBrightness(ClockBrightness_Config);
	} else if (CurrentMode == MODE_ALARM || CurrentMode == MODE_NIGHTLIGHT){
		// Display brightness is set within the display methods for these modes, so do nothing here.
	} else {
    // Record ambient light level and add it to the running average
    ambientBrightness.addValue(analogRead(PIN_LDR));
    
    // Only update the brightness if the mapped brightness has changed by more than +- 1 and more than a second has passed since the last update
    uint8_t brightnessMapped = map(ambientBrightness.getAverage(), LDR_READING_VERYDARK, LDR_READING_VERYLIGHT, ClockBrightness_Min, ClockBrightness_Max);
    
    if (brightnessMapped > brightnessMappedPrevious + 1 || brightnessMapped < brightnessMappedPrevious - 1){
      if(millis() - brightnessChangedLastTimestamp > 1000){
		    // Otherwise, update brightness to reflect LDR reading - mapped between it's readings for light / dark and the min /max brightness settings
		    display.setBrightness(brightnessMapped);
        brightnessMappedPrevious = brightnessMapped;
        brightnessChangedLastTimestamp = millis();
		    //display.setBrightness(15);
      }        
    }      
	}
	
	// Check whether we should be updating the view
	if(shouldUpdateView()){		
		// If we should, update the display for the given mode
		switch(CurrentMode){
		
			case MODE_DISPLAY_WORD:
				displayWordClock();
				break;
		
			case MODE_DISPLAY_LIVETIME:
				displayLiveTime();
				break;
				
			case MODE_DISPLAY_DATE:
				displayDate();
				break;
				
			case MODE_DISPLAY_SECONDS:
				displaySeconds();
				break;
				
			case MODE_ALARM:
				displayAlarm();
				break;
				
			case MODE_NIGHTLIGHT:
				displayNightlight();
				break;
				
			case MODE_SETUP_BRIGHTNESS_MAX:
			case MODE_SETUP_BRIGHTNESS_MIN:
				displayBrightnessSettings();
				break;
		}
	}

	// Update the display;
	display.update();
}

void displayWordClock(){
	
  time_t timeNow = London.toLocal(now());
  
	uint8_t nowHour = hourFormat12(timeNow); // Get the current hour, 0-11
	uint8_t nowMinute = minute(timeNow);
	uint8_t nowSecond = second(timeNow);
	
	uint8_t roundedMinute = roundMinutesToNearestFive(nowMinute, nowSecond);
	boolean isAfterHalfPast = true;
	
	// Point the index to the next hour (this can be used for values after half past)
	uint8_t hourIndex = nowHour;
	uint8_t minuteIndex = roundedMinute;
	
	// If the time is half past or earlier, decrement the index value by one to point the current hour.
	if(roundedMinute <= 30){
		isAfterHalfPast = false;
    hourIndex--;
  }     
	else {
		// For times after half past, we need to take their value away from 60
		minuteIndex = MINUTES_IN_HOUR - roundedMinute;
	}
  
  if(hourIndex > 11){
    hourIndex = 0;
  }
	
	// Map the minute index to the array indices
	minuteIndex = (minuteIndex / MINUTE_INDEX_MAX - 1);
	
	// Clear the canvas
	display.clear();
	
	/* =========================================
	   ======= Light the correct words =========
	   ========================================= */
	
	// It is
	display.setPixels(TIME_WORD_ITIS);
  
	// Minute
	if(roundedMinute == 0 || roundedMinute == 60){
		display.setPixels(TIME_WORD_OCLOCK);
		}
	else {
		// Display Minute
		// 25 and 35 are special cases
		if(roundedMinute == 25 || roundedMinute == 35){
			display.setPixels(TIME_WORDS_MINUTES[MINUTE_INDEX_TWENTY]);
			display.setPixels(TIME_WORDS_MINUTES[MINUTE_INDEX_FIVE]);
		} else {
			display.setPixels(TIME_WORDS_MINUTES[minuteIndex]);
		}
		
		// Don't display "minutes" for quarter past or quarter to
		if(minuteIndex != MINUTE_INDEX_QUARTER){
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

void displayLiveTime(){
  
  time_t timeNow = London.toLocal(now());
	
	uint8_t hr = hour(timeNow);
	uint8_t hrSmall = hr % 10;
	uint8_t hrBig = (hr - hrSmall) / 10;
	
	uint8_t min = minute(timeNow);
	uint8_t minSmall = min % 10;
	uint8_t minBig = (min - minSmall) / 10;
	
	// Clear the display buffer
	display.clear();
	
	// Draw the time to the canvas
	display.setPixels(3, 1, NUMBERS_SMALL[hrBig]);
	display.setPixels(8, 1, NUMBERS_SMALL[hrSmall]);
	
	display.setPixels(3, 8, NUMBERS_SMALL[minBig]);
	display.setPixels(8, 8, NUMBERS_SMALL[minSmall]);
	
	// Display seconds
	uint8_t secs = (second(timeNow) / 5) + 1;
	for(uint8_t i = 0; i < secs; i++){
		display.setPixels(1, 12 - i, PIXEL_ON);
		display.setPixels(12, 12 - i, PIXEL_ON);
	}	
}

void displaySeconds(){
  
  time_t timeNow = London.toLocal(now());
	
	uint8_t secs = second(timeNow);
	uint8_t secsSmall = secs % 10;
	uint8_t secsBig = (secs - secsSmall) / 10;
	
	// Clear the display buffer
	display.clear();
	
	// Draw the numbers to the canvas
	display.setPixels(1, 3, NUMBERS[secsBig]);
	display.setPixels(8, 3, NUMBERS[secsSmall]);
}

void displayDate(){
  
  time_t timeNow = London.toLocal(now());
	
	uint8_t dom = day(timeNow);
	uint8_t domSmall = dom % 10;
	uint8_t domBig = (dom - domSmall) / 10;
	
	uint8_t mon = month(timeNow);
	uint8_t monSmall = mon % 10;
	uint8_t monBig = (mon - monSmall) / 10;
	
	// Clear the display buffer
	display.clear();
	
	// Draw the date to the canvas
	display.setPixels(0, 0, NUMBERS_SMALL[domBig]);
	display.setPixels(4, 0, NUMBERS_SMALL[domSmall]);
	
	displayBuildDateSlash(display, 4, 4);
	
	display.setPixels(7, 9, NUMBERS_SMALL[monBig]);
	display.setPixels(11, 9, NUMBERS_SMALL[monSmall]);
}

void displayBrightnessSettings(){
	
	display.clear();
	
	// Draw the brightness display bar
	displayBuildBrightnessBar(display, 1, 1);
	
	// Draw the rest of the UI, depending on which brightness setting is being changed
	if(CurrentMode == MODE_SETUP_BRIGHTNESS_MAX){
		// Draw the arrow
		display.setPixels(8, 1, BRIGHTNESS_ARROW_UP);
	} else {
		// Draw the arrow
		display.setPixels(8, 5, BRIGHTNESS_ARROW_DOWN);
	}

	// Draw the brightness indicator
	display.setPixels(4, 12 - (ClockBrightness_Config), PIXEL_ON);
	display.setPixels(5, 12 - (ClockBrightness_Config), PIXEL_ON);
}

void displayAlarm(){
	
	// Do something to make the alarm mode a bit more interesting
	uint32_t currentSeconds = hour() * SECS_PER_HOUR + minute() * SECONDS_IN_MINUTE + second();
	int16_t alarmDelta = alarmTime - currentSeconds;
	
	// Build up the brightness of the display as we get closer to the alarm time
	display.setBrightness(map(max(0, alarmDelta), 0, ALARM_BRIGHTNESS_RAMP_PERIOD_MINS * SECONDS_IN_MINUTE, 15, ClockBrightness_Min));
	
	if(alarmDelta < ALARM_MAX_DURATION_MINS * SECONDS_IN_MINUTE * -1){
		// Cancel the alarm if it has been going on for too long
		setCurrentMode(MODE_DISPLAY_WORD);
	} else if(alarmDelta < ALARM_FLASH_ONSET_DELAY_MINS * SECONDS_IN_MINUTE * -1){
		// If the alarm is still going after ALARM_FLASH_ONSET_DELAY_MINS
		// Every two seconds, switch between the word clock and a fully white display
		if(alarmDelta % 2){
			displayWordClock();
		} else {
			display.setAllPixels(PIXEL_ON);
		}
	}
}

void displayNightlight(){
	
	display.setAllPixels(PIXEL_ON);
	display.setBrightness(NightlightBrightness);
}

void onMainButtonPressed(Button& button, uint16_t duration){

	cycleMode();
}

void onMainButtonHeld(Button& button, uint16_t duration){
  
  // Disable this for now
  return;

	// If we're currently in a setup mode, save the values and then go to the default display mode
	if (CurrentMode == MODE_SETUP_BRIGHTNESS_MAX || CurrentMode == MODE_SETUP_BRIGHTNESS_MIN){
		// Save the current value
		saveNewValue();
		// Go to the default display mode
		setCurrentMode(MODE_DEFAULT);
	}
	else{
		// Go to max brightness setup
		setCurrentMode(MODE_SETUP_BRIGHTNESS_MAX);
	}
}

void cycleMode(){
	// Go to the next view
	switch(CurrentMode){
	
		// For most of the modes, we can just increment the mode
		case MODE_DISPLAY_WORD:
		case MODE_DISPLAY_LIVETIME:
		case MODE_DISPLAY_DATE:
			setCurrentMode(CurrentMode + 1);
			break;
	
		// For seconds, we loop back to the beginning
		case MODE_DISPLAY_SECONDS:
			setCurrentMode(MODE_DEFAULT);
			break;
	
		// For the alarm nightlight and external frame driver, we go back to the word clock mode
		case MODE_ALARM:
		case MODE_NIGHTLIGHT:
    case MODE_EXTERNAL_FRAMES:
			setCurrentMode(MODE_DEFAULT);
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

void onValueChangeButtonPress(Button& btn, uint16_t duration){
	
  // Disable for now
  return;
  
	// Only listen to the up / down buttons if we're on a settings screen
	if(CurrentMode == MODE_SETUP_BRIGHTNESS_MAX || CurrentMode == MODE_SETUP_BRIGHTNESS_MIN){
		// Check whether the up or down button was pressed by comparing the addresses of the Button parameter
		//changeBrightnessConfig(btn.is(btn2));	
	}
}

void onValueChangeButtonHold(Button& btn, uint16_t duration, uint16_t repeat_count){
	
	// Disable for now
	return;
	
	// Only listen to the up / down buttons if we're on a settings screen
	if(CurrentMode == MODE_SETUP_BRIGHTNESS_MAX || CurrentMode == MODE_SETUP_BRIGHTNESS_MIN){
		//changeBrightnessConfig(btn.is(btn2));
	}
}

void startResetConfigTimeout(){
	
	Alarm.enable(configTimer);
}

void cancelConfigTimeout(){
	
	Alarm.disable(configTimer);
}

void configTimedout(){
	
	setCurrentMode(MODE_DEFAULT);
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
	
	// Constrain the value of ClockBrightness to 2 - 9 first, so that decrementing it won't make it rollover
	ClockBrightness_Config = constrain(ClockBrightness_Config, 2, 9);
	
	// If we want to increment the value...
	if(increment){
		ClockBrightness_Config ++;
	} else {
		// Or, if we're decrementing...
		ClockBrightness_Config --;
	}
	
	startResetConfigTimeout();
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
		case MODE_DISPLAY_LIVETIME:
		case MODE_DISPLAY_DATE:
		case MODE_DISPLAY_SECONDS:
		case MODE_ALARM:
			updateView = (timeSinceLastUpdate > FREQ_3HZ);
			break;
			
		case MODE_SETUP_BRIGHTNESS_MAX:
		case MODE_SETUP_BRIGHTNESS_MIN:
			updateView = (timeSinceLastUpdate > FREQ_15HZ);
			break;	
			
		case MODE_NIGHTLIGHT:
			updateView = (lastViewUpdateTimestamp == 0);	// Only return true once for this view
			break;	
      
    case MODE_EXTERNAL_FRAMES:
      updateView = false;             // All new frames come from the serial port
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
	uint16_t seconds = (currentMinute * SECONDS_IN_MINUTE) + currentSecond + SECONDS_IN_2ANDAHALFMINUTES;
	
  return round(seconds / SECONDS_IN_5MINUTES) * 5;	// Round to nearest 5
}

void setCurrentMode(uint8_t newMode){
	
  _PRINT(F("New mode: "));
  _PRINTLN(newMode);
  
	// Save the new mode
	CurrentMode = newMode;
	
	// Reset the display counter
	lastViewUpdateTimestamp = 0;
	
	// Reset the display
	display.flashOff();
	
	// For the setup screens, set the config value
	if(newMode == MODE_SETUP_BRIGHTNESS_MAX){
		ClockBrightness_Config = ClockBrightness_Max;
		startResetConfigTimeout();
	} else if (newMode == MODE_SETUP_BRIGHTNESS_MIN){
		ClockBrightness_Config = ClockBrightness_Min;
		startResetConfigTimeout();
	} else {
		cancelConfigTimeout();
	}
}

void serialProcessSetTime() {
	
	// Time given must be in UTC
	uint32_t newTime = String(serialBuffer).toInt();
	
	if( newTime >= MIN_TIMESTAMP) { // check the integer is a valid time (greater than Jan 1 2013)
		setCurrentTimeUTC(newTime); // Sets the RTC and Arduino times to the time given
    _PRINT(F("Time set to: "));
    _PRINTLN(newTime);
	}
}

void serialProcessMode(){
	
  // Check if the first character is a 'c'
	if(serialBuffer[0] == 'c'){
		// Cycle the mode
		cycleMode();
	} else {
		// Set the Mode to the given integer
		setCurrentMode(String(serialBuffer).toInt());
	}
}

void serialProcessAlarm(){
	
	// Set the alarm mode to the given time
	alarmTime = String(serialBuffer).toInt();
  
  _PRINT(F("Set alarm time: "));
  _PRINTLN(alarmTime);
  
	// Take away the alarm build up time to set the alarm start time
	Alarm.alarmOnce(alarmTime - ALARM_BRIGHTNESS_RAMP_PERIOD_MINS * SECONDS_IN_MINUTE, alarmStartBuildUp);
}

void serialProcessDismissAlarm(){
	
  _PRINTLN(F("Dismiss alarm"));
	setCurrentMode(MODE_DISPLAY_WORD);
}

void serialProcessNightlight(){
	
	NightlightBrightness = String(serialBuffer).toInt();
  _PRINT(F("Nightlight brightness: "));
  _PRINTLN(NightlightBrightness);
	setCurrentMode(MODE_NIGHTLIGHT);
}

void serialProcessCapacitiveThreshold(){
	
  // TODO: Needs fixing
  
	// Data comes in the format [0-9]:[0-9] where [0-9] is any two digit integer with leading zeros and : separates the two values
	uint8_t touchThreshold = (String(serialBuffer[0]) + String(serialBuffer[1])).toInt();
	// Remove the colon from the stream
	uint8_t releaseTheshold = (String(serialBuffer[3]) + String(serialBuffer[4])).toInt();
	
	_PRINT(F("New touch threshold: "));
	_PRINTLN(touchThreshold);
	_PRINT(F("New release threshold: "));
	_PRINTLN(releaseTheshold);
	
  // Save the new values to EEPROM
  
  EEPROM.write(ADDRESS_SENSITIVITY_TOUCH, touchThreshold);
  EEPROM.write(ADDRESS_SENSITIVITY_RELEASE, releaseTheshold);
  
  // Set the sensitivity of the touch button  
  mainButton.setThresholds(touchThreshold, releaseTheshold);
  
  _PRINTLN("New sensitivity set.");
}

void serialProcessFrame(){
  
  uint8_t newFrame[MATRIX_BUFFER_SIZE];
  
  uint8_t nullCharFound = false;
  
  for(int i = 0; i < MATRIX_BUFFER_SIZE; i++){
    uint8_t charValue = 0;
    char serialChar[2] = {0, 0};
    
    if(!nullCharFound){
      // Process the incoming serial stream two characters at a time
      uint8_t serialCharIndex = i * 2;
      serialChar[0] = serialBuffer[serialCharIndex];
      serialChar[1] = serialBuffer[serialCharIndex + 1];
    
      if(serialChar[0] == 0 || serialChar[1] == 0){
        nullCharFound = true;
      } else {
        charValue = (uint8_t) strtoul(serialChar, NULL, 16);
      }        
    }
  
    newFrame[i] = charValue;
  }  
  
  setCurrentMode(MODE_EXTERNAL_FRAMES);
  
  display.overwriteBuffer(newFrame);  
}

//TODO: Change LDR limits via Serial

void alarmStartBuildUp(){
	
	// Switch to alarm mode
	setCurrentMode(MODE_ALARM);
}

time_t getCurrentTimeFromRTC(){
	
	// Get the current time from the RTC and then add the DST offset
  _PRINTLN("Getting time fromRTC...");
  return RTC.get();
}

void setCurrentTimeUTC(uint32_t newUTCTime){
	
	RTC.set(newUTCTime); // Set the time on the RTC - UTC
	setTime(newUTCTime);
}

void displayBuildBrightnessBar(Matrix &display, uint8_t x, uint8_t y){
	
	// Build the brightness bar - 3 x 12
	
	display.setPixels(x,	 y, PIXEL_ON);
	display.setPixels(x + 1, y, PIXEL_ON);
	display.setPixels(x + 2, y, PIXEL_ON);
	
	for(uint8_t i = 1; i <= 10; i++){
		y++;
		display.setPixels(x + 1, y, PIXEL_ON);
	}
	
	// Increment y to draw the bottom line
	y++;

	display.setPixels(x,	 y, PIXEL_ON);
	display.setPixels(x + 1, y, PIXEL_ON);
	display.setPixels(x + 2, y, PIXEL_ON);
}

void displayBuildDateSlash(Matrix &display, uint8_t x, uint8_t y){
	
	// Build the date slash - 6 pixels diagonally
	for(uint8_t i = 0; i < 6; i++){
		display.setPixels(x + i, y + (5 - i), PIXEL_ON);
	}
}

void resetSerialCommand(){
  
  serialSendComplete = false;
  serialCommandChar = 0;
}

void serialEvent(){
  
  while(Serial.available()){
    char nextChar = Serial.read();
    
    // If a new line has been received or the buffer is full
    if(nextChar == '\n' || serialBufferIndex == (serialBufferLength - 1)){
      serialBuffer[serialBufferIndex] = 0;  // Terminate the string
      serialBufferIndex = 0;                // Reset the index counter
      serialSendComplete = true;            // Indicate that the command has been sent
      break;
    } else if(serialCommandChar == 0){
      serialCommandChar = nextChar;
    } else {
      serialBuffer[serialBufferIndex++] = nextChar;
    }
  }
}