import numpy
import math
import paho.mqtt.client as mqtt
from datetime import datetime
import json
import atexit

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
light_utime = None
idle_utime = None

fout = open('output.csv', 'w+')

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("#")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    #print(msg.topic+" "+str(msg.payload))
    json_obj = json.loads(msg.payload) 
    raw = json_obj["value"].split(",")
    if raw[2] == "a":
        #acc_data.append([int(raw[1]),float(raw[3]),float(raw[4]),float(raw[5])])
        avg_a = math.sqrt(pow(float(raw[3]),2) + pow(float(raw[4]),2) + pow(float(raw[5]), 2))
        acc_data.append([int(raw[1]), avg_a])
        process_acc_data()

    elif raw[2] == "b":
        baro_data.append([int(raw[1]),float(raw[3])])
        process_baro_data()

    elif raw[2] == "l":
        light_data.append([int(raw[1]),float(raw[3])])
        process_light_data()
    
    elif raw[2] == "t":
        temp_data.append([int(raw[1]),float(raw[3])])
    
    elif raw[2] == "h":
        humid_data.append([int(raw[1]),float(raw[3])])
    
    fout.write(msg.payload + '\n')

    # DEBUG
    # sec_diff = int((datetime.now()- start_time).total_seconds())
    # if sec_diff != 0 and sec_diff % 5 == 0:
    #     print("====================")
    #     print("Time: " + str(sec_diff)+"s")
    #     print("acc_data: " + str(len(acc_data)) + "samples receied at " + str(len(acc_data)/sec_diff)+"Hz")
    #     print("baro_data: " + str(len(baro_data)) + "samples receied at " + str(len(baro_data)/sec_diff)+"Hz")
    #     print("light_data: " + str(len(light_data)) + "samples receied at " + str(len(light_data)/sec_diff)+"Hz")
    
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
    pass

def process_light_data():
    global indoor
    global light_utime

    now_time = datetime.now()
    if(light_utime is None or int((now_time - light_utime).total_seconds()) >= 10):
        if len(light_data) >= LIGHT_WINDOW:
            mean = numpy.mean([x[1] for x in light_data[-LIGHT_WINDOW:]])
            # print([x[1] for x in light_data[-LIGHT_WINDOW:]])
            # print(mean)
            state = False if mean >= 400 else True
            if(indoor != state):
                indoor = state
                if(indoor == True):
                    print(str(light_data[-1][0]) + ', INDOOR\n')
                else:
                    print(str(light_data[-1][0]) + ', OUTDOOR\n')


def exit_handler():
    fout.close()

atexit.register(exit_handler)
# Connect to mqtt server
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("cs4222g17test@gmail.com", password="jnuwYcGglObRCNs6")
client.connect("ocean.comp.nus.edu.sg", 1883, 1000)
client.loop_forever()