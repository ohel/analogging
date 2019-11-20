#include "Arduino.h"
#include "string.h"

#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ESP8266HTTPClient.h"

#include "wificonfig.h"
#include "apiurl.h"

// On D1 boards the integrated LED is inverted (on when pin is LOW).
#ifdef ESP8266_WEMOS_D1MINI
    static const int _LED_PIN_LEVEL_ON = LOW;
#else
    static const int _LED_PIN_LEVEL_ON = HIGH;
#endif

#ifdef DEBUG
    const char _INDEX_HTML[] =
    "    <html lang=\"en\">"
    "    <head>"
    "    <meta charset=\"utf-8\">"
    "    <style>"
    "    body {"
    "        background-color: black;"
    "        color: lightgray;"
    "        font-family: sans-serif;"
    "        text-align: center;"
    "    }"
    "    h1 {"
    "        font-size: 2rem;"
    "        margin-top: 30px;"
    "    }"
    "    p {"
    "        display: inline;"
    "        margin: 0;"
    "        vertical-align: center;"
    "    }"
    "    </style>"
    "    </head>"
    "    <body>"
    "        <h1>Analog in value</h1>"
    "        <p>VALUE</p>"
    "    </body>"
    "    </html>"
    ;

    ESP8266WebServer _SERVER(80);
#endif

// Sensor is connected to GPIO5 (D1 on a D1 board).
// The Capacitive Soil Moisture Sensor V1.2 takes about 5mA of power and therefore can be powered via GPIO.
static const int _SENSOR_PIN = 5;

HTTPClient _CLIENT;

String getAnalogValue() {
    return String(analogRead(A0));
}

inline void printDebugNoNewLine(String message) {
#ifdef DEBUG
    Serial.print(message);
#endif
}

inline void printDebug(String message) {
#ifdef DEBUG
    Serial.println(message);
#endif
}

void setupWifi() {

    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);

    WiFi.mode(WIFI_STA);

    Serial.println("");
    if (_WIFI_IP == INADDR_NONE) {
        printDebug("Using dynamic IP address.");
    } else {
        printDebug("Using static IP address: " + String(_WIFI_IP.toString()));
        WiFi.config(_WIFI_IP, _WIFI_GATEWAY, subnet, dns);
    }

    WiFi.begin(_WIFI_SSID, _WIFI_PASSWORD);
    int timeout = 0;
    printDebug("Connecting to: " + String(_WIFI_SSID));
    while (WiFi.status() != WL_CONNECTED && timeout < 60) {
        delay(1000);
        printDebugNoNewLine(".");
        timeout++;
    }
    printDebug("");
    if (WiFi.status() == WL_CONNECTED) {
        printDebug("WiFi connected using IP address: " + WiFi.localIP().toString());
    } else {
        printDebug("WiFi connection timed out.");
    }

#ifdef DEBUG
    _SERVER.on("/", HTTP_GET, [](){
        Serial.println("GET: /");
        String response = String(_INDEX_HTML);
        response.replace("VALUE", getAnalogValue());
        _SERVER.send(200, "text/html", response);
    });
#endif

}

// Deep-sleep for specified amount of hours, one hour at a time.
// If powered on (not a deep-sleep reset), nothing will happen.
// Call this twice: in the beginning of setup (end_of_setup == false)
// and at the end of setup (end_of_setup == true).
void deepSleepCycle(uint32_t hours, bool end_of_setup = false) {

    uint32_t reset_counter = 0;
    bool waking_from_sleep = ESP.getResetReason() == "Deep-Sleep Wake";

    if (!end_of_setup) {
        if (waking_from_sleep) {
            printDebugNoNewLine("Waking up from deep-sleep via reset pin. Reset counter: ");
            ESP.rtcUserMemoryRead(0, &reset_counter, sizeof(reset_counter));
            reset_counter++;
            ESP.rtcUserMemoryWrite(0, &reset_counter, sizeof(reset_counter));
            printDebug(String(reset_counter));
        } else {
            printDebug("Zeroing reset counter.");
            ESP.rtcUserMemoryWrite(0, &reset_counter, sizeof(reset_counter));
            return;
        }
    }

    // With larger values, deep-sleep is unrealiable: it might never wake up and consume a lot of power.
    // Therefore sleep one hour at a time.
    // In reality, the ESP sleeps around 59 minutes when told to sleep 60.
    if (reset_counter < hours) {
        // If this is the first time going to sleep, do the radio calibration once.
        // Otherwise, disable radio (WiFi).
        RFMode wake_mode = waking_from_sleep ? WAKE_RF_DISABLED : WAKE_RFCAL;
        if (reset_counter + 1 == hours) {
            // Wake up with radio on if the next power cycle finishes sleeping.
            wake_mode = WAKE_NO_RFCAL;
        }
        printDebug("Going to deep-sleep for 1 hour.");
        // 1: WAKE_RFCAL
        // 2: WAKE_NO_RFCAL
        // 4: WAKE_RF_DISABLED
        printDebug("Radio mode will be: " + String(wake_mode));

#ifdef DEBUG
        // Give some time to print debug messages to monitor before going to sleep.
        delay(100);
#endif

        ESP.deepSleep(3600*1e6, wake_mode);
    }
    reset_counter = 0;
    ESP.rtcUserMemoryWrite(0, &reset_counter, sizeof(reset_counter));

}

void setup() {

    Serial.begin(115200);

    deepSleepCycle(12);

    // For debugging and info.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, _LED_PIN_LEVEL_ON);

    // Power on the soil moisture sensor.
    pinMode(_SENSOR_PIN, OUTPUT);
    digitalWrite(_SENSOR_PIN, HIGH);

    setupWifi();

    printDebug("Connecting to: " + String(_APIURL));
    _CLIENT.setTimeout(60000);
    _CLIENT.begin(_APIURL);

    int responseCode = _CLIENT.POST("{\"value\":\"" + getAnalogValue() + "\"}");
    if (responseCode > 0) {
        printDebug("Response: " + String(responseCode));
        printDebug(_CLIENT.getString());
    } else {
        printDebug("Error: " + _CLIENT.errorToString(responseCode));
    }

    _CLIENT.end();

    // For debugging and info.
    digitalWrite(LED_BUILTIN, _LED_PIN_LEVEL_ON == HIGH ? LOW : HIGH);

    // Power off the soil moisture sensor.
    digitalWrite(_SENSOR_PIN, LOW);

    deepSleepCycle(12, true);

    // For debugging, unreachable code with deep-sleep.
#ifdef DEBUG
    _SERVER.begin();
    Serial.println("Server is running.");
#endif

}

// For debugging, unreachable code with deep-sleep.
void loop() {

#ifdef DEBUG
    _SERVER.handleClient();
#endif

}
