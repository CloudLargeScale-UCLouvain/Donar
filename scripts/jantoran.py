#!/usr/bin/python3
import os,sys,re,functools

default_perc = [0, 0.25, 0.5, 0.75, 0.99, 0.999, 1]

def tool_distri(arr, perc):
  r = {}
  for p in perc:
    r[str(p)] = arr[round(p * (len(arr) - 1))]
  return r

def compute_failure(s):
  it = s['current']['interval']
  if it not in s['failure']: s['failure'][it] = []
  s['failure'][it].append(round(s['current']['max_pkt'] * s['current']['interval'] / 1000 / 60))

def compute_circuit_distri(s):
  l = sorted(s['current']['lats'])
  s['per_circuit_res'].append(tool_distri(l, default_perc))

def compute_interval_distri(s):
  print("  + latency distribution for given packet freq.")
  to_process = len(s['per_interval'])
  processed = 0
  for inter, val in s['per_interval'].items():
    progress = round(processed / to_process * 100)
    print(f"{progress}%", end="\r")
    #print(val[0]['lats'][0:10])
    x = sorted(functools.reduce(lambda acc, v: acc + v['lats'], val, []))
    s['per_interval_res'][inter] = tool_distri(x, default_perc)
    processed += 1

def extract_measlat(log, s):
  s['current']['max_pkt'] = 0
  s['current']['lats'] = []
  with open(log) as f:
    for l in f:
      x = re.search(r'Packet (\d+) latency (\d+)µs with', l)
      if x:
        pkt = int(x.groups()[0])
        lat = int(x.groups()[1])
        s['current']['max_pkt'] = max(s['current']['max_pkt'], pkt)
        s['current']['lats'].append(lat)

def extract_info(inf, s):
  with open(inf) as f:
    full = ''.join(f.readlines())
    x = re.search(r'orig-server (\d+) (\d+) \d+', full)
    if x:
      s['current']['npkt'] = int(x.groups()[0])
      s['current']['interval'] = int(x.groups()[1])
      return True
    else:
      print("read error for",inf)
      return False

def extract_folder(p, s):
  if not extract_info(p + '/info.txt', s): return False
  extract_measlat(p + '/log/client-measlat-stdout.log', s)

  compute_failure(s)
  compute_circuit_distri(s) if s['current']['interval'] == 40 else None

def categorize(folder, s):
  s[folder] = s['current']

  i = str(s['current']['interval'])
  if i not in s['per_interval']: s['per_interval'][i] = []
  s['per_interval'][i].append(s['current'])

def extract(p, s):
  item_count = functools.reduce(lambda acc, prev: acc + 1, os.listdir(p), 0)

  counter = 0
  print("extracting...")
  for folder in os.listdir(p):
    s['current'] = {}
    extract_folder(p + '/' + folder, s) and categorize(folder, s)
    counter += 1
    progress = round(counter / item_count * 100)
    print(f"{progress}%", end="\r")
  print("done")

def compute_global(s):
  print("computing on global values...")
  compute_interval_distri(s)

def analyze_failure(s):
  with open('jan_failure.csv', 'w') as f:
    f.write(f"rate,duration,ecdf\n")
    for k, v in s['failure'].items():
      v = sorted(v)
      total = len(v)
      rate = round(1000 / k)
      score = 0
      f.write(f"{rate},0,0\n")
      for idx,e in enumerate(v,start=1):
        if e >= 90: 
          f.write(f"{rate},90,{score}\n")
          break
        score = idx/total
        f.write(f"{rate},{e},{score}\n")

def analyze_interval(s):
  with open('jan_interval.csv', 'w') as f:
    f.write(f"rate,perc,lat\n")
    for inter, entr in s['per_interval_res'].items():
      rate = round(1000 / int(inter))
      for perc, lat in entr.items():
        f.write(f"{rate},{float(perc)*100}%,{lat/1000}\n")

def analyze_circuit(s):
  a = sorted(s['per_circuit_res'], key=lambda v: v['0.5'])
  with open('jan_circuit_median.csv', 'w') as f:
    f.write(f"id,perc,lat\n")
    for idx,e in enumerate(a,start=1):
      for perc, lat in e.items():
        f.write(f"{idx},{float(perc)*100}%,{lat/1000}\n")

  a = sorted(s['per_circuit_res'], key=lambda v: v['1'])
  with open('jan_circuit_max.csv', 'w') as f:
    f.write(f"id,perc,lat\n")
    for idx,e in enumerate(a,start=1):
      for perc, lat in e.items():
        f.write(f"{idx},{float(perc)*100}%,{lat/1000}\n")

def analyze(s):
  print("analyzing...")
  analyze_failure(s)
  analyze_interval(s)
  analyze_circuit(s)

state = {'failure': {}, 'per_interval': {}, 'per_interval_res': {}, 'per_circuit_res': []}
extract(sys.argv[1], state)
compute_global(state)
analyze(state)

#for key, value in state.items():
#  print(value) 
