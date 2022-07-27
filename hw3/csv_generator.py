import random

for i in range(10000):
	if i == 9999:
		print(str(random.randint(1,200)), end='')
	else:
		print(str(random.randint(1,200))+',', end='')