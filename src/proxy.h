// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "evt_core.h"
#include "algo_utils.h"
#include "capture_traffic.h"
#include "url.h"
#include "utils.h"
#include "packet.h"

typedef int (*stream_repair)(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);

struct algo_params {
  uint8_t is_waiting_bootstrap;
  char *algo_name, *capture_file, *algo_specific_params;
  int links, fresh_data, redundant_data, base_port;
  stream_repair sr;
};

struct algo_ctx;
typedef void (*algo_init)(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap);
typedef int  (*algo_ctx_on_buffer)(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
typedef int  (*algo_ctx_on_event)(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);

typedef void (*algo_ctx_free_misc)(void*);

struct algo_desc {
  char* name;
  algo_init init;
  algo_ctx_on_buffer on_stream;
  algo_ctx_on_buffer on_datagram;
  algo_ctx_on_event on_err;
};

struct algo_ctx {
  struct algo_desc* desc;
  uint8_t link_count;
  uint8_t is_rdy;
  uint8_t past_links;
  struct algo_params ap;
  int ref_count;
  uint64_t udp_rcv, udp_sent, cell_rcv, cell_sent;
  struct capture_ctx cap;
  struct buffer_resources br;
  void* misc;                       // Additional structures
  algo_ctx_free_misc free_misc;     // Fx ptr to free misc
};

void algo_naive_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap);
int  algo_naive_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int  algo_naive_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int  algo_naive_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo);

void algo_dup2_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap);
int algo_dup2_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int algo_dup2_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int algo_dup2_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo);

void algo_thunder_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap);
int algo_thunder_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int algo_thunder_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int algo_thunder_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo);

void algo_lightning_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap);
int algo_lightning_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int algo_lightning_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp);
int algo_lightning_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo);

static struct algo_desc available_algo[] = {
  {
    .name = "naive",
    .init = algo_naive_init,
    .on_stream = algo_naive_on_stream,
    .on_datagram = algo_naive_on_datagram,
    .on_err = algo_naive_on_err
  },
  {
    .name = "dup2",
    .init = algo_dup2_init,
    .on_stream = algo_dup2_on_stream,
    .on_datagram = algo_dup2_on_datagram,
    .on_err = algo_dup2_on_err
  },
  {
    .name = "thunder",
    .init = algo_thunder_init,
    .on_stream = algo_thunder_on_stream,
    .on_datagram = algo_thunder_on_datagram,
    .on_err = algo_thunder_on_err
  },
  {
    .name = "lightning",
    .init = algo_lightning_init,
    .on_stream = algo_lightning_on_stream,
    .on_datagram = algo_lightning_on_datagram,
    .on_err = algo_lightning_on_err
  }
};

void algo_main_init(struct evt_core_ctx* evt, struct algo_params* ap);

int main_on_tcp_co(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);
int main_on_tcp_read(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);
int main_on_udp_read(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);
int main_on_tcp_write(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);
int main_on_udp_write (struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);
int main_on_timer(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);
int main_on_err(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo);

//@FIXME UGLY

// A Tor cell size is 512 bytes but handle only 498 bytes of data
#define TOR_CELL_SIZE 498
#define MAX_LINKS 64

struct thunder_ctx {
  // shared/global parameters
  uint8_t total_links;
  struct timespec start_time;

  // scheduler parameters
  uint8_t selected_link;
  uint64_t blacklisted[MAX_LINKS];

  // classifier send side parameters
  struct timespec prev_link_time;
  uint8_t to_increment[MAX_LINKS];
  uint64_t delta_t_per_link[MAX_LINKS];
  uint64_t sent_pkts_on_link[MAX_LINKS];

  // classifier receive side parameters
  uint64_t rcv_delta_t_per_link[MAX_LINKS];
  uint64_t received_pkts_on_link[MAX_LINKS];
  struct timespec prev_rcv_link_time;
  int64_t owdd[MAX_LINKS];
  // uint16_t received_delta_by_links[MAX_LINKS][MAX_LINKS];
  // struct timespec last_received_on_link[MAX_LINKS];

  // prepare/adapt parameters
  uint16_t recv_id;
  uint16_t emit_id;

  // pad/unpad parameter
  size_t monit_pkt_size;

  // user parameters
  int64_t allowed_jitter_ms;
  int scheduler_activated;

};
