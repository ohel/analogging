#include "Arduino.h"
#include "string.h"

#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ESP8266HTTPClient.h"

#include "wificonfig.h"
#include "apiurl.h"

// On D1 boards the integrated LED is inverted (on when pin is LOW).
#ifdef ESP8266_WEMOS_D1MINI
    static const int _PIN_LEVEL_ON = LOW;
#else
    static const int _PIN_LEVEL_ON = HIGH;
#endif

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
HTTPClient _CLIENT;

String getAnalogValue() {
    return String(analogRead(A0));
}

void setupWifi() {

    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);

    WiFi.mode(WIFI_STA);

    Serial.println("");
    if (_WIFI_IP == INADDR_NONE) {
        Serial.println("Using dynamic IP address.");
    } else {
        Serial.println("Using static IP address: " + String(_WIFI_IP.toString()));
        WiFi.config(_WIFI_IP, _WIFI_GATEWAY, subnet, dns);
    }

    WiFi.begin(_WIFI_SSID, _WIFI_PASSWORD);
    int timeout = 0;
    Serial.print("Connecting to: ");
    Serial.println(_WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED && timeout < 10) {
        delay(1000);
        Serial.print(".");
        timeout++;
    }
    Serial.println("");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected using IP address: ");
        Serial.println(WiFi.localIP());
    }

    _SERVER.on("/", HTTP_GET, [](){
        Serial.println("GET: /");
        String response = String(_INDEX_HTML);
        response.replace("VALUE", getAnalogValue());
        _SERVER.send(200, "text/html", response);
    });

}

// Deep-sleep for specified amount of hours, one hour at a time.
// If powered on (not a deep-sleep reset), nothing will happen.
// Call this twice: in the beginning of setup and at the end of setup.
void deepSleepCycle(uint32_t hours, bool end_of_setup = false) {

    uint32_t reset_counter = 0;

    if (!end_of_setup) {
        if (ESP.getResetReason() == "Deep-Sleep Wake") {
            Serial.print("Waking up from deep-sleep via reset pin. Reset counter: ");
            ESP.rtcUserMemoryRead(0, &reset_counter, sizeof(reset_counter));
            reset_counter++;
            ESP.rtcUserMemoryWrite(0, &reset_counter, sizeof(reset_counter));
            Serial.println(reset_counter);
        } else {
            Serial.println("Zeroing reset counter.");
            ESP.rtcUserMemoryWrite(0, &reset_counter, sizeof(reset_counter));
            return;
        }
    }

    // Sleep one hour at a time.
    // The deep sleep is unrealiable with larger values.
    if (reset_counter < hours) {
        Serial.println("Going to deep-sleep for 1 hour.");
        ESP.deepSleep(3600*1e6);
    }
    reset_counter = 0;
    ESP.rtcUserMemoryWrite(0, &reset_counter, sizeof(reset_counter));

}

void setup() {

    Serial.begin(115200);

    deepSleepCycle(12);

    // For debugging and info.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, _PIN_LEVEL_ON);

    setupWifi();

    Serial.println("Connecting to: " + String(_APIURL));
    _CLIENT.setTimeout(10000);
    _CLIENT.begin(_APIURL);

    int responseCode = _CLIENT.POST("{\"value\":\"" + getAnalogValue() + "\"}");
    if (responseCode > 0) {
        Serial.println("Response: " + String(responseCode));
        Serial.println(_CLIENT.getString());
    } else {
        Serial.println("Error: " + _CLIENT.errorToString(responseCode));
    }

    _CLIENT.end();

    // For debugging and info.
    digitalWrite(LED_BUILTIN, _PIN_LEVEL_ON == HIGH ? LOW : HIGH);

    deepSleepCycle(12, true);

    // For debugging, unreachable code with deep-sleep.
    _SERVER.begin();
    Serial.println("Server is running.");

}

// For debugging, unreachable code with deep-sleep.
void loop() {

    _SERVER.handleClient();

}
