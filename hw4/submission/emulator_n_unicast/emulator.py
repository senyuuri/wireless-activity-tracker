#!/usr/bin/python3
import serial
import traceback
from argparse import ArgumentParser
from time import sleep, time

parser = ArgumentParser(description='Emulates a real-time data collection from offline data', epilog="?")
parser.add_argument("-s", "--serialdevice", dest="serialdevice", help="serial port of the sensorTag to read data (e.g. /dev/ttyACM0)", metavar="SERIAL", required=True)
parser.add_argument("-f", "--file", dest="filepath", help="file containing the collected data (not the ground truth one)", metavar="FILE", required=True)

args = parser.parse_args()

ACCEPTABLE_DELAY_MARGIN = 50 #millis
TS=0
print("**********************************************************************")
print("PRESS THE RESET BUTTON OF THE USB-CONNECTED SENSORTAG TO START SENDING")
print("**********************************************************************")
try:
	ser = serial.Serial(args.serialdevice, 115200, timeout=30)
	f = open(args.filepath,"r")

	emulator_start_time = round(time() * 1000) # ms
	line_file = f.readline()
	wearable_start_time = int(line_file.split(",")[TS])

	while(1):
		line_ser = ser.readline() 
		while("next" not in str(line_ser)):	
			print(line_ser)
			sleep(0.001)
			line_ser = ser.readline() 

		line_file = f.readline()
		if(line_file == ""):
			break

		emulator_now = round(time()*1000)
		wearable_now = int(line_file.split(",")[TS])

		# CASE 1: EMULATOR TIME IS BEHIND WEARABLE TIME 
		while( (emulator_now - emulator_start_time + ACCEPTABLE_DELAY_MARGIN) < (wearable_now - wearable_start_time)):
			sleep(0.0005)
			emulator_now = round(time()*1000)

		# CASE 2: WEARABLE TIME IS BEHIND EMULATOR TIME
		while( (emulator_now - emulator_start_time - ACCEPTABLE_DELAY_MARGIN) > (wearable_now - wearable_start_time)):
			line_file = f.readline()
			if(line_file != ""):
				print("SYNC DATA LOSS:" + line_file)
				wearable_now = int(line_file.split(",")[TS])

		print(str(emulator_now)+ ",E: " + str(emulator_start_time) + ",W: " + str(wearable_start_time) + ":" + line_file.split(",")[1] + ":" + str(line_file))
		line_bytes = line_file.encode()
		ser.write(line_bytes)	

except:
	print(traceback.print_exc())
