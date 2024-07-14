import serial
from datetime import datetime
import localization as lx
import numpy as np
from os import path
import time
import csv
import json

#Anchor positions
FTM_1_POS = (-251,-172,66)
FTM_2_POS = (377,146,70)
FTM_3_POS = (-226,738,89)
FTM_4_POS = (256,628,89)
FTM_5_POS = (-251,-172,66)
FTM_6_POS = (377,146,70)
FTM_7_POS = (-226,738,89)
FTM_8_POS = (256,628,89)
FTM_9_POS = (-251,-172,66)

#RSSI LOC WiFi
d0 = 1 #d0 distance
#ref_rssi = -32.08 #rssi at d0
ref_rssi = -28.88
var_rssi = 0.24 #var wifi rssi
rssi_distance = 0
path_loss = 3.5
params = (d0, ref_rssi, path_loss, var_rssi)

tof_calib_1meter = 5303 #cm

#RSSI distance estimation
def estimate_rssi_distance(power_received):
    d_ref = params[0] # reference distance
    power_ref = params[1] # mean received power at reference distance
    path_loss_exp = params[2] # path loss exponent
    stdev_power = np.sqrt(params[3]) # standard deviation of received power

    uncertainty = 2*stdev_power # uncertainty in RSS corresponding to 95.45% confidence

    d_est = d_ref*(10**(-(power_received - power_ref)/(10*path_loss_exp)))
    d_min = d_ref*(10**(-(power_received - power_ref + uncertainty)/(10*path_loss_exp)))
    d_max = d_ref*(10**(-(power_received - power_ref - uncertainty)/(10*path_loss_exp)))

    return (np.round(d_est,2), np.round(d_min,2), np.round(d_max,2))

#{"ssid": "FTM_1", "mac": "f6:12:fa:5a:05:10", "rtt_est": 346, "rtt_raw": 478 , "dist_est": 5200, "num_frames": 15, "mean_rssi": -45.00}

MEASURE_TIME = 20 #minutes


value = input("Please enter file name for the measurement logs\n")
wifi_file = 'WiFi/wifi'+value+'.csv'
wifi_head = ['timestamp','SSID','MAC','RSSI','RTT_EST','RTT_RAW', "DIST","POINT"]

#ser = serial.Serial('/dev/tty.usbserial-01EED935',115200)
ser_wifi = serial.Serial('/dev/tty.usbmodem2101',115200)
ser_wifi.flushInput()

if (path.exists(wifi_file)):
    f = open(wifi_file, 'a', encoding='UTF8', newline='')
    write_wifi = csv.writer(f)
else:
    f = open(wifi_file, 'w', encoding='UTF8', newline='')
    write_wifi = csv.writer(f)
    write_wifi.writerow(wifi_head)

TIME_LIMIT = time.time() + 60*MEASURE_TIME 

trilateration_data = []
count = 0
point = 0
while (time.time()<=TIME_LIMIT):
    try:
        ser_bytes = ser_wifi.readline()
        decoded_bytes = ser_bytes.decode()
        #print(str(datetime.now())+":"+decoded_bytes)
        try:
            if (json.loads(decoded_bytes)):
                j = json.loads(decoded_bytes)

                data = []

                data.append(datetime.now())
                data.append(j["ssid"])
                data.append(j["mac"])
                data.append(j["mean_rssi"])
                data.append(j["rtt_est"])
                data.append(j["rtt_raw"])
                data.append(j["dist_est"])
                data.append(count)
                #print(data)
                #write_wifi.writerow(data)
                trilateration_data.append(data)
                print(j)
        except:
            #print("Not a JSON line")
            print('')
        
        if decoded_bytes.find("SCAN DONE") != -1:
            print("Try to localize")
            #print(trilateration_data)
            stamp = datetime.now()
            for i in range(len(trilateration_data)):
                trilateration_data[i][0] = stamp
                trilateration_data[i][7] = point
                write_wifi.writerow(trilateration_data[i])

            #Trilateration
            P_ToF=lx.Project(mode='3D',solver='LSE')
            P_RSSI=lx.Project(mode='3D',solver='LSE')
            #Anchors
            P_ToF.add_anchor('FTM_1',FTM_1_POS)
            P_ToF.add_anchor('FTM_2',FTM_2_POS)
            P_ToF.add_anchor('FTM_3',FTM_3_POS)
            P_ToF.add_anchor('FTM_4',FTM_4_POS)
            P_ToF.add_anchor('FTM_5',FTM_5_POS)
            P_ToF.add_anchor('FTM_6',FTM_6_POS)
            P_ToF.add_anchor('FTM_7',FTM_7_POS)
            P_ToF.add_anchor('FTM_8',FTM_8_POS)
            P_ToF.add_anchor('FTM_9',FTM_9_POS)
            P_RSSI.add_anchor('FTM_1',FTM_1_POS)
            P_RSSI.add_anchor('FTM_2',FTM_2_POS)
            P_RSSI.add_anchor('FTM_3',FTM_3_POS)
            P_RSSI.add_anchor('FTM_4',FTM_4_POS)
            P_RSSI.add_anchor('FTM_5',FTM_5_POS)
            P_RSSI.add_anchor('FTM_6',FTM_6_POS)
            P_RSSI.add_anchor('FTM_7',FTM_7_POS)
            P_RSSI.add_anchor('FTM_8',FTM_8_POS)
            P_RSSI.add_anchor('FTM_9',FTM_9_POS)

            #Gateway
            tof,label=P_ToF.add_target()
            rssi,label=P_RSSI.add_target()

            if len(trilateration_data)>2:
                print("localizatioin in progress")
                for i in range(len(trilateration_data)):
                    tof.add_measure(trilateration_data[i][1],(trilateration_data[i][6]-tof_calib_1meter))
                    rssi.add_measure(trilateration_data[i][1],(estimate_rssi_distance(trilateration_data[i][3])[0]*100))

                P_ToF.solve()
                P_RSSI.solve()

                print("ToF localization: "+str(tof.loc))
                print("RSSI localization: "+str(rssi.loc))


            trilateration_data = []
            point += 1

    except:
        print("Keyboard Interrupt")
        break







