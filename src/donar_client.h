// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>
#include "tor_os.h"
#include "socks5.h"
#include "proxy.h"
#include "donar_init.h"
#include "timer.h"

#define CLIENT_PORT_SIZE 64

struct donar_client_ctx {
  struct tor_os_str tos;
  struct evt_core_ctx evts;
  char *tor_ip;
  char *tor_port;
  uint16_t base_port;
  uint16_t ports[CLIENT_PORT_SIZE];
  struct {
    int fd;
    enum socks5_state state;
  } client_sock[CLIENT_PORT_SIZE];
};

void donar_client(struct donar_client_ctx* ctx, struct donar_params* dp);
