#!/usr/bin/python3
import sys,re

way=sys.argv[1]
for line in sys.stdin:
  res = re.match(r".*Packet (\d+) latency (\d+)µs with flag (\d+) sent on link (\d+) with vanilla (\d+).*", line)
  if not res: continue
  pid,lat,flag,link,vanilla = res.groups()
  print(f"{pid},{lat},{flag},{link},{vanilla},{way}")
