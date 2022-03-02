// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <errno.h>
#include "net_tools.h"
#include "evt_core.h"
#include "packet.h"
#include "tor_ctl.h"

struct donar_params {
  int opt, is_server, is_client, is_waiting_bootstrap, errored, verbose, links, fresh_data, redundant_data, base_port;
  char *bound_ip, *port, *onion_file, *algo, *capture_file, *algo_specific_params, tor_ip[16], my_ip_for_tor[16], *tor_port;
  GPtrArray *remote_ports, *exposed_ports;
  enum TOR_ONION_FLAGS tof;
};

void signal_init(struct evt_core_ctx* evts);
void init_udp_remote(char* port, struct evt_core_ctx* evts);
void init_udp_exposed(char *bound_ip, char* port, struct evt_core_ctx* evts);
void donar_init_params(struct donar_params* dp);
