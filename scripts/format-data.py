#!/usr/bin/python3
import sys

xp_id = sys.argv[1]
values = []
for line in sys.stdin:
    values.append(int(line))

values = sorted(values)
lval = len(values)
if (len(values) == 0):
    print("No values for "+xp_id, file=sys.stderr)
    sys.exit(0)
print(xp_id+",min,"+str(values[0]))
print(xp_id+",5th,"+str(values[int(5/100*lval)]))
print(xp_id+",25th,"+str(values[int(25/100*lval)]))
print(xp_id+",med,"+str(values[int(50/100*lval)]))
print(xp_id+",75th,"+str(values[int(75/100*lval)]))
print(xp_id+",95th,"+str(values[int(95/100*lval)]))
print(xp_id+",max,"+str(values[lval-1]))
