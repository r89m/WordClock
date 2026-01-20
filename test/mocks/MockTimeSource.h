#ifndef NODEMCU_FIXES_MOCKTIMESOURCE_H
#define NODEMCU_FIXES_MOCKTIMESOURCE_H

#include <gmock/gmock.h>

#include "WordClock.h"

struct MockTimeSource : TimeSource {
    MOCK_METHOD(int8_t, day, (), (override));
    MOCK_METHOD(int8_t, month, (), (override));

    MOCK_METHOD(int8_t, hours, (), (override));
    MOCK_METHOD(int8_t, minutes, (), (override));
    MOCK_METHOD(int8_t, seconds, (), (override));
};

#endif // NODEMCU_FIXES_MOCKTIMESOURCE_H
