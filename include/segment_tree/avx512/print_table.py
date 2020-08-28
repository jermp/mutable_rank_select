import sys

n = int(sys.argv[1])
val = sys.argv[2] # +1 or -1
for row in range(n+1):
    for col in range(row):
        print("0,", end='')
    for col in range(n - row):
        print(val+",", end='')
    print("")