with open('10118.csv', 'r') as fopen:
	with open('received_data.txt', 'w') as fout:
		for line in fopen.readlines():
			raw = line.split(',')[3][:-5]
			parts = [raw[i:i+2] for i in range(0, len(raw), 2)]
			fout.write(','.join(parts)+'\n')