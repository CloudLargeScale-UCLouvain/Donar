// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "proxy.h"

int main_on_tcp_co(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  int conn_sock1, conn_sock2;
  struct sockaddr_in addr;
  socklen_t in_len;
  char url[1024], port[6];
  struct evt_core_cat local_cat = {0};
  struct evt_core_fdinfo to_fdinfo = {0};
  to_fdinfo.cat = &local_cat;
  to_fdinfo.url = url;

  in_len = sizeof(addr);
  conn_sock1 = accept(fdinfo->fd, (struct sockaddr*)&addr, &in_len);

  if (conn_sock1 == -1 && errno == EAGAIN) return 1;
  if (conn_sock1 == -1) goto co_error;
  conn_sock2 = dup(conn_sock1);
  if (conn_sock2 == -1) goto co_error;
  //printf("fd=%d accepts, creating fds=%d,%d\n", fd, conn_sock1, conn_sock2);

  url_get_port(port, fdinfo->url);

  to_fdinfo.fd = conn_sock1;
  to_fdinfo.cat->name = "tcp-read";
  sprintf(to_fdinfo.url, "tcp:read:127.0.0.1:%s", port);
  evt_core_add_fd (ctx, &to_fdinfo);

  to_fdinfo.fd = conn_sock2;
  to_fdinfo.cat->name = "tcp-write";
  sprintf(to_fdinfo.url, "tcp:write:127.0.0.1:%s", port);
  evt_core_add_fd (ctx, &to_fdinfo);

  printf("[%s][proxy] Accepted a new connection on port=%s: read_fd=%d, write_fd=%d\n", current_human_datetime (), port, conn_sock1, conn_sock2);

  return 0;

co_error:
  perror("Failed to handle new connection");
  exit(EXIT_FAILURE);
}

int main_on_tcp_read(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct buffer_packet* bp;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  int read_res = FDS_READY;

  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Get current read buffer OR a new read buffer OR subscribe to be notified later\n");
  if ((bp = get_read_buffer(&app_ctx->br, fdinfo)) == NULL) return 1;

  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Try to read a whole packet in the buffer\n");
  while (bp->mode == BP_READING) {
    read_res = read_packet_from_tcp (fdinfo, bp);
    if (read_res == FDS_ERR) goto co_error;
    if (read_res == FDS_AGAIN) return 1;
  }

  app_ctx->cell_rcv++;
  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Call logic on packet\n");
  return app_ctx->desc->on_stream(ctx, fdinfo, bp);

co_error:
  perror("Failed to TCP read");
  mv_buffer_rtof (&app_ctx->br, fdinfo);
  evt_core_rm_fd (ctx, fdinfo->fd);
  return 1;
}

int main_on_udp_read(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct buffer_packet* bp;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  int read_res = FDS_READY;
  char url[255];

  // 1. Get current read buffer OR a new read buffer OR subscribe to be notified later
  if ((bp = get_read_buffer(&app_ctx->br, fdinfo)) == NULL) return 1;

  // 2. Read packet from socket
  read_res = read_packet_from_udp (fdinfo, bp, fdinfo->other);
  if (read_res == FDS_ERR) goto co_error;
  if (read_res == FDS_AGAIN) return 1;

  // 3. Notify helpers
  app_ctx->udp_rcv++;
  traffic_capture_notify (&app_ctx->cap, bp, "in");

  // 4. Apply logic
  return app_ctx->desc->on_datagram(ctx, fdinfo, bp);

co_error:
  perror("Failed to UDP read");
  mv_buffer_rtof (&app_ctx->br, fdinfo);
  return 0;
}

int main_on_tcp_write(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct buffer_packet* bp;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct rr_ctx* rr = app_ctx->misc;
  int write_res = FDS_READY;

  // 0. Show some information about circuits
  uint8_t is_rdy = fdinfo->cat->socklist->len >= app_ctx->link_count ? 1 : 0;
  if (!app_ctx->is_rdy && is_rdy) printf("=== Our %d requested circuits are now up ===\n", app_ctx->link_count);
  else if (app_ctx->is_rdy && !is_rdy && fdinfo->cat->socklist->len != app_ctx->past_links) printf("=== Only %d/%d circuits are available, results could be biased ===\n", fdinfo->cat->socklist->len, app_ctx->link_count);
  app_ctx->is_rdy = app_ctx->is_rdy || is_rdy; // we don't want deactivation finally, after all the test is running
  app_ctx->past_links =  fdinfo->cat->socklist->len;

  // 1. Get current write buffer OR a buffer from the waiting queue OR leave
  if ((bp = get_write_buffer(&app_ctx->br, fdinfo)) == NULL) return 1;

  // 1.5. Prevent buffer sending if we are still bootstrapping...
  if (app_ctx->ap.is_waiting_bootstrap && !app_ctx->is_rdy) goto free_buffer;

  // 2. Write data from the buffer to the socket
  write_res = write_packet_to_tcp(fdinfo, bp);
  if (write_res == FDS_ERR) goto co_error;
  if (write_res == FDS_AGAIN) return EVT_CORE_FD_EXHAUSTED;

  app_ctx->cell_sent++;

free_buffer:
  // 3. A whole packet has been written
  // Release the buffer and notify
  mv_buffer_wtof(&app_ctx->br, fdinfo);
  notify_read(ctx, &app_ctx->br);

  return EVT_CORE_FD_UNFINISHED;
co_error:
  perror("Failed to TCP write");
  mv_buffer_wtof (&app_ctx->br, fdinfo);
  evt_core_rm_fd (ctx, fdinfo->fd);
  return EVT_CORE_FD_EXHAUSTED;
}

int main_on_udp_write (struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct buffer_packet* bp;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  int write_res = FDS_READY;

  // 1. Get current write buffer OR a buffer from the waiting queue OR leave
  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Find write buffer\n");
  if ((bp = get_write_buffer(&app_ctx->br, fdinfo)) == NULL) return 1;

  // 2. Write buffer
  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Write UDP packet\n");
  write_res = write_packet_to_udp(fdinfo, bp, fdinfo->other);
  if (write_res == FDS_ERR) goto co_error;
  if (write_res == FDS_AGAIN) return 1;

  // 3. Notify helpers
  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Notify traffic capture\n");
  app_ctx->udp_sent++;
  traffic_capture_notify (&app_ctx->cap, bp, "out");

  // 4. A whole packet has been written
  // Release the buffer and notify
  if (ctx->verbose > 1) fprintf(stderr, "  [proxy] Release buffer and notify\n");
  mv_buffer_wtof(&app_ctx->br, fdinfo);
  notify_read(ctx, &app_ctx->br);

  return 0;
co_error:
  perror("Failed to UDP write");
  mv_buffer_wtof (&app_ctx->br, fdinfo);
  return 0;
}

int main_on_err(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct buffer_packet* bp;

  // 1. If has a "used" buffer, remove it
  mv_buffer_rtof (&app_ctx->br, fdinfo);

  // 2. If appears in the write waiting queue, remove it
  while (get_write_buffer (&app_ctx->br, fdinfo) != NULL) {
    mv_buffer_wtof(&app_ctx->br, fdinfo);
  }

  // 3. If appears in the read waiting queue, remove it
  g_queue_remove_all (app_ctx->br.read_waiting, &(fdinfo->fd));

  return app_ctx->desc->on_err(ctx, fdinfo);
}

void algo_main_destroy(void* app_ctx) {
  struct algo_ctx* ctx = (struct algo_ctx*) app_ctx;

  ctx->ref_count--;
  if (ctx->ref_count > 0) return;

  printf("udp_sent: %ld, udp_rcv: %ld, cells_sent: %ld, cells_rcv: %ld\n", ctx->udp_sent, ctx->udp_rcv, ctx->cell_sent, ctx->cell_rcv);
  traffic_capture_stop(&ctx->cap);
  destroy_buffer_management(&ctx->br);
  if (ctx->free_misc) ctx->free_misc(ctx->misc);
  free(ctx);
}

void algo_main_init(struct evt_core_ctx* evt, struct algo_params* ap) {
  struct algo_ctx* ctx = malloc(sizeof(struct algo_ctx));
  if (ctx == NULL) goto init_err;
  memset(ctx, 0, sizeof(struct algo_ctx));
  ctx->link_count = ap->links;
  ctx->is_rdy = 0;
  ctx->ap = *ap;

  struct evt_core_cat tcp_listen = {
    .name = "tcp-listen",
    .flags = EPOLLIN,
    .app_ctx = ctx,
    .free_app_ctx = algo_main_destroy,
    .cb = main_on_tcp_co,
    .err_cb = NULL
  };
  ctx->ref_count++;
  evt_core_add_cat(evt, &tcp_listen);

  struct evt_core_cat tcp_read = {
    .name = "tcp-read",
    .flags = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP,
    .app_ctx = ctx,
    .free_app_ctx = algo_main_destroy,
    .cb = main_on_tcp_read,
    .err_cb = main_on_err
  };
  ctx->ref_count++;
  evt_core_add_cat(evt, &tcp_read);

  struct evt_core_cat udp_read = {
    .name = "udp-read",
    .flags = EPOLLIN | EPOLLET,
    .app_ctx = ctx,
    .free_app_ctx = algo_main_destroy,
    .cb = main_on_udp_read,
    .err_cb = main_on_err
  };
  ctx->ref_count++;
  evt_core_add_cat(evt, &udp_read);

  struct evt_core_cat tcp_write = {
    .name = "tcp-write",
    .flags = EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLRDHUP,
    .app_ctx = ctx,
    .free_app_ctx = algo_main_destroy,
    .cb = main_on_tcp_write,
    .err_cb = main_on_err
  };
  ctx->ref_count++;
  evt_core_add_cat(evt, &tcp_write);

  struct evt_core_cat udp_write = {
    .name = "udp-write",
    .flags = EPOLLOUT | EPOLLET,
    .app_ctx = ctx,
    .free_app_ctx = algo_main_destroy,
    .cb = main_on_udp_write,
    .err_cb = main_on_err
  };
  ctx->ref_count++;
  evt_core_add_cat(evt, &udp_write);

  init_buffer_management(&ctx->br);
  traffic_capture_init(&ctx->cap, ap->capture_file);

  for (int i = 0; i < sizeof(available_algo) / sizeof(available_algo[0]); i++) {
    if (strcmp(available_algo[i].name, ap->algo_name) == 0) {
      ctx->desc = &(available_algo[i]);
      ctx->desc->init(evt, ctx, ap);
      return;
    }
  }
  fprintf(stderr, "Algorithm %s has not been found\n", ap->algo_name);
  exit(EXIT_FAILURE);

init_err:
  fprintf(stderr, "Failed to init proxy\n");
  exit(EXIT_FAILURE);
}
