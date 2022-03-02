#!/usr/bin/python3
import sys,re

print("run,packet_id,measure_type,latency,latency_ms")
for i in range(1,len(sys.argv)):
  path = sys.argv[i]
  client_server = f"{sys.argv[i]}/log/server-measlat-stdout.log"
  server_client = f"{sys.argv[i]}/log/client-measlat-stdout.log"
  run = path

  agg = {}
  with open(client_server, 'r') as f:
    for line in f:
      res = re.match(r".*Packet (\d+) latency (\d+)µs", line)
      if not res: continue
      pkt_id,lat = res.groups()
      pkt_id,lat = int(pkt_id), int(lat)
      agg[pkt_id] = {"client_server": lat }

  with open(server_client, 'r') as f:
    for line in f:
      res = re.match(r".*Packet (\d+) latency (\d+)µs", line)
      if not res: continue
      pkt_id,lat = res.groups()
      pkt_id,lat = int(pkt_id), int(lat)
      if not pkt_id in agg: agg[pkt_id] = {}
      agg[pkt_id]["server_client"] = lat

  for packet_id, lats in agg.items():
    if "client_server" not in lats or "server_client" not in lats: continue
    print(f"{run},{packet_id},client_server,{lats['client_server']},{lats['client_server'] / 1000}")
    print(f"{run},{packet_id},server_client,{lats['server_client']},{lats['server_client'] / 1000}")
    print(f"{run},{packet_id},delta,{lats['client_server'] - lats['server_client']},{(lats['client_server'] - lats['server_client'])/1000}")
    print(f"{run},{packet_id},delta_abs,{abs(lats['client_server'] - lats['server_client'])},{abs(lats['client_server'] - lats['server_client'])/1000}")
    
    if packet_id - 1 not in agg or "client_server" not in agg[packet_id - 1] or "server_client" not in agg[packet_id - 1]: continue
    dcs = lats['client_server'] - agg[packet_id - 1]['client_server']
    dsc = lats['server_client'] - agg[packet_id - 1]['server_client']
    print(f"{run},{packet_id},d_client_server,{dcs},{dcs/1000}")
    print(f"{run},{packet_id},d_server_client,{dsc},{dsc/1000}")
    print(f"{run},{packet_id},d_client_server_abs,{abs(dcs)},{abs(dcs)/1000}")
    print(f"{run},{packet_id},d_server_client_abs,{abs(dsc)},{abs(dsc)/1000}")
