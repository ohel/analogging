#!/bin/bash

# The contents of $api_url_in should be a single line.
# The contents of $wifi_cfg_in should be these three lines:
#    ssid
#    password
#    static IPv4 address (use 0.0.0.0 for dynamic IP)
# If static IPv4 is set e.g. 192.168.1.4, a gateway of 192.168.1.1 is assumed.

piodev="1A86:7523"

wifi_cfg_in="web/wifi.cfg"
wifi_cfg_out="src/wificonfig.h"
api_url_in="web/apiurl.cfg"
api_url_out="src/apiurl.h"

[ ! -e $api_url_in ] && echo "No API URL" && exit 1
sed "s/\(.*\)/const char* _APIURL = \"\0\";/" $api_url_in > $api_url_out

wifi_ssid=$(head -n 1 $wifi_cfg_in 2>/dev/null)
wifi_pass=$(head -n 2 $wifi_cfg_in 2>/dev/null | tail -n 1)
wifi_ip=$(tail -n 1 $wifi_cfg_in 2>/dev/null)
echo ${wifi_ssid:-"wlan"} | sed "s/\(.*\)/const char* _WIFI_SSID = \"\0\";/" > $wifi_cfg_out
echo ${wifi_pass:-"password"} | sed "s/\(.*\)/const char* _WIFI_PASSWORD = \"\0\";/" >> $wifi_cfg_out
echo ${wifi_ip:-"0.0.0.0"} | sed "s/\./, /g" | sed "s/\(.*\)/IPAddress _WIFI_IP(\0);/" >> $wifi_cfg_out
echo ${wifi_ip:-"0.0.0.0"} | sed "s/\./, /g" | sed "s/[0-9]\{1,\}$/1/" | sed "s/\(.*\)/IPAddress _WIFI_GATEWAY(\0);/" >> $wifi_cfg_out

port=$(pio device list | grep -B 2 "VID:PID=$piodev" | head -n 1)
pio run -t upload --upload-port $port
