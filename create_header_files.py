import json
import os.path
import re
import traceback

def createWifiConfig(cfg_source, cfg_header):
    try:
        if os.path.isfile(cfg_source):
            with open(cfg_source, 'r') as f:
                contents = f.read()
            config = json.loads(contents)
            ssid = config['ssid']
            password = config['password']
            ip = config['ip']
        else:
            raise Exception('Missing WiFi config.')

        # If static IPv4 is set e.g. 192.168.1.4, a gateway of 192.168.1.1 is assumed.
        gateway = re.sub(r'[0-9]{1,}$', '1', ip).replace('.', ',')

        with open(cfg_header, 'w') as f:
            f.write('const char* _WIFI_SSID = "' + ssid + '";\n')
            f.write('const char* _WIFI_PASSWORD = "' + password + '";\n')
            f.write('const IPAddress _WIFI_IP(' + ip.replace('.', ',') + ');\n')
            f.write('const IPAddress _WIFI_GATEWAY(' + gateway + ');\n')

    except Exception:
        traceback.print_exc()
        exit(1)

createWifiConfig('src/wifi_cfg.json', 'src/wifi_cfg.h')

if not os.path.isfile('src/apiurl.h'):
    print('Create the src/apiurl.h file with contents like:')
    print('const char* _APIURL = "your API URL with API key"')
    exit(1)
