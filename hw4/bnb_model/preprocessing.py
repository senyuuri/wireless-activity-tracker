filename = 'walk_4/data_io1.csv'

a = []
b = -1
t = -1
l = -1
h = -1

result = []

with open(filename, 'r') as fopen:
    raw = fopen.readline()
    while raw != '':
        line = raw.split(',')

        if line[1] == 'a':
            a = [x.replace('\n','') for x in line[2:]]
        elif line[1] == 'b':
            b = line[2]
        elif line[1] == 't':
            t = line[2]
        elif line[1] == 'l':
            l = line[2]
        elif line[1] == 'h':
            h = line[2]

        if (b != -1) and (t != -1) and (l != -1) and (h != -1):
            result.append([line[0], a[0], a[1], a[2], b, t, l, h])
        raw = fopen.readline()

with open('walk_4/testing1.csv', 'w') as fout:
    for r in result:
        print(r)
        row = ','.join(r)
        fout.write(row+'\n')