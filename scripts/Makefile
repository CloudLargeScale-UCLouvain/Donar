SHELL := /bin/bash

lightning-base:
	./run-3 \
		1,lightning_base. \
		lightning-client 3000 40 172 8 tick_tock=1!measlat=1. \
		lightning-server 3000 40 172 8 tick_tock=1!measlat=1. \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple

lightning-scheduler-effect:
	./run-3 \
		32,lightning_base. \
		lightning-client 7500 40 172 16 disable_scheduler=0!tick_tock=1!measlat=1, \
		  lightning-client 7500 40 172 16 disable_scheduler=1!tick_tock=1!measlat=1!fast_count=8, \
		  lightning-client 7500 40 172 16 disable_scheduler=0!tick_tock=0!measlat=1, \
		  lightning-client 7500 40 172 16 disable_scheduler=1!tick_tock=0!measlat=1!fast_count=8, \
		  lightning-client 7500 40 172 16 disable_scheduler=0!tick_tock=1!measlat=1, \
		  lightning-client 7500 40 172 16 disable_scheduler=1!tick_tock=1!measlat=1!fast_count=8, \
		  lightning-client 7500 40 172 16 disable_scheduler=0!tick_tock=0!measlat=1, \
		  lightning-client 7500 40 172 16 disable_scheduler=1!tick_tock=0!measlat=1!fast_count=8. \
		lightning-server 7500 40 172 16 disable_scheduler=0!tick_tock=1!measlat=1, \
		  lightning-server 7500 40 172 16 disable_scheduler=1!tick_tock=1!measlat=1!fast_count=8, \
		  lightning-server 7500 40 172 16 disable_scheduler=0!tick_tock=0!measlat=1, \
		  lightning-server 7500 40 172 16 disable_scheduler=1!tick_tock=0!measlat=1!fast_count=8, \
		  lightning-server 7500 40 172 16 disable_scheduler=0!tick_tock=1!measlat=1, \
		  lightning-server 7500 40 172 16 disable_scheduler=1!tick_tock=1!measlat=1!fast_count=8, \
		  lightning-server 7500 40 172 16 disable_scheduler=0!tick_tock=0!measlat=1, \
		  lightning-server 7500 40 172 16 disable_scheduler=1!tick_tock=0!measlat=1!fast_count=8. \
		tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple, \
		  tor2 -f /etc/torrc_simple.

tor2-pkt-per-sec:
	./run-3 \
		32,tor2_pkt_per_sec. \
		orig-client 60 1000 172, \
		orig-client 120 500 172, \
		orig-client 240 250 172, \
		orig-client 360 166 172, \
		orig-client 480 125 172, \
		orig-client 600 100 172, \
		orig-client 722  83 172, \
		orig-client 845  71 172, \
		orig-client 952  63 172, \
		orig-client 1071 56 172, \
		orig-client 1200 50 172, \
		orig-client 1333 45 172, \
		orig-client 1463 41 172, \
		orig-client 1579 38 172, \
		orig-client 1714 35 172. \
		orig-server 60 1000 172, \
		orig-server 120 500 172, \
		orig-server 240 250 172, \
		orig-server 360 166 172, \
		orig-server 480 125 172, \
		orig-server 600 100 172, \
		orig-server 722  83 172, \
		orig-server 845  71 172, \
		orig-server 952  63 172, \
		orig-server 1071 56 172, \
		orig-server 1200 50 172, \
		orig-server 1333 45 172, \
		orig-server 1463 41 172, \
		orig-server 1579 38 172, \
		orig-server 1714 35 172. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple.

lightning-window:
	./run-3 \
		64,lightning_window. \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=200, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=1000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=3000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=5000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=8000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=13000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=34000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=200, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=1000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=3000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=5000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=8000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=13000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=34000. \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=200, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=1000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=3000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=5000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=8000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=13000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=34000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=200, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=1000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=3000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=5000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=8000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=13000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=34000. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple.

lightning-links:
	./run-3 \
		64,lightning_links. \
		lightning-client 7500 40 172 6 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 8 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 10 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 14 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 16 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 20 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 6 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 8 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 10 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 14 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 16 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 20 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		lightning-server 7500 40 172 6 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 8 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 10 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 14 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 16 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 20 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 6 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 8 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 10 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 14 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 16 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 20 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple.

lightning-complementarity:
	./run-3 \
		64,lightning_complementarity. \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 350 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 350 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 350 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 350 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 350 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 350 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 350 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 disable_scheduler=1!fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 350 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple. \

battle:
	./run-3 \
		64,battle. \
		orig-client 7500 40 172, \
		orig-client 7500 40 172, \
		orig-client 7500 40 172, \
		dup2-client 7500 40 172 2, \
		dup2-client 7500 40 172 2, \
		dup2-client 7500 40 172 2, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=8!tick_tock=1!measlat=1!window=300000, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=8!tick_tock=1!measlat=1!window=300000, \
		lightning-client 7500 40 172 12 disable_scheduler=1!fast_count=8!tick_tock=1!measlat=1!window=300000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		orig-server-single 7500 40 172, \
		orig-server 7500 40 172, \
		orig-server 7500 40 172, \
		dup2-server-single 7500 40 172 2, \
		dup2-server 7500 40 172 2, \
		dup2-server 7500 40 172 2, \
		lightning-server-single 7500 40 172 12 disable_scheduler=1!fast_count=8!tick_tock=1!measlat=1!window=300000, \
		lightning-server 7500 40 172 12 disable_scheduler=1!fast_count=8!tick_tock=1!measlat=1!window=300000, \
		lightning-server 7500 40 172 12 disable_scheduler=1!fast_count=8!tick_tock=1!measlat=1!window=300000, \
		lightning-server-single 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server-single 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_guard_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_guard_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_guard_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_guard_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_guard_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2. \
		tor2 -f /etc/torrc_single_hop_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_single_hop_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_single_hop_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_single_hop_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2, \
		tor2 -f /etc/torrc_single_hop_16, \
		tor2 -f /etc/torrc_guard_16, \
		tor3 -f /etc/torrc_guard_2.

lightning-fastcount:
	./run-3 \
		64,lightning_fastcount. \
		lightning-client 7500 40 172 12 fast_count=1!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=2!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=4!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=5!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=6!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=1!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=2!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=4!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=5!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=6!tick_tock=0!measlat=1!window=2000. \
		lightning-server 7500 40 172 12 fast_count=1!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=2!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=4!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=5!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=6!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=1!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=2!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=4!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=5!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=6!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_simple.

lightning-guards:
	./run-3 \
		64,lightning_guards. \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 7500 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_guard_1, \
		tor2 -f /etc/torrc_guard_3, \
		tor2 -f /etc/torrc_guard_5, \
		tor2 -f /etc/torrc_guard_7, \
		tor2 -f /etc/torrc_guard_9, \
		tor2 -f /etc/torrc_guard_11, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_guard_1, \
		tor2 -f /etc/torrc_guard_3, \
		tor2 -f /etc/torrc_guard_5, \
		tor2 -f /etc/torrc_guard_7, \
		tor2 -f /etc/torrc_guard_9, \
		tor2 -f /etc/torrc_guard_11, \
		tor2 -f /etc/torrc_simple. \
		tor2 -f /etc/torrc_guard_1, \
		tor2 -f /etc/torrc_guard_3, \
		tor2 -f /etc/torrc_guard_5, \
		tor2 -f /etc/torrc_guard_7, \
		tor2 -f /etc/torrc_guard_9, \
		tor2 -f /etc/torrc_guard_11, \
		tor2 -f /etc/torrc_simple, \
		tor2 -f /etc/torrc_guard_1, \
		tor2 -f /etc/torrc_guard_3, \
		tor2 -f /etc/torrc_guard_5, \
		tor2 -f /etc/torrc_guard_7, \
		tor2 -f /etc/torrc_guard_9, \
		tor2 -f /etc/torrc_guard_11, \
		tor2 -f /etc/torrc_simple.


jan_tor_analysis:
	./run-3 \
		8,tor_analysis. \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172, \
		orig-client 135000 40 172, \
		orig-client 113400 48 172, \
		orig-client 91800 59 172, \
		orig-client 70800 77 172, \
		orig-client 48600 111 172, \
		orig-client 27000 200 172, \
		orig-client 5400 1000 172. \
		\
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172, \
		orig-server 135000 40 172, \
		orig-server 113400 48 172, \
		orig-server 91800 59 172, \
		orig-server 70800 77 172, \
		orig-server 48600 111 172, \
		orig-server 27000 200 172, \
		orig-server 5400 1000 172. \
		\
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1. \
		\
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1, \
		tor3 -f /etc/torrc_guard_1.

jan_battle_normal:
	./run-3 \
		32,jan_battle_normal. \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		orig-server 135000 40 172, \
		orig-server 135000 40 172, \
		orig-server 135000 40 172, \
		orig-server 135000 40 172, \
		dup2-server 135000 40 172 2, \
		dup2-server 135000 40 172 2, \
		dup2-server 135000 40 172 2, \
		dup2-server 135000 40 172 2, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12. \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12.

jan_battle_hardened:
	./run-3 \
		32,jan_battle_hardened. \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		orig-server 135000 40 172, \
		orig-server 135000 40 172, \
		orig-server 135000 40 172, \
		orig-server 135000 40 172, \
		dup2-server 135000 40 172 2, \
		dup2-server 135000 40 172 2, \
		dup2-server 135000 40 172 2, \
		dup2-server 135000 40 172 2, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2. \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2.
	
jan_battle_light:
	./run-3 \
		32,jan_battle_light. \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		orig-client 135000 40 172, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		dup2-client 135000 40 172 2, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-client 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		orig-server-single 135000 40 172, \
		orig-server-single 135000 40 172, \
		orig-server-single 135000 40 172, \
		orig-server-single 135000 40 172, \
		dup2-server-single 135000 40 172 2, \
		dup2-server-single 135000 40 172 2, \
		dup2-server-single 135000 40 172 2, \
		dup2-server-single 135000 40 172 2, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=1!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000, \
		lightning-server-single 135000 40 172 12 fast_count=3!tick_tock=0!measlat=1!window=2000. \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12. \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12.


jan_dcall_normal:
	./run-3 \
		32,jan_dcall_normal. \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196. \
		dcall-simple-server 1 360, \
		dcall-simple-server 1 360, \
		dcall-simple-server 1 360, \
		dcall-simple-server 1 360, \
		dcall-dup2-server 2 360, \
		dcall-dup2-server 2 360, \
		dcall-dup2-server 2 360, \
		dcall-dup2-server 2 360, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 233, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 196. \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12. \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12.

jan_dcall_hardened:
	./run-3 \
		32,jan_dcall_hardened. \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331. \
		dcall-simple-server 1 360, \
		dcall-simple-server 1 360, \
		dcall-simple-server 1 360, \
		dcall-simple-server 1 360, \
		dcall-dup2-server 2 360, \
		dcall-dup2-server 2 360, \
		dcall-dup2-server 2 360, \
		dcall-dup2-server 2 360, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-server 12 fast_count=3!tick_tock=1!measlat=0!window=2000 360, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331, \
		dcall-lightning-server 12 fast_count=3!tick_tock=0!measlat=0!window=2000 331. \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2. \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2, \
		tor3 -f /etc/torrc_guard_2.

jan_dcall_light:
	./run-3 \
		32,jan_dcall_light. \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-simple-client 1 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-dup2-client 2 360, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-client 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173, \
		dcall-lightning-client 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173. \
		dcall-simple-server-single 1 360, \
		dcall-simple-server-single 1 360, \
		dcall-simple-server-single 1 360, \
		dcall-simple-server-single 1 360, \
		dcall-dup2-server-single 2 360, \
		dcall-dup2-server-single 2 360, \
		dcall-dup2-server-single 2 360, \
		dcall-dup2-server-single 2 360, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=1!measlat=0!window=2000 218, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173, \
		dcall-lightning-server-single 12 fast_count=3!tick_tock=0!measlat=0!window=2000 173. \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12, \
		tor2 -f /etc/torrc_guard_12. \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12, \
		tor2 -f /etc/torrc_single_hop_12.
