import serial
from datetime import datetime
#import localization as lx
import numpy as np
from os import path
import time
import csv
import json

#RSSI LOC WiFi
d0 = 1 #d0 distance
ref_rssi = -40 #rssi at d0
var_rssi = 0 #var wifi rssi
rssi_distance = 0

def estimate_distance(power_received, params=None):
    """This function returns an estimated distance range
       given a single radio signal strength (RSS) reading
       (received power measurement) in dBm.
    Parameters:
        power_received (float): RSS reading in dBm
        params (4-tuple float): (d_ref, power_ref, path_loss_exp, stdev_power)
            d_ref is the reference distance in m
            power_ref is the received power at the reference distance
            path_loss_exp is the path loss exponent
            stdev_power is standard deviation of received Power in dB
    Returns:
        (d_est, d_min, d_max): a 3-tuple of float values containing
            the estimated distance, as well as the minimum and maximum
            distance estimates corresponding to the uncertainty in RSS,
            respectively, in meters rounded to two decimal points
    """

    if params is None:
        params = (1.0, -55.0, 2.0, 2.5)
          # the above values are arbitrarily chosen "default values"
          # should be changed based on measurements

    d_ref = params[0] # reference distance
    power_ref = params[1] # mean received power at reference distance
    path_loss_exp = params[2] # path loss exponent
    stdev_power = params[3] # standard deviation of received power

    uncertainty = 2*stdev_power # uncertainty in RSS corresponding to 95.45% confidence

    d_est = d_ref*(10**(-(power_received - power_ref)/(10*path_loss_exp)))
    d_min = d_ref*(10**(-(power_received - power_ref + uncertainty)/(10*path_loss_exp)))
    d_max = d_ref*(10**(-(power_received - power_ref - uncertainty)/(10*path_loss_exp)))

    return (np.round(d_est,2), np.round(d_min,2), np.round(d_max,2))


# # example usage, for testing
# if __name__ == '__main__':
#     print("Example: say RSS = -70dBm")
#     d_est, d_min, d_max = estimate_distance(-70)
#     print("Estimated distance in meters is: ", d_est)
#     print("Distance uncertainty range in meters is: ", (d_min, d_max))
#     print(estimate_distance(-70, (1.0, -55.0, 4, 3)))

#{"ssid": "FTM_1", "mac": "f6:12:fa:5a:05:10", "rtt_est": 346, "rtt_raw": 478 , "dist_est": 5200, "num_frames": 15, "mean_rssi": -45.00}

MEASURE_TIME = 2 #minutes


value = input("Please enter file name for the measurement logs\n")
lora_file = 'Lora/lora'+value+'.csv' #edit lora path based on freq
lora_head = ['timestamp','ID','RSSI','DIST']

#ser = serial.Serial('/dev/tty.usbserial-01EED935',115200)
ser_lora = serial.Serial('/dev/tty.usbmodem1101',115200)
ser_lora.flushInput()

if (path.exists(lora_file)):
    l = open(lora_file, 'a', encoding='UTF8', newline='')
    write_lora = csv.writer(l)
else:
    l = open(lora_file, 'w', encoding='UTF8', newline='')
    write_lora = csv.writer(l)
    write_lora.writerow(lora_head)

TIME_LIMIT = time.time() + 60*MEASURE_TIME 

trilateration_data = []

while (time.time()<=TIME_LIMIT):
    try:
        ser_bytes = ser_lora.readline()
        decoded_bytes = ser_bytes.decode()
        #print(str(datetime.now())+":"+decoded_bytes)
        try:
            if (json.loads(decoded_bytes)):
                j = json.loads(decoded_bytes)

                data = []

                data.append(datetime.now())
                data.append(j["ID"])
                data.append(j["rssi_average"])
                data.append(j["dist_average"])
                #print(data)
                #write_lora.writerow(data)
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
                write_lora.writerow(trilateration_data[i])

            if len(trilateration_data)>2:
                print("localizatioin in progress")

            trilateration_data = []

    except:
        print("Keyboard Interrupt")
        break







