#include <MPR121Button.h>
#include <Adafruit_MPR121.h>
#include <binary.h>

#include "Symbols.h"
#include "Matrix.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <WiFiUdp.h>

#include "NTP.h"

#define _PRINT(x) Serial.print(x)
#define _PRINTLN(x) Serial.println(x)

#define I2C_ADDR_TOUCH 0x5A

#define MATRIX_WIDTH 14
#define MATRIX_HEIGHT 14
#define MATRIX_BUFFER_SIZE 25 // Buffer size should be (height * width) / 8 rounded up to the nearest integer

// Define MODE constants - integers should be continuous
#define MODE_DISPLAY_WORD 1
#define MODE_DISPLAY_LIVETIME 2
#define MODE_DISPLAY_DATE 3
#define MODE_DISPLAY_SECONDS 4
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
MPR121Button mainButton = MPR121Button(TouchSensor, 6);

// Default mode is display word
uint8_t CurrentMode = MODE_DISPLAY_WORD;

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

// Touch Button Sensitivity
uint8_t TouchSensitivity_Touch;
uint8_t TouchSensitivity_Release;

uint32_t lastViewUpdateTimestamp = 0;

AsyncWebServer server(80);

// Forward declarations
uint8_t roundMinutesToNearestFive(uint8_t currentMinute, uint8_t currentSecond);
void displayBuildDateSlash(Matrix &display, uint8_t x, uint8_t y);
void displayBuildBrightnessBar(Matrix &display, uint8_t x, uint8_t y);
void setCurrentMode(uint8_t newMode);
void cycleMode();
void saveNewValue();
void setCurrentTimeUTC(uint32_t newUTCTime);
void alarmStartBuildUp();
void saveToEEPROM();

void displayWordClock() {
    uint8_t nowHour = ntp.hours();
    uint8_t nowMinute = ntp.minutes();
    uint8_t nowSecond = ntp.seconds();

    uint8_t roundedMinute = roundMinutesToNearestFive(nowMinute, nowSecond);
    boolean isAfterHalfPast = true;

    // Point the index to the next hour (this can be used for values after half past)
    uint8_t hourIndex = nowHour;
    uint8_t minuteIndex = roundedMinute;

    // If the time is half past or earlier, decrement the index value by one to point the current hour.
    if (roundedMinute <= 30) {
        isAfterHalfPast = false;
        hourIndex--;
    } else {
        // For times after half past, we need to take their value away from 60
        minuteIndex = MINUTES_IN_HOUR - roundedMinute;
    }

    if (hourIndex > 11) {
        hourIndex -= 12;
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
    if (roundedMinute == 0 || roundedMinute == 60) {
        display.setPixels(TIME_WORD_OCLOCK);
    } else {
        // Display Minute
        // 25 and 35 are special cases
        if (roundedMinute == 25 || roundedMinute == 35) {
            display.setPixels(TIME_WORDS_MINUTES[MINUTE_INDEX_TWENTY]);
            display.setPixels(TIME_WORDS_MINUTES[MINUTE_INDEX_FIVE]);
        } else {
            display.setPixels(TIME_WORDS_MINUTES[minuteIndex]);
        }

        // Don't display "minutes" for quarter past or quarter to
        if (minuteIndex != MINUTE_INDEX_QUARTER) {
            display.setPixels(TIME_WORD_MINS);
        }

        // To / Past
        if (isAfterHalfPast) {
            display.setPixels(TIME_WORD_TO);
        } else {
            display.setPixels(TIME_WORD_PAST);
        }
    }

    // Hour
    display.setPixels(TIME_WORDS_HOURS[hourIndex]);
}

void displayLiveTime() {

    uint8_t hr = ntp.hours();
    uint8_t hrSmall = hr % 10;
    uint8_t hrBig = (hr - hrSmall) / 10;

    uint8_t min = ntp.minutes();
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
    uint8_t secs = (ntp.seconds() / 5) + 1;
    for (uint8_t i = 0; i < secs; i++) {
        display.setPixels(1, 12 - i, PIXEL_ON);
        display.setPixels(12, 12 - i, PIXEL_ON);
    }
}

void displaySeconds() {

    uint8_t secs = ntp.seconds();
    uint8_t secsSmall = secs % 10;
    uint8_t secsBig = (secs - secsSmall) / 10;

    // Clear the display buffer
    display.clear();

    // Draw the numbers to the canvas
    display.setPixels(1, 3, NUMBERS[secsBig]);
    display.setPixels(8, 3, NUMBERS[secsSmall]);
}

void displayDate() {

    uint8_t dom = ntp.day();
    uint8_t domSmall = dom % 10;
    uint8_t domBig = (dom - domSmall) / 10;

    uint8_t mon = ntp.month();
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

void displayNightlight() { display.setAllPixels(PIXEL_ON); }

void onMainButtonPressed(Button &button, uint16_t duration) { cycleMode(); }

void cycleMode() {
    // Go to the next view
    switch (CurrentMode) {

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

    // For the nightlight and external frame driver, we go back to the word clock mode
    case MODE_NIGHTLIGHT:
    case MODE_EXTERNAL_FRAMES:
        setCurrentMode(MODE_DEFAULT);
        break;
    }
}

boolean shouldUpdateView() {

    // The frequency of the view being updated depends on which mode we are on:
    //	- date / time modes should be updated	3Hz
    //	- temp mode								1Hz

    uint32_t timeSinceLastUpdate = millis() - lastViewUpdateTimestamp;
    boolean updateView = false;

    switch (CurrentMode) {
    case MODE_DISPLAY_WORD:
    case MODE_DISPLAY_LIVETIME:
    case MODE_DISPLAY_DATE:
    case MODE_DISPLAY_SECONDS:
        updateView = (timeSinceLastUpdate > FREQ_3HZ);
        break;

    case MODE_NIGHTLIGHT:
        updateView = (lastViewUpdateTimestamp == 0); // Only return true once for this view
        break;

    case MODE_EXTERNAL_FRAMES:
        updateView = false; // All new frames come from the serial port
        break;
    }

    if (updateView) {
        // If we're updating the view, update the timestamp
        lastViewUpdateTimestamp = millis();
    }

    return updateView;
}

uint8_t roundMinutesToNearestFive(uint8_t currentMinute, uint8_t currentSecond) {

    // Convert minutes and seconds into seconds
    uint16_t seconds = (currentMinute * SECONDS_IN_MINUTE) + currentSecond + SECONDS_IN_2ANDAHALFMINUTES;

    return round(seconds / SECONDS_IN_5MINUTES) * 5; // Round to nearest 5
}

void setCurrentMode(uint8_t newMode) {

    _PRINT(F("New mode: "));
    _PRINTLN(newMode);

    // Save the new mode
    CurrentMode = newMode;

    // Reset the display counter
    lastViewUpdateTimestamp = 0;

    // Reset the display
    display.flashOff();
}

void displayBuildDateSlash(Matrix &display, uint8_t x, uint8_t y) {

    // Build the date slash - 6 pixels diagonally
    for (uint8_t i = 0; i < 6; i++) {
        display.setPixels(x + i, y + (5 - i), PIXEL_ON);
    }
}

void setup() {
    // This baud rate must match that of the HC-06 module
    Serial.begin(115200);

    Serial.println("Boot Start");

    // Connect to Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/plain", String(ntp.formattedTime("%A %T")).c_str()); });

    server.on("/mode-cycle", HTTP_POST, [](AsyncWebServerRequest *request) {
        cycleMode();
        request->send(200, "text/plain", "Mode cycled");
    });

    server.on("/mode-set", HTTP_POST, [](AsyncWebServerRequest *request) {
        const auto newModeStr = request->getParam("mode", true);
        const auto newMode = newModeStr->value().toInt();
        setCurrentMode(newMode);
        request->send(200, "text/plain", ("Mode set to: " + newModeStr->value()).c_str());
    });

    // Prepare the touch sensor
    if (!TouchSensor.begin(I2C_ADDR_TOUCH)) {
        Serial.println("Couldn't connect to touch sensor");
    }

    // Initialise the display
    display.init();

    mainButton.onRelease(0, 500, onMainButtonPressed);

    setCurrentMode(MODE_DEFAULT);

    _PRINTLN("Boot animation start");

    display.setBrightness(255);

    // Boot animation
    for (int col = 0; col < MATRIX_WIDTH; col++) {
        for (int row = 0; row < MATRIX_HEIGHT; row++) {
            display.setPixels(col, row, PIXEL_ON);
        }
        display.update();
        delay(100);
        for (int row = 0; row < MATRIX_HEIGHT; row++) {
            display.setPixels(col, row, PIXEL_OFF);
        }
        display.update();
    }

    display.setAllPixels(PIXEL_ON);
    display.update();
    delay(500);
    display.setAllPixels(PIXEL_OFF);
    display.update();

    // Start server
    server.begin();

    ntp.ruleSTD("DST", Last, Sun, Mar, 1, 60);
    ntp.ruleSTD("GMT", Last, Sun, Oct, 2, 0);

    ntp.begin();

    _PRINTLN("Boot complete");
}

void loop() {
    mainButton.update();

    ntp.update();

    // Check whether we should be updating the view
    if (shouldUpdateView()) {
        // If we should, update the display for the given mode
        switch (CurrentMode) {

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

        case MODE_NIGHTLIGHT:
            displayNightlight();
            break;
        }
    }

    // Update the display;
    display.update();
}
