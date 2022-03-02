#!/usr/bin/python3
import os,sys,re,functools

default_perc = [0, 0.25, 0.5, 0.75, 0.99, 0.999, 1]

def tool_distri(arr, perc):
  r = {}
  for p in perc:
    r[str(p)] = arr[round(p * (len(arr) - 1))]
  return r

def compute_dropped(s):
  s['current']['missing'] = []
  start = 0
  state = "BROKEN"
  for missing_pkt in range(s['current']['npkt']+1):
    if s['current']['crible'][missing_pkt] and state == 'WORKING':
      state = 'BROKEN'
      start = missing_pkt
    elif (not s['current']['crible'][missing_pkt]) and state == 'BROKEN':
      state = 'WORKING'
      s['current']['missing'].append((start, missing_pkt-1,missing_pkt-start))

  if state == 'BROKEN':
    s['current']['missing'].append((start, s['current']['npkt'], 1+s['current']['npkt']-start))
 
  t = s['current']['missing']
  # Don't consider first 10 seconds are links are still being connected
  t = filter(lambda p: (lambda start, stop, diff: start > 250)(*p), t) 
  # Below 1 second continuous drop, don't consider the call dropped
  t = filter(lambda p: (lambda start, stop, diff: diff > 25)(*p), t)
  # Don't consider the last drop as it is already parsed elsewhere
  t = filter(lambda p: (lambda start, stop, diff: stop != s['current']['npkt'])(*p), t)
  t = [x for x in t]
  if len(t) > 0:
    print(s['current']['identifier'])
    print(t)
    print('old max pkt', s['current']['max_pkt'])
    s['current']['max_pkt'] = min(s['current']['max_pkt'], t[0][0] - 1)
    print('new max pkt', s['current']['max_pkt'])
  return True

def compute_failure(s):
  it = (s['current']['strat'], s['current']['mode'])
  if it not in s['failure']: s['failure'][it] = []
  s['failure'][it].append(round(s['current']['max_pkt'] * s['current']['interval'] / 1000 / 60))
  return True

def extract_measlat(log, s):
  s['current']['max_pkt'] = 0
  s['current']['crible'] = [True] * (s['current']['npkt']+1)
  try:
    with open(log) as f:
      for l in f:
        x = re.search(r'Packet (\d+) latency (\d+)µs with', l)
        if x:
          pkt = int(x.groups()[0])
          lat = int(x.groups()[1])
          s['current']['max_pkt'] = max(s['current']['max_pkt'], pkt)
          if pkt <= s['current']['npkt']: s['current']['crible'][pkt] = False
    return True
  except Exception as e:
    print("read error", e)
    return False

def extract_info(inf, s):
  try:
    with open(inf) as f:
      full = ''.join(f.readlines())
      w = re.search(r'identifier=jan_battle_(\w+)', full)
      if not w: return False
      s['current']['mode'] = w.groups()[0]
      x = re.search(r'server= (\S+) (\d+) (\d+) \d+ (\d+ (\S+))?', full)
      if x:
        s['current']['strat'] = x.groups()[0]
        if x.groups()[4] != None:
          y = re.search(r'tick_tock=(\d)', x.groups()[4])
          if y:
            s['current']['strat'] +=  "-ticktock" if y.groups()[0] == '1' else "-duplicate"
        s['current']['npkt'] = int(x.groups()[1])
        s['current']['interval'] = int(x.groups()[2])
        return True
      else:
        print("parse error for",inf)
        return False
  except Exception as e:
    print("read error", inf, e)
    return False

def extract_folder(p, s):
  return \
    extract_info(p + '/info.txt', s) and \
    extract_measlat(p + '/log/client-measlat-stdout.log', s) and \
    compute_dropped(s) and \
    compute_failure(s)

def categorize(folder, s):
  s[folder] = s['current']

  i = (s['current']['strat'], s['current']['mode'])
  if i not in s['per_strat']: s['per_strat'][i] = []
  s['per_strat'][i].append(s['current'])
  return True

def extract(p, s):
  item_count = functools.reduce(lambda acc, prev: acc + 1, os.listdir(p), 0)

  counter = 0
  print("extracting...")
  for folder in os.listdir(p):
    s['current'] = { 'identifier': folder}
    extract_folder(p + '/' + folder, s) and \
      categorize(folder, s) or \
      print(f"An error occured with {folder}")
    counter += 1
    progress = round(counter / item_count * 100)
    print(f"{progress}%", end="\r")
  print("done")

def analyze_failure(s):
  with open('jan2_failure.csv', 'w') as f:
    f.write(f"strat,duration,ecdf\n")
    for k, v in s['failure'].items():
      strat, mode = k
      v = sorted(v)
      total = len(v)
      score = 0
      f.write(f"{mode},{strat},0,0\n")
      for idx,e in enumerate(v,start=1):
        if e >= 90: 
          f.write(f"{mode},{strat},90,{score}\n")
          break
        score = idx/total
        f.write(f"{mode},{strat},{e},{score}\n")

def analyze(s):
  print("analyzing...")
  analyze_failure(s)

state = {'failure': {}, 'per_strat': {}, 'per_interval_res': {}, 'per_circuit_res': []}
extract(sys.argv[1], state)
analyze(state)
