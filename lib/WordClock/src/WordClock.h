#ifndef NODEMCU_FIXES_WORDCLOCK_H
#define NODEMCU_FIXES_WORDCLOCK_H

#include "Matrix.h"

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

// Time constants
#define MS_IN_SECOND 1000
#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_2ANDAHALFMINUTES (SECONDS_IN_MINUTE * 2.5)
#define SECONDS_IN_5MINUTES (SECONDS_IN_MINUTE * 5)
#define MINUTES_IN_HOUR 60

// Define update frequencies in ms
#define FREQ_1HZ 1000
#define FREQ_3HZ 333
#define FREQ_15HZ 67

class TimeSource {
    public:
    virtual ~TimeSource() = default;

    virtual int8_t day() = 0;
    virtual int8_t month() = 0;

    virtual int8_t hours() = 0;
    virtual int8_t minutes() = 0;
    virtual int8_t seconds() = 0;
};

class WordClock {
    public:
    WordClock(TimeSource &timeSource, IMatrix &display) : timeSource(timeSource), display(display) {}

    void cycleMode();
    void setCurrentMode(uint8_t newMode);

    void displayWordClock();
    void displayLiveTime();
    void displaySeconds();
    void displayDate();
    void displayNightlight();

    bool shouldUpdateView();

    // TODO: Move to utility file
    uint8_t roundMinutesToNearestFive(uint8_t currentMinute, uint8_t currentSecond);

    void displayBuildDateSlash(IMatrix &display, uint8_t x, uint8_t y);

    void update();

    private:
    TimeSource &timeSource;
    IMatrix &display;

    // Default mode is display word
    uint8_t CurrentMode = MODE_DISPLAY_WORD;

    uint32_t lastViewUpdateTimestamp = 0;
};

#endif // NODEMCU_FIXES_WORDCLOCK_H
