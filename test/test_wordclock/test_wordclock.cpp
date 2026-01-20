#include <gmock/gmock.h>

#include <cmath>
#include <format>

#include <Arduino.h>

#include <WordClock.h>
#include "PrintMatrix.h"

#include "../mocks/MockTimeSource.h"

#include <filesystem>
#include <fstream>
#include <string>

class fdd : public MockTimeSource {};

using ::testing::NiceMock;
using ::testing::Return;

using namespace fakeit;

namespace fs = std::filesystem;

class ConsolePrinter : public Printer {
    public:
    void print(const char character) override { _output(character); }
    void print(const char *output) override { _output(output); };
    void println(const char *output) override {
        print(output);
        _output("\n");
    };

    const char *bufferData() const { return buffer.c_str(); }

    size_t bufferSize() const { return buffer.size(); }

    void clearBuffer() { buffer = ""; }

    private:
    void _output(const char character) {
        printf("%c", character);
        buffer += character;
    }
    void _output(const char *output) {
        printf("%s", output);
        buffer += output;
    }
    std::string buffer;
};

struct TestDisplayTimes {
    std::optional<uint8_t> month;
    std::optional<uint8_t> day;
    std::optional<uint8_t> hours;
    std::optional<uint8_t> minutes;
    std::optional<uint8_t> seconds;

    static std::string ParamInfoToString(const testing::TestParamInfo<TestDisplayTimes> &info) { return ParamToString(info.param); }
    static std::string ParamToString(const TestDisplayTimes &param) {
        std::string text = "";
        if (param.month.has_value() && param.day.has_value()) {
            text = std::format("{:02}_{:02}", param.month.value(), param.day.value());
        }
        if (param.hours.has_value() && param.minutes.has_value() && param.seconds.has_value()) {
            if (!text.empty()) {
                text += "T";
            }
            text += std::format("{:02}_{:02}_{:02}", param.hours.value_or(0), param.minutes.value_or(0), param.seconds.value_or(0));
        }
        // Do something
        return text;
    }
};

class TestModeDisplayFixture : public testing::TestWithParam<TestDisplayTimes> {
    public:
    void doTheTest(const int displayMode, const std::string displayDirName, TestDisplayTimes param) {

        NiceMock<MockTimeSource> mockTimeSource{};
        ON_CALL(mockTimeSource, month).WillByDefault(Return(param.month.value_or(0)));
        ON_CALL(mockTimeSource, day).WillByDefault(Return(param.day.value_or(0)));

        ON_CALL(mockTimeSource, hours).WillByDefault(Return(param.hours.value_or(0)));
        ON_CALL(mockTimeSource, minutes).WillByDefault(Return(param.minutes.value_or(0)));
        ON_CALL(mockTimeSource, seconds).WillByDefault(Return(param.seconds.value_or(0)));

        When(Method(ArduinoFake(), millis)).Return(100, 10000, 10000, 10000);

        const std::string &paramName = TestDisplayTimes::ParamToString(param);

        ConsolePrinter printer{};
        PrintMatrix display{printer, MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_BUFFER_SIZE};

        WordClock wordClock{mockTimeSource, display};

        wordClock.setCurrentMode(displayMode);
        wordClock.update();
        display.update();

        std::string displayDirNameNormalised = "";
        for (int i = 0; i < displayDirName.length(); i++) {
            const char c = displayDirName[i];
            if (i > 0 && isupper(c)) {
                displayDirNameNormalised += "-";
            }
            displayDirNameNormalised += tolower(displayDirName[i]);
        }

        const auto filename = paramName + ".txt";
        const fs::path expectedOutputPath = canonical(fs::path(__FILE__)).parent_path() / "display" / displayDirNameNormalised / filename;

        // std::ofstream outFile;
        // outFile.open(expectedOutputPath);
        // outFile.write(printer.bufferData(), printer.bufferSize());
        // outFile.close();

        if (!fs::exists(expectedOutputPath)) {
            FAIL() << "Couldn't find expected output: " << expectedOutputPath;
        }

        std::ifstream inputFile(expectedOutputPath.string());
        std::stringstream inputBuffer;
        inputBuffer << inputFile.rdbuf();

        EXPECT_STREQ(inputBuffer.str().c_str(), printer.bufferData());
    }
};

#define TEST_WORDCLOCK_DISPLAY(displayMode, testDirName, testParams)                                                                                                               \
    class TestModeDisplay##testDirName : public TestModeDisplayFixture {};                                                                                                         \
    TEST_P(TestModeDisplay##testDirName, TestModeDisplay##testDirName) { doTheTest(displayMode, #testDirName, GetParam()); }                                                       \
    INSTANTIATE_TEST_SUITE_P(TestDisplayTimesGroup, TestModeDisplay##testDirName, testParams, TestDisplayTimes::ParamInfoToString);

auto getWordHoursTestParams() {
    std::vector<TestDisplayTimes> testDisplayTimes{};
    for (uint8_t i = 0; i < 24; i++) {
        testDisplayTimes.push_back({.hours = i, 17, 39});
        testDisplayTimes.push_back({.hours = i, 42, 39});
    }

    return testing::ValuesIn(testDisplayTimes);
}

auto getWordMinutesTestParams() {
    std::vector<TestDisplayTimes> testDisplayTimes{};
    for (float i = 0; i < 60; i += 2.5f) {
        testDisplayTimes.push_back({.hours = 17, static_cast<uint8_t>(std::lround(i)), 39});
    }

    return testing::ValuesIn(testDisplayTimes);
}

auto getLiveTimeTestParams() {
    std::vector<TestDisplayTimes> testDisplayTimes{};
    for (int i = 0; i < 12; i++) {
        testDisplayTimes.push_back({.hours = i * 2, i * 5, 60 - (i * 5)});
    }

    return testing::ValuesIn(testDisplayTimes);
}

auto getSecondsTestParams() {
    std::vector<TestDisplayTimes> testDisplayTimes{};
    for (uint8_t i = 0; i < 60; i++) {
        testDisplayTimes.push_back({.hours = 8, 17, i});
    }

    return testing::ValuesIn(testDisplayTimes);
}

auto getDatesTestParams() {
    std::vector<TestDisplayTimes> testDisplayTimes{};
    for (uint8_t i = 1; i <= 12; i++) {
        // TODO: Make support dates. Can we use named property init and std::optional
        testDisplayTimes.push_back({i, i * 2});
    }

    return testing::ValuesIn(testDisplayTimes);
}

TEST_WORDCLOCK_DISPLAY(MODE_DISPLAY_WORD, WordHours, getWordHoursTestParams());
TEST_WORDCLOCK_DISPLAY(MODE_DISPLAY_WORD, WordMinutes, getWordMinutesTestParams());
TEST_WORDCLOCK_DISPLAY(MODE_DISPLAY_LIVETIME, LiveTime, getLiveTimeTestParams());
TEST_WORDCLOCK_DISPLAY(MODE_DISPLAY_SECONDS, Seconds, getSecondsTestParams());
TEST_WORDCLOCK_DISPLAY(MODE_DISPLAY_DATE, Dates, getDatesTestParams());

int main(int argc, char **argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS())
        ;

    // Always return zero-code and allow PlatformIO to parse results
    return 0;
}