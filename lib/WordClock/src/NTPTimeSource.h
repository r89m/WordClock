#ifndef NODEMCU_FIXES_NTPTIMESOURCE_H
#define NODEMCU_FIXES_NTPTIMESOURCE_H

#include "NTP.h"
#include "WordClock.h"

class NTPTimeSource : public TimeSource {
    public:
    NTPTimeSource(NTP &ntp) : ntp(ntp) {}

    int8_t day() override { return ntp.day(); }
    int8_t month() override { return ntp.minutes(); }
    int8_t hours() override { return ntp.hours(); }
    int8_t minutes() override { return ntp.minutes(); }
    int8_t seconds() override { return ntp.seconds(); }

    private:
    NTP &ntp;
};

#endif // NODEMCU_FIXES_NTPTIMESOURCE_H
