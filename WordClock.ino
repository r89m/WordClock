#include <Sprite.h>
#include <binary.h>
#include <CapacitiveSensor.h>
#include <PushButton.h>
#include <CapacitiveButton.h>
#include <Button.h>
#include <MenuSystem.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <Time.h>
#include <DS3232RTC.h>
#include <Wire.h>

#include "Symbols.h"
#include "Matrix.h"

#define MATRIX_WIDTH 14
#define MATRIX_HEIGHT 14

Matrix display = Matrix(MATRIX_WIDTH, MATRIX_HEIGHT);

// Button Variables
PushButton btnUp = PushButton(4);
PushButton btnMain = PushButton(5);
PushButton btnDown = PushButton(6);

CapacitiveButton btnCapUp = CapacitiveButton(2, 3);

#define MODE_DISPLAY "Display"
#define MODE_SETUP "Setup"

#define MODE_DISPLAY_WORD "WordView"
#define MODE_DISPLAY_DATE "DateView"
#define MODE_DISPLAY_SECONDS "SecondsView"
#define MODE_DISPLAY_TEMP "TempView"
#define MODE_SETUP_BRIGHTNESS_MIN "MinBrightness"
#define MODE_SETUP_BRIGHTNESS_MAX "MaxBrightness"

extern uint8_t ClockMode;

// Menu Variables
MenuSystem menuSystem;
Menu menuRoot("Root");

Menu menuDefault(MODE_DISPLAY);
MenuItem menuItemWordClock(MODE_DISPLAY_WORD);
MenuItem menuItemDateView(MODE_DISPLAY_DATE);
MenuItem menuItemSecondsView(MODE_DISPLAY_SECONDS);
MenuItem menuItemTempView(MODE_DISPLAY_TEMP);

Menu menuSetup(MODE_SETUP);
MenuItem menuItemSetupMinBrightness(MODE_SETUP_BRIGHTNESS_MIN);
MenuItem menuItemSetupMaxBrightness(MODE_SETUP_BRIGHTNESS_MAX);

void setup()
{
	// Setup the time library
	setSyncProvider(RTC.get);

	// If the time is not set correctly
	if (timeStatus() != timeSet){
		// Make the whole face flash
		for (uint8_t i = 0; i < MATRIX_WIDTH; i++){
			for (uint8_t j = 0; j < MATRIX_HEIGHT; j++){
				display.setPixels(i, j, 0x01);
			}
		}
	}

	// Setup the buttons
	btnMain.configureButton(configurePushButtons);
	btnMain.onHold(1000, onMainButtonHeld);
	btnMain.onRelease(0, 400, onMainButtonPressed);

	btnCapUp.setThreshold(500);
	btnCapUp.configureButton(configureCapacitiveButtons);

	// Setup menu structure
	menuRoot.add_menu(&menuDefault);
	menuRoot.add_menu(&menuSetup);

	menuDefault.add_item(&menuItemWordClock, &onMenuItemSelected);
	menuDefault.add_item(&menuItemDateView, &onMenuItemSelected);
	menuDefault.add_item(&menuItemSecondsView, &onMenuItemSelected);
	menuDefault.add_item(&menuItemTempView, &onMenuItemSelected);

	menuSetup.add_item(&menuItemSetupMinBrightness, &onMenuItemSelected);
	menuSetup.add_item(&menuItemSetupMaxBrightness, &onMenuItemSelected);

	menuSystem.set_root_menu(&menuRoot);
}

void loop()
{

	// Update the display; handle multiplexing etc.
	display.loop();
}

void mainButtonHold(Button&, uint16_t duration){

}

void onMainButtonPressed(Button&, uint16_t duration){

	menuSystem.next(true);
	menuSystem.select();
}

void onMainButtonHeld(Button&, uint16_t duration){

	// If we're currently displaying, go into the setup menu
	if (getCurrentMode() == MODE_DISPLAY){
		menuSetup.activate();
	}
	else if (getCurrentMode() == MODE_SETUP){
		// Save the currently set value
	}

}

void configurePushButtons(Bounce& btn){

	btn.interval(15);
}

void configureCapacitiveButtons(CapacitiveSensor& capSense){

	capSense.set_CS_AutocaL_Millis(10000);
}

void onMenuItemSelected(MenuItem* menuItem){



}

char* getCurrentView(){

	return menuSystem.get_current_menu()->get_name();
}

char* getCurrentMode(){

	return menuSystem.get_current_menu()->get_parent()->get_parent()->get_name();
}