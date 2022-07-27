"""
Convert stdout readings to csv files

For internal use. NOT to be submitted.
"""
flist = ['1min30sec163steps', '1min30sec164hold', '1min30sec167steps', '1min30sec177hold']

for f in flist:
	with open('out/'+f, 'r') as fin:
		with open('csv2/'+f+'.csv', 'w') as fout:
			lines = fin.readlines()
			for line in lines:
				if 'G' in line:
					parts = line.replace('G','').replace(' ','').split(',')
					new = [str(round(float(x)*9.80, 2)) for x in parts]
					fout.write(','.join(new)+'\n')

flist = ['accel-interval-noiseless.file', 'accel-interval-noisy.file', 'accel-noiseless', 'accel-noisy', 'accel-vnoisy']

for f in flist:
	with open('archive/'+f, 'r') as fin:
		with open('archive_csv/'+f+'.csv', 'w') as fout:
			lines = fin.readlines()
			for line in lines:
				parts = line.split(',')[2:]
				new = [str(round(float(x)/9.80, 2)) for x in parts]
				fout.write(','.join(new)+'\n')

