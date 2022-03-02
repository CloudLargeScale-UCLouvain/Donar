#!/usr/bin/python3
import sys,os,functools,re
import datetime

convert = {
  "dcall-lightning-server-single": "donar",
  "dcall-lightning-server": "donar",
  "dcall-dup2-server-single": "torfone",
  "dcall-dup2-server": "torfone",
  "dcall-simple-server-single": "simple",
  "dcall-simple-server": "simple"
}

def extract_info(inf, s):
  try:
    with open(inf) as f:
      full = ''.join(f.readlines())
      w = re.search(r'identifier=jan_dcall_(\w+)', full)
      if not w: return False
      s['current']['mode'] = w.groups()[0]
      x = re.search(r'server= (\S+) \d+ ((\S+) (\d+))?', full)
      if x:
        s['current']['strat'] = convert[x.groups()[0]]
        if x.groups()[2] != None:
          y = re.search(r'tick_tock=(\d)', x.groups()[2])
          if y:
            s['current']['strat'] +=  "-alternate" if y.groups()[0] == '1' else "-double-send"
        return True
      else:
        print("parse error for",inf)
        return False
  except Exception as e:
    print("read error", inf, e)
    return False

def categorize(f, s):
  return True

def do_new_parse(line, s):
  if 'Using buffer of size 160' not in line: return True
  try:
    current_time = datetime.datetime.strptime(line[:14], "%H:%M:%S.%f")
    if 'start' not in s['current']: 
        s['current']['start'] = current_time
    if 'dlvbuf' not in s['current']:
      s['current']['dlvbuf'] = 0
    if 'delta_list' not in s['current']:
      s['current']['delta_list'] = []
    s['current']['dlvbuf'] += 1
    if 'last' in s['current']:
      delta = (current_time - s['current']['last']) / datetime.timedelta(milliseconds=1)
      s['current']['delta_list'].append(delta)
      #if delta > 1000:
      #  print('packet',s['current']['dlvbuf'],'has been delivered after',delta,'ms of inactivity')
    s['current']['last'] = current_time
  except Exception as e:
    print(e)
    pass
  return True

def extract_dcall(f, s):
  #s['current']['success_buf'] = 0
  #s['current']['failed_buf'] = 0
  try:
    with open(f) as f:
      for line in f:
        do_new_parse(line, s)

        #time = line.split()[0]
        #minutes = int(time.split(':')[1])
        #if minutes < 5 or minutes > 10: continue
        #if 'Using buffer of size 160' in line:
        #  s['current']['success_buf'] += 1
        #elif 'Using NULL buffer' in line:
        #  s['current']['failed_buf'] += 1
    #print(s['current'])
    return True
  except Exception as e:
    print("read error", f, e)
    return False

def aggregate_failed_pct(s):
  try:
    k = (s['current']['strat'], s['current']['mode'])
    if k not in s['failed_5pct']: s['failed_5pct'][k] = 0
    if k not in s['failed_1pct']: s['failed_1pct'][k] = 0

    s['current']['duration'] = (s['current']['last'] - s['current']['start']) / datetime.timedelta(seconds=1)
    s['current']['durmin'] = s['current']['duration'] / 60
    expected = s['current']['duration'] * 25
    s['current']['expected'] = expected
    s['current']['threshold'] = expected * 0.99
    s['current']['effective'] = s['current']['dlvbuf'] / s['current']['expected']
    s['current']['avg_delta'] = sum(s['current']['delta_list']) / len(s['current']['delta_list'])

    print(s['current']['identifier'], k, s['current']['effective'], s['current']['avg_delta'], min(s['current']['delta_list']), max(s['current']['delta_list']))

    if s['current']['dlvbuf'] < expected * 0.99: 
      s['failed_1pct'][k] += 1
 # except Exception as e:
 #   print('err', e)
 #   pass
  #if k not in s['failed_1pct']: s['failed_1pct'][k] = 0
  #if s['current']['success_buf'] < 7500 * 0.99: 
  #  s['failed_1pct'][k] += 1
  #  print(s['current'])

  #if k not in s['failed_5pct']: s['failed_5pct'][k] = 0
  #if s['current']['success_buf'] < 7500 * 0.95: s['failed_5pct'][k] += 1
  
  #if k not in s['run_per_strat']: s['run_per_strat'][k] = 0
  #s['run_per_strat'][k] += 1

  except Exception as e:
    print(e)

  return True

def extract_folder(f, s):
  return \
    extract_info(f + '/info.txt', s) and \
    extract_dcall(f + '/log/server-dcall-gstreamer.log', s) and \
    aggregate_failed_pct(s)

def extract(p, s):
  item_count = functools.reduce(lambda acc, prev: acc + 1, os.listdir(p), 0)

  counter = 0
  print("extracting...")
  for folder in os.listdir(p):
    s['current'] = { 'identifier': folder}
    True and \
      extract_folder(p + '/' + folder, s) and \
      categorize(folder, s) or \
      print(f"An error occured with {folder}")
    counter += 1
    progress = round(counter / item_count * 100)
    print(f"{progress}%", end="\r")
  print("done")

def output_res(s):
  with open('dcall_drop.csv', 'w') as f:
    f.write("mode,strat,threshold,ratio\n")
    for thr_name, thr_val in [('failed_1pct', 0.01), ('failed_5pct', 0.05)]:
      for key,val in s[thr_name].items():
        mode, strat = key
        ratio = val / s['run_per_strat'][key]
        f.write(f"{mode},{strat},{thr_val},{ratio}\n")

state = {'failed_1pct': {}, 'failed_5pct': {}, 'run_per_strat': {}}
extract(sys.argv[1], state)
#output_res(state)

