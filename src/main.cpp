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

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, _PIN_LEVEL_ON);

    Serial.begin(115200);
    setupWifi();

    Serial.println("Connecting to: " + String(_APIURL));
    _CLIENT.begin(_APIURL);

    int responseCode = _CLIENT.POST("{\"value\":\"" + getAnalogValue() + "\"}");
    if (responseCode > 0) {
        Serial.println("Response: " + String(responseCode));
        Serial.println(_CLIENT.getString());
    } else {
        Serial.println("Error: " + _CLIENT.errorToString(responseCode));
    }

    _CLIENT.end();

    digitalWrite(LED_BUILTIN, _PIN_LEVEL_ON == HIGH ? LOW : HIGH);
    Serial.println("Entering deep sleep for maximum time.");
    ESP.deepSleep(ESP.deepSleepMax());

    _SERVER.begin();
    Serial.println("Server is running.");

}

void loop() {

    _SERVER.handleClient();

}
