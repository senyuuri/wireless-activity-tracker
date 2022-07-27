def process_light_data():
    now_time = datetime.now()
    if(light_utime is None or int((now_time - light_utime).total_seconds()) >= 10):
        readings = numpy.array([baro_data[-1][1], temp_data[-1][1], light_data[-1][1], humid_data[-1][1]])
        # using Bernoulli Naive Bayes model to predict indoor/outdoor status
        result = bool(bnb.predict(readings)[0])
        if result != indoor:
            # update state change
            indoor = result
            indoor_utime = now_time

            if(indoor == True):
                print(str(light_data[-1][0]) + ', INDOOR\n')
            else:
                print(str(light_data[-1][0]) + ', OUTDOOR\n')