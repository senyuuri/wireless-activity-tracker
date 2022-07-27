with open('send_data_raw.txt', 'r') as fopen:
	with open('transmitted_data.txt', 'w') as fout:
		for line in fopen.readlines():
			parts = ['{0:02d}'.format(int(x)) for x in line.split(',')[:-1]]
			fout.write(','.join(parts)+'\n')