#!/usr/bin/python3
# Packet 7495 latency 153496µs with flag 0 sent on link 6 with vanilla 0

import sys,re

groupmap = {"0": "fast", "1": "probe"}
redmap = {"1": "orig", "0": "pad"}
res = {}
seen = set()
vals = 0

for line in sys.stdin:
    if "~ measlat ~" in line: 
      seen = set()
      continue
    m = re.match(r".*Packet (\d+) latency (\d+)µs with flag (\d) sent on link (\d+) with vanilla (\d)", line)
    if not m: continue
    if m[1] in seen: continue
    seen.add(m[1])
    group = groupmap[m[3]]
    red = redmap[m[5]]
    if (group, red) not in res: res[(group,red)] = 0
    res[(group,red)] += 1
    vals += 1

print("group,redundancy,count")
for key,val in res.items():
    g,r = key
    print(f"{g},{r},{val/vals}")
