// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include "tor_os.h"
#include "net_tools.h"
#include "evt_core.h"

/*
 * We want to use fscanf and fprintf as these func provide a nice abstraction
 * to parse text content. However, we must convert our file descriptor to a
 * STREAM*. It appears that we have to create 2 streams (cf link). Moreover
 * we have to disable the buffering with setbuf.
 * https://ycpcs.github.io/cs365-spring2019/lectures/lecture15.html
 */
struct tor_ctl {
  char* os_endpoint;
  FILE* rsock;
  FILE* wsock;
};

enum TOR_ONION_FLAGS {
  TOR_ONION_FLAG_NONE = 0,
  TOR_ONION_FLAG_NON_ANONYMOUS = 1 << 0
};

int tor_ctl_connect(struct tor_ctl* ctx, char* addr, char* service);
int tor_ctl_add_onion(struct tor_ctl* ctx, struct tor_os_str* tos, uint16_t* port, uint64_t port_per_os, enum TOR_ONION_FLAGS flags);
void tor_ctl_list_onions(struct tor_ctl* ctx);
void tor_ctl_close(struct tor_ctl* ctx);

void tor_ctl_server_init(struct evt_core_ctx *ctx);
void tor_ctl_server_handle(struct evt_core_ctx *ctx, int fd);
