get_xp() {
  grep -rn "identifier=$1" ./out/*/info.txt|grep -Po "^./out/\w+"|grep -Po '\w+$$'|uniq
}

is_measurement_done() {
  grep -q 'Measurement done' ./out/$1-$2/res/*.csv
}

extract_us() {
  cat ./out/$1-$2/res/*.csv | grep -P "Packet (\d+) latency (\d+)" | perl -pe's/^.*Packet (\d+) latency (\d+).*$/$2,$1/'
}

parse_latency() {
  echo "run,conf,latency,ident"
  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
	  extract_us $r $i | while read l; do 
	    echo $r,$i,$l;
      done
	done
  done
}

parse_thunder() {
  echo "run,jmax,links,latency,ident"
  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
          links=$(grep -Po "thunder-server \d+" out/$r-$i/info.txt|grep -Po "\d+$")
          jmax=$(grep -Po "thunder-server \d+ \d+" out/$r-$i/info.txt|grep -Po "\d+$")
	  is_measurement_done $r $i && extract_us $r $i | while read l; do 
	    echo $r,$jmax,$links,$l;
      done
	done
  done
}

parse_thunder_bw() {
  echo "run,jmax,links,udp_sent,udp_rcv,cells_sent,cells_rcv"
  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
      links=$(grep -Po "thunder-server \d+" out/$r-$i/info.txt|grep -Po "\d+$")
      jmax=$(grep -Po "thunder-server \d+ \d+" out/$r-$i/info.txt|grep -Po "\d+$")
      udp_sent=$(grep -Po "udp_sent: \d+" out/$r-$i/log/client-donar-stdout.log|grep -Po "\d+$")
      udp_rcv=$(grep -Po "udp_rcv: \d+" out/$r-$i/log/client-donar-stdout.log|grep -Po "\d+$")
      cells_sent=$(grep -Po "cells_sent: \d+" out/$r-$i/log/client-donar-stdout.log|grep -Po "\d+$")
      cells_rcv=$(grep -Po "cells_rcv: \d+" out/$r-$i/log/client-donar-stdout.log|grep -Po "\d+$")
      [ -n "$udp_sent" ] && [ -n "$udp_rcv" ] && [ -n "$cells_sent" ] && [ -n "$cells_rcv" ] && \
        echo "$r,$jmax,$links,$udp_sent,$udp_rcv,$cells_sent,$cells_rcv"
	done
  done
}

parse_thunder_links() {
  base_name=$3
  inited=""

  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
      links=$(grep -Po "thunder-server \d+" out/$r-$i/info.txt|grep -Po "\d+$")
      jmax=$(grep -Po "thunder-server \d+ \d+" out/$r-$i/info.txt|grep -Po "\d+$")
      output="${base_name}_${links}_${jmax}.csv"
      
      [ -n "$inited" ] \
        || echo "run,links,jmax,ts,link_id,status,delta,duration,will_change,xp_time,durations_global,will_change_global" > $output
      cat out/$r-$i/log/client-donar-stdout.log | ./links_parse.py "$r-$i,$links,$jmax" >> $output || true
    done
    inited="done"
  done
}

parse_thunder_drop() {
  echo "run,packet_range,count"
  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
      cat out/$r-$i/res/thunder.csv | ./drop_parse.py 990 9900 $r-$i || true
    done
  done
}

parse_thunder_drop_burst() {
  echo "run,prev,cur,count"
  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
      cat out/$r-$i/res/thunder.csv  | ./drop_burst_parse.py $r-$i || true
    done
  done
}

parse_thunder_red() {
  echo "run,delivered_at_once,occur"
  get_xp $1 | while read r; do 
    for i in $(seq 0 1 $2); do 
      grep -Po "Delivered \d+ packets" out/$r-$i/log/client-donar-stdout.log | grep -Po "\d+"|sort|uniq -c|perl -pe"s/^\s*(\d+)\s+(\d+)$/$r-$i,\$2,\$1/"
    done
  done
}
