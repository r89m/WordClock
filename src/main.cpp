#include "HT1632Matrix.h"

#include <MPR121Button.h>
#include <Adafruit_MPR121.h>

#include "Symbols.h"
#include "Matrix.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <WiFiUdp.h>

#include "NTP.h"
#include "NTPTimeSource.h"
#include "PrintMatrix.h"
#include "WordClock.h"

#define _PRINT(x) Serial.print(x)
#define _PRINTLN(x) Serial.println(x)

#define I2C_ADDR_TOUCH 0x5A

class SerialPrinter : public Printer {
    public:
    void print(const char character) override { Serial.print(character); }
    void print(const char *output) override { Serial.print(output); }
    void println(const char *output) override { Serial.println(output); }
};

SerialPrinter printer;

HT1632Matrix displayHT1632{MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_BUFFER_SIZE};
PrintMatrix displayPrint{printer, MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_BUFFER_SIZE};
MatrixCollection display{{&displayHT1632, &displayPrint}};

// Button Variables
Adafruit_MPR121 TouchSensor = Adafruit_MPR121();
MPR121Button mainButton = MPR121Button(TouchSensor, 6);

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

NTPTimeSource timeSource{ntp};

WordClock wordClock{timeSource, display};

// Touch Button Sensitivity
uint8_t TouchSensitivity_Touch = 40;
uint8_t TouchSensitivity_Release = 5;

AsyncWebServer server(80);

// TODO: Move WordClock into its own Class - pass in TimeSource and Display - Serial / LEDs or both
// TODO: Serial output - use Grid letters
// TODO: Migrate to Wifi Core
// TODO: Enable OTACore

void onMainButtonPressed(Button &button, uint16_t duration) { wordClock.cycleMode(); }

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
        wordClock.cycleMode();
        request->send(200, "text/plain", "Mode cycled");
    });

    server.on("/mode-set", HTTP_POST, [](AsyncWebServerRequest *request) {
        const auto newModeStr = request->getParam("mode", true);
        const auto newMode = newModeStr->value().toInt();
        wordClock.setCurrentMode(newMode);
        request->send(200, "text/plain", ("Mode set to: " + newModeStr->value()).c_str());
    });

    // Prepare the touch sensor
    if (!TouchSensor.begin(I2C_ADDR_TOUCH)) {
        Serial.println("Couldn't connect to touch sensor");
    }

    // Initialise the display
    display.begin();

    mainButton.onRelease(0, 500, onMainButtonPressed);

    wordClock.setCurrentMode(MODE_DEFAULT);

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
    wordClock.update();
    display.update();
}
