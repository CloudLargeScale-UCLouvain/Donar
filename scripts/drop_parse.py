#!/usr/bin/python3

import sys,re,math

group_by = int(sys.argv[1])
total = int(sys.argv[2])
run = sys.argv[3]
prev = 0
bins = [0] * (total // group_by)

for line in sys.stdin:
  res = re.match(r".*Packet (\d+) latency.*", line)
  if not res: continue
  pkt_id, = res.groups()
  pkt_id = int(pkt_id) - 1
  for missing in range(prev+1,pkt_id):
    idx = missing // group_by
    if idx >= len(bins): continue
    bins[idx] += 1
  prev = pkt_id

for i in range(len(bins)):
  print(f"{run},{i*group_by}-{(i+1)*group_by-1},{bins[i]}")
