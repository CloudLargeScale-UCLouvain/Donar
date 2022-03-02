#!/usr/bin/python3

import sys,re

print('authenticate ""')
input()
print('setevents circ')
input()

with open('better_logs.txt', 'w') as f:
  for line in sys.stdin:
    r = re.search(r'650 CIRC (\d+) (\S+) (\S+) BUILD_FLAGS=(\S+) PURPOSE=(\S+) HS_STATE=(\S+)( REND_QUERY=(\S+))? TIME_CREATED=(\S+)( REASON=(\S+))?', line)
    if not r: continue
    circ_id, circ_status, circ_relay_list, build_flags, purpose, hs_state, _, rend_query, time_created, _, reason= r.groups()
    f.write(f"{circ_id},{circ_status},{purpose},{hs_state},{rend_query},{reason}\n")
    f.flush()
