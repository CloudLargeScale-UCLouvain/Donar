#!/usr/bin/python3

import sys, re

current_run="none"
d = {}
input()
for line in sys.stdin:
  packet_id,latency,flag,link_id,vanilla,way,run,conf = line.split(",")
  conf = conf.rstrip()
  if int(packet_id) > 7400 or int(packet_id) < 10: continue
  latency = int(latency)
  
  if conf not in d: d[conf] = {}
  if (run,way,packet_id) not in d[conf]:
    d[conf][(run,way,packet_id)] = latency
  else:
    d[conf][(run,way,packet_id)] = min(latency, d[conf][(run,way,packet_id)])

#print("strat,window,decile,latency")
print("secmode,algo,percentile,latency")
for conf,value in d.items():
  s = sorted(value.values())
  szs = len(s)

  secmode, algo = "bug", "bug"
  if "tor3 " in conf: secmode = "hardened"
  elif "_single_" in conf: secmode = "light"
  elif "tor2 " in conf: secmode = "default"

  if "orig-client" in conf: algo = "simple"
  elif "dup2" in conf: algo = "dup2"
  elif "lightning" in conf and "tick_tock=1" in conf: algo = "lightning-ticktock"
  elif "lightning" in conf and "tick_tock=0" in conf: algo = "lightning-dup"

  #m = re.match(r".*tick_tock=(\d+).+window=(\d+)", conf)
  #m = re.match(r".*fast_count=(\d+)!tick_tock=(\d+)", conf)
  #m = re.match(r"^(\d+).*tick_tock=(\d+)", conf)
  print(f"{secmode},{algo},MIN,{s[0]}")
  print(f"{secmode},{algo},P0.1,{s[int(szs*0.001)]}")
  print(f"{secmode},{algo},P1,{s[int(szs*0.01)]}")
  print(f"{secmode},{algo},P25,{s[int(szs*0.25)]}")
  print(f"{secmode},{algo},P50,{s[int(szs*0.5)]}")
  print(f"{secmode},{algo},P75,{s[int(szs*0.75)]}")
  print(f"{secmode},{algo},P99,{s[int(szs*0.99)]}")
  print(f"{secmode},{algo},P99.9,{s[int(szs*0.999)]}")
  print(f"{secmode},{algo},MAX,{s[-1]}")
