// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "proxy.h"
#include "algo_utils.h"

void algo_naive_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap) {
  // We do nothing
}

int algo_naive_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  char url[256];
  struct evt_core_fdinfo *to_fdinfo = NULL;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;

  if (ctx->verbose > 1) fprintf(stderr, "    [algo_naive] 1/2 Find destination\n");
  sprintf(url, "udp:write:127.0.0.1:%d", ap->fmt.content.udp_encapsulated.port);
  to_fdinfo = evt_core_get_from_url (ctx, url);
  if (to_fdinfo == NULL) {
    fprintf(stderr, "No fd for URL %s in tcp-read. Dropping packet :( \n", url);
    mv_buffer_wtof (&app_ctx->br, fdinfo);
    return 1;
  }

  if (ctx->verbose > 1) fprintf(stderr, "    [algo_naive] 2/2 Move buffer\n");
  mv_buffer_rtow (&app_ctx->br, fdinfo, to_fdinfo);
  main_on_udp_write(ctx, to_fdinfo);

  return 0;
}

int algo_naive_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct evt_core_fdinfo *to_fdinfo = NULL;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;

  // 1. A whole packet has been read, we will find someone to write it
  struct evt_core_cat* cat = evt_core_get_from_cat (ctx, "tcp-write");
  to_fdinfo = cat->socklist->len > 0 ? g_array_index(cat->socklist, struct evt_core_fdinfo*, 0) : NULL;
  if (to_fdinfo == NULL) {
    fprintf(stderr, "No fd for cat %s in udp-read. Dropping packet :( \n", cat->name);
    mv_buffer_wtof(&app_ctx->br, fdinfo);
    return 1;
  }
  //printf("Pass packet from %s to %s\n", fdinfo->url, url);

  // 2. We move the buffer and notify the target
  mv_buffer_rtow (&app_ctx->br, fdinfo, to_fdinfo);
  main_on_tcp_write(ctx, to_fdinfo);

  return 0;
}

int algo_naive_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  if (strcmp("tcp-read", fdinfo->cat->name) == 0 || strcmp("tcp-write", fdinfo->cat->name) == 0)
    return app_ctx->ap.sr(ctx, fdinfo);

  fprintf(stderr, "%s is not eligible for a reconnect\n", fdinfo->url);
 // We do nothing
  return 1;
}
