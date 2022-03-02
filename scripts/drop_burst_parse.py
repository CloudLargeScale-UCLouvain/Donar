#!/usr/bin/python3

import sys,re,math

run = sys.argv[1]
prev = 0

for line in sys.stdin:
  res = re.match(r".*Packet (\d+) latency.*", line)
  if not res: continue
  pkt_id, = res.groups()
  pkt_id = int(pkt_id) - 1
  dropped = pkt_id - (prev+1)
  if dropped > 0:
    print(f"{run},{prev},{pkt_id},{dropped}")
  prev = pkt_id
