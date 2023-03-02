import serial
from datetime import datetime
from os import path
import time
import csv
import json


#{"ssid": "FTM_1", "mac": "f6:12:fa:5a:05:10", "rtt_est": 346, "rtt_raw": 478 , "dist_est": 5200, "num_frames": 15, "mean_rssi": -45.00}

MEASURE_TIME = 10 #minutes
value = input("Please enter file name for the measurement logs\n")
wifi_file = 'WiFi/wifi'+value+'.csv'
wifi_head = ['timestamp','SSID','MAC','RSSI','RTT_EST','RTT_RAW', "DIST"]
lora_file = 'Lora/lora'+value+'.csv' #edit lora path based on freq
lora_head = ['timestamp','ID','RSSI','SNR']

#ser = serial.Serial('/dev/tty.usbserial-01EED935',115200)
ser = serial.Serial('/dev/tty.usbmodem101',115200)
ser.flushInput()

if (path.exists(wifi_file)):
    f = open(wifi_file, 'a', encoding='UTF8', newline='')
    write_wifi = csv.writer(f)
else:
    f = open(wifi_file, 'w', encoding='UTF8', newline='')
    write_wifi = csv.writer(f)
    write_wifi.writerow(wifi_head)

if (path.exists(lora_file)):
    l = open(lora_file, 'a', encoding='UTF8', newline='')
    write_lora = csv.writer(l)
else:
    l = open(lora_file, 'w', encoding='UTF8', newline='')
    write_lora = csv.writer(l)
    write_lora.writerow(lora_head)

TIME_LIMIT = time.time() + 60*MEASURE_TIME 

while (time.time()<=TIME_LIMIT):
    try:
        ser_bytes = ser.readline()
        decoded_bytes = ser_bytes.decode()
        print(str(datetime.now())+":"+decoded_bytes)
        try:
            if (json.loads(decoded_bytes)):
                j = json.loads(decoded_bytes)
                print(j["ssid"])
                print(j["mac"])

                data = []

                data.append(datetime.now())
                data.append(j["ssid"])
                data.append(j["mac"])
                data.append(j["mean_rssi"])
                data.append(j["rtt_est"])
                data.append(j["rtt_raw"])
                data.append(j["dist_est"])
                write_wifi.writerow(data)
            #elif data[0]=='LORA':
                #data[3] = float(data[3])
                #print("WriteLoRa")
                #data[0]=datetime.now()
                #write_lora.writerow(data)
            #else:
                #print("Uknown Serial Message")
        except:
            print("Not a JSON line")

    except:
        print("Keyboard Interrupt")
        break







