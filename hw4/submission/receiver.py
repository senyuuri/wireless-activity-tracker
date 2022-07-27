import paho.mqtt.client as mqtt
from datetime import datetime
import json
import numpy as np
import atexit
import math

curr_baro_data_id = 1

# Global states
floor_change = False
indoor = True
idle = True

# Received data
acc_data = []
baro_data = []
light_data = []
temp_data = []
humid_data = []

# Sliding window size
ACC_WINDOW = 100
BARO_WINDOW = 100
LIGHT_WINDOW = 5

# Last state change time
start_time = datetime.now()
floor_utime = None
indoor_utime = None
light_utime = None
idle_utime = None

fout = open('output.csv', 'w+')
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("#")

threshold = 0.21
mean_list = []
first_mean = 0
diff_list = []
window_list = []
time_list = []
size=20

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    # print(msg.topic+" "+str(msg.payload))
    json_obj = json.loads(msg.payload) 
    raw = json_obj["value"].split(",")
    # print(raw)
    if raw[2] == "a":
        avg_a = math.sqrt(pow(float(raw[3]),2) + pow(float(raw[4]),2) + pow(float(raw[5]), 2))
        acc_data.append([int(raw[1]), avg_a])
        process_acc_data()

    elif raw[2] == "b":
        baro_data.append([int(raw[1]),float(raw[3])])
        process_baro_data()

    elif raw[2] == "l":
        light_data.append([int(raw[1]),float(raw[3])])
        process_light_data()
    elif raw[2] == "h":
        humid_data.append([int(raw[1]),float(raw[3])])
    
    fout.write(msg.payload.decode('utf8') + '\n')
    # sec_diff = int((datetime.now()- start_time).total_seconds())
    # if sec_diff != 0 and sec_diff % 5 == 0:
    #     print("====================")
    #     print("Time: " + str(sec_diff)+"s")
    #     print("acc_data: " + str(len(acc_data)) + "samples receied at " + str(len(acc_data)/sec_diff)+"Hz")
    #     print("baro_data: " + str(len(baro_data)) + "samples receied at " + str(len(baro_data)/sec_diff)+"Hz")
    #     # print(baro_data)
    #     print("light_data: " + str(len(light_data)) + "samples receied at " + str(len(light_data)/sec_diff)+"Hz")
    #     # print(light_data)
    
def process_acc_data():
    global idle 
    global idle_utime

    now_time = datetime.now()
    if(idle_utime is None or int((now_time - idle_utime).total_seconds()) >= 10):
        if len(acc_data) >= ACC_WINDOW:
            window_data = [x[1] for x in acc_data[-ACC_WINDOW:]]
            diff = max(window_data) - min(window_data)
            new_idle = True if diff < 0.5 else False
            if new_idle != idle:
                idle = new_idle
                idle_utime = now_time
                if (diff >= 0.5):
                    print(str(acc_data[-1][0]) +',WALKING')
                else:
                    print(str(acc_data[-1][0]) +',IDLE')



def process_baro_data():
    global floor_change 
    global floor_utime
    now_time = datetime.now()
    if(floor_utime is None or int((now_time - floor_utime).total_seconds()) >= 10):
        if len(baro_data) > size:
            window_list = [i[1] for i in baro_data[-size:]]
            time_list = [i[0] for i in baro_data[-size:]]
            mean = np.mean(window_list)
            if len(baro_data) >= size:
                first_mean = np.mean([i[1] for i in baro_data[:size]])
            mean_list.append(mean)

            diff = mean - first_mean
            diff_list.append(diff)
            # with open("test.txt", "a") as myfile:
            #     myfile.write('time is: ' + str(time_list[len(time_list)-1])+ 'diff list len is: ' + str(len(diff_list)) + ' curr diff: ' + str(diff) + ' prev diff: ' + str(diff_list[len(diff_list)-2]) +'\n')
            is_floor_change = (diff > threshold and diff_list[len(diff_list)-2] < threshold) or (diff < threshold and diff_list[len(diff_list)-2] > threshold) or (diff > -threshold and diff_list[len(diff_list)-2] < -threshold) or (diff < -threshold and diff_list[len(diff_list)-2] > -threshold)
            if is_floor_change:
                new_floor_change = True
            else:
                new_floor_change = False

            if new_floor_change != floor_change:
                floor_change = new_floor_change
                floor_utime = now_time 
                if is_floor_change:
                    print(str(time_list[len(time_list)-2]) + ', FLOOR CHANGE')     
                else:
                    print(str(time_list[len(time_list)-2]) + ', NO FLOOR CHANGE')
    
def process_light_data():
    global indoor
    global light_utime

    now_time = datetime.now()
    if(light_utime is None or int((now_time - light_utime).total_seconds()) >= 10):
        if len(light_data) >= LIGHT_WINDOW:
            mean = np.mean([x[1] for x in light_data[-LIGHT_WINDOW:]])
            # print([x[1] for x in light_data[-LIGHT_WINDOW:]])
            # print(mean)
            state = False if mean >= 400 else True
            if(indoor != state):
                indoor = state
                if(indoor == True):
                    print(str(light_data[-1][0]) + ', INDOOR')
                else:
                    print(str(light_data[-1][0]) + ', OUTDOOR')


def exit_handler():
    fout.close()
# def write_to_file(content):
atexit.register(exit_handler)
# Connect to mqtt server
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("cs4222g17test@gmail.com", password="jnuwYcGglObRCNs6")
client.connect("ocean.comp.nus.edu.sg", 1883, 1000)
client.loop_forever()