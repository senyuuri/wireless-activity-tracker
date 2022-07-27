import math
import numpy
from datetime import datetime

isIdle = True

acc_data = []
window_size = 100

start_time = datetime.now()
update_time = None
numpy.set_printoptions(suppress=True)

with open('dataset/walk_1/data_collect_2018_03_20_13_52_12.csv', 'r') as fin:
    line = fin.readline()
    while(line != ''):
        parts = line.split(',')
        if(parts[1] == 'a'):
            avg_a = math.sqrt(pow(float(parts[2]),2) + pow(float(parts[3]),2) + pow(float(parts[4]), 2))
            #print(parts[0] + ','+ str(avg_a))
            acc_data.append([parts[0], avg_a])
            
            # if update_time is None or (start_time-update_time).total_seconds() >= 10:
            if len(acc_data) >= window_size:
                window_data = [x[1] for x in acc_data[-window_size:]]
                diff = max(window_data) - min(window_data)
                if (diff >= 0.5):
                    print(parts[0] +',1')
                else:
                    print(parts[0] +',0')
            #     mean = numpy.mean([x[1] for x in acc_data[-window_size:]])
            #     # print(acc_data[-window_size:])
            #     # print(mean)
            #     state = False if mean > 1.0 else True
            #     if(state != isIdle):
            #         isIdle = state
            #         print(mean)
            #         if isIdle:
            #             print(parts[0]+', IDLE\n')
            #         else:
            #             print(parts[0]+', WALKING\n')

        line = fin.readline()
        
        
        