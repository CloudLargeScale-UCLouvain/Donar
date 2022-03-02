// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "proxy.h"
#include "algo_utils.h"
#include "packet.h"

struct dup2_ctx {
  uint16_t recv_id;
  uint16_t emit_id;
};

void algo_dup2_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap) {
  app_ctx->misc = malloc(sizeof(struct dup2_ctx));
  if (app_ctx->misc == NULL) {
    perror("malloc failed in algo dup2 init");
    exit(EXIT_FAILURE);
  }
  memset(app_ctx->misc, 0, sizeof(struct dup2_ctx));
}

int algo_dup2_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  char url[256];
  struct evt_core_fdinfo *to_fdinfo = NULL;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  union abstract_packet *ap = (union abstract_packet*) &bp->ip;
  struct dup2_ctx* dup2c = app_ctx->misc;
  int32_t id = -1, port = -1;
  if (ctx->verbose > 1) {
    fprintf(stderr, "    [algo_dup2] Received a buffer\n");
    dump_buffer_packet(bp);
  }

  do {
    switch (ap->fmt.headers.cmd) {
    case CMD_UDP_METADATA_THUNDER:
      id = ap->fmt.content.udp_metadata_thunder.id;
      break;
    case CMD_UDP_ENCAPSULATED:
      port = ap->fmt.content.udp_encapsulated.port;
      break;
    default:
      break;
    }
  } while ((ap = ap_next(ap)) != NULL);
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_dup2] Extracted port=%d and id=%d\n", port, id);

  if (port == -1 || id == -1) {
    fprintf(stderr, "Missing data port=%d and id=%d...\n", port, id);
    exit(EXIT_FAILURE);
  }

  // Check that received identifier has not been delivered
  if (ring_ge(dup2c->recv_id, id)) {
    if (ctx->verbose > 1) fprintf(stderr, "    [algo_dup2] Packet already delivered, dropping\n");
    mv_buffer_rtof(&app_ctx->br, fdinfo);
    return 0;
  }

  // Update delivered identifier
  dup2c->recv_id = id;

  // 1. Find destination
  sprintf(url, "udp:write:127.0.0.1:%d", port);
  to_fdinfo = evt_core_get_from_url (ctx, url);
  if (to_fdinfo == NULL) {
    fprintf(stderr, "No fd for URL %s in tcp-read. Dropping packet :( \n", url);
    mv_buffer_rtof (&app_ctx->br, fdinfo);
    return 1;
  }

  // 2. Move buffer
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_dup2] Scheduling packet for write\n");
  mv_buffer_rtow (&app_ctx->br, fdinfo, to_fdinfo);
  main_on_udp_write(ctx, to_fdinfo);

  return 0;
}

int algo_dup2_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct evt_core_fdinfo *to_fdinfo = NULL;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;

  struct dup2_ctx* dup2c = app_ctx->misc;
  dup2c->emit_id = dup2c->emit_id + 1;
  union abstract_packet metadata = {
    .fmt.headers.cmd = CMD_UDP_METADATA_THUNDER,
    .fmt.headers.size = sizeof(metadata),
    .fmt.headers.flags = 0,
    .fmt.content.udp_metadata_thunder.id = dup2c->emit_id
  };
  union abstract_packet *apbuf = buffer_append_ap (bp, &metadata);
  if (ctx->verbose > 1) {
    dump_buffer_packet(bp);
    fprintf(stderr, "    [algo_dup2] Added metadata\n");
  }

  struct evt_core_cat* cat = evt_core_get_from_cat (ctx, "tcp-write");
  for (int i = 0; i < app_ctx->ap.links; i++) {
    // 1. A whole packet has been read, we will find someone to write it
    to_fdinfo = cat->socklist->len > i ? g_array_index(cat->socklist, struct evt_core_fdinfo*, i) : NULL;
    if (to_fdinfo == NULL) {
      fprintf(stderr, "No fd for cat %s in udp-read.\n", cat->name);
      continue;
    }

    // 2. We move the buffer and notify the target
    dup_buffer_tow (&app_ctx->br, bp, to_fdinfo);
    main_on_tcp_write(ctx, to_fdinfo);
  }
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_dup2] Packets sent\n");

  // 3. Release the buffer
  mv_buffer_rtof (&app_ctx->br, fdinfo);

  return 0;
}

int algo_dup2_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  if (strcmp("tcp-read", fdinfo->cat->name) == 0 || strcmp("tcp-write", fdinfo->cat->name) == 0)
    return app_ctx->ap.sr(ctx, fdinfo);

  fprintf(stderr, "%s is not eligible for a reconnect\n", fdinfo->url);
 // We do nothing
  return 1;
}
