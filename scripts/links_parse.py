#!/usr/bin/python3

import sys,re

acc = []
previous=None
for line in sys.stdin:
  res = re.match(r"\[(\d+)\] Blacklisted links: ([_U]+)", line)
  if not res: continue
  ts,l = res.groups()
  ts = int(ts)
  l2 = [(i, 'up' if l[i] == "U" else 'down') for i in range(len(l))]
  acc.append((ts,l2))

l2 = None
if len(acc) <= 0: sys.exit(0)

ts_first, l_first = acc[0]
ts_last, l_last = acc[len(acc) - 1]
xp_time = ts_last - ts_first

durations = [0 for i,v in l_first]
durations_link_config = 0

for j in range(len(acc)):
  ts, l = acc[j]

  ts_next, l_next = acc[j]
  if j+1 < len(acc): ts_next, l_next = acc[j+1]

  delta = ts_next - ts

  durations_link_config += delta
  will_change_duration = l != l_next

  for i,v in l:
    durations[i] += delta
    i_next,v_next = l_next[i]
    will_change = v != v_next or j+1 == len(acc)
    print(f"{sys.argv[1]},{ts},{i},{v},{delta},{durations[i]},{will_change},{xp_time},{durations_link_config},{will_change_duration}")
    if will_change: durations[i] = 0
  if will_change_duration: durations_link_config = 0
  
