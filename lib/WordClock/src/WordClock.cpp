#include "WordClock.h"

#include "Symbols.h"

void WordClock::cycleMode() {
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

void WordClock::setCurrentMode(uint8_t newMode) {

    // Save the new mode
    CurrentMode = newMode;

    // Reset the display counter
    lastViewUpdateTimestamp = 0;

    // Reset the display
    display.flashOff();
}

void WordClock::displayWordClock() {
    uint8_t nowHour = timeSource.hours();
    uint8_t nowMinute = timeSource.minutes();
    uint8_t nowSecond = timeSource.seconds();

    uint8_t roundedMinute = roundMinutesToNearestFive(nowMinute, nowSecond);
    boolean isAfterHalfPast = true;

    // Point the index to the next hour (this can be used for values after half past)
    auto hourIndex = 12 + nowHour; // Offset by 12 (hours per AM / PM)
    uint8_t minuteIndex = roundedMinute;

    // If the time is half past or earlier, decrement the index value by one to point the current hour.
    if (roundedMinute <= 30) {
        isAfterHalfPast = false;
        hourIndex--;
    } else {
        // For times after half past, we need to take their value away from 60
        minuteIndex = MINUTES_IN_HOUR - roundedMinute;
    }

    hourIndex = hourIndex % 12;

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

void WordClock::displayLiveTime() {

    uint8_t hr = timeSource.hours();
    uint8_t hrSmall = hr % 10;
    uint8_t hrBig = (hr - hrSmall) / 10;

    uint8_t min = timeSource.minutes();
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
    uint8_t secs = (timeSource.seconds() / 5);
    for (uint8_t i = 0; i < secs; i++) {
        display.setPixels(1, 12 - i, PIXEL_ON);
        display.setPixels(12, 12 - i, PIXEL_ON);
    }
}

void WordClock::displaySeconds() {

    uint8_t secs = timeSource.seconds();
    uint8_t secsSmall = secs % 10;
    uint8_t secsBig = (secs - secsSmall) / 10;

    // Clear the display buffer
    display.clear();

    // Draw the numbers to the canvas
    display.setPixels(1, 3, NUMBERS[secsBig]);
    display.setPixels(8, 3, NUMBERS[secsSmall]);
}

void WordClock::displayDate() {

    uint8_t dom = timeSource.day();
    uint8_t domSmall = dom % 10;
    uint8_t domBig = (dom - domSmall) / 10;

    uint8_t mon = timeSource.month();
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

void WordClock::displayNightlight() { display.setAllPixels(PIXEL_ON); }
boolean WordClock::shouldUpdateView() {

    // The frequency of the view being updated depends on which mode we are on:
    //	- date / time modes should be updated	3Hz
    //	- temp mode								1Hz

    // return true; // TODO: Temp fix

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

uint8_t WordClock::roundMinutesToNearestFive(uint8_t currentMinute, uint8_t currentSecond) {

    // Convert minutes and seconds into seconds
    uint16_t seconds = (currentMinute * SECONDS_IN_MINUTE) + currentSecond + SECONDS_IN_2ANDAHALFMINUTES;

    return round(seconds / SECONDS_IN_5MINUTES) * 5; // Round to nearest 5
}
void WordClock::displayBuildDateSlash(IMatrix &display, uint8_t x, uint8_t y) {

    // Build the date slash - 6 pixels diagonally
    for (uint8_t i = 0; i < 6; i++) {
        display.setPixels(x + i, y + (5 - i), PIXEL_ON);
    }
}
void WordClock::update() {
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
}