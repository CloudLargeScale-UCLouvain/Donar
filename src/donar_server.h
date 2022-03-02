// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>
#include <gmodule.h>
#include "socks5.h"
#include "tor_os.h"
#include "tor_ctl.h"
#include "evt_core.h"
#include "donar_init.h"
#include "proxy.h"
#include "timer.h"

#define PORT_SIZE 64

struct donar_server_ctx {
  struct tor_os_str tos;
  struct tor_ctl tctl;
  struct evt_core_ctx evts;
  uint16_t ports[PORT_SIZE];
};

void donar_server(struct donar_server_ctx* ctx, struct donar_params* dp);
