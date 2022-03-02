// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "tor_ctl.h"
#include "evt_core.h"
#include "net_tools.h"
#include "url.h"
#include "measure.h"

struct torecho_ctx {
  uint8_t is_measlat, is_tor, verbose;
  struct measure_params mp;
  struct measure_state ms;
};

void te_create_onion_services(struct tor_os_str* tos, struct tor_ctl* tctl, uint16_t* ports, int ports_count, enum TOR_ONION_FLAGS tof) {
    tor_os_create (tos, "onion_services.pub", "onion_services.txt", 1);
    tor_os_read (tos);

    int err = 0;
    err = tor_ctl_connect (tctl, "127.0.0.1", "9051");
    if (err < 0) {
      fprintf(stderr, "Unable to open Tor Socket\n");
      exit(EXIT_FAILURE);
    }
    err = tor_ctl_add_onion (tctl, tos, ports, ports_count, tof);
    if (err != 0) {
      fprintf(stderr, "Unable to create Onion Services (error: %d)\n", err);
      exit(EXIT_FAILURE);
    }
}

int te_on_tcp_co(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
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

  url_get_port(port, fdinfo->url);

  to_fdinfo.fd = conn_sock1;
  to_fdinfo.cat->name = "tcp-all";
  sprintf(to_fdinfo.url, "tcp:all:127.0.0.1:%s", port);
  evt_core_add_fd (ctx, &to_fdinfo);

  return 0;

co_error:
  perror("Failed to handle new connection");
  exit(EXIT_FAILURE);
}

int te_on_tcp(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  ssize_t nread, nwritten;
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  struct torecho_ctx *tctx = fdinfo->cat->app_ctx;

  nread = recv(fdinfo->fd, tctx->ms.mp_in, tctx->mp.payload_size, 0);
  if (nread == -1 && errno == EAGAIN) return 1; // Read done
  if (nread == 0) { fprintf(stderr, "WARN! Read 0 bytes.\n"); return 1; }
  if (nread < 0 || nread > tctx->mp.payload_size) {
    fprintf(stderr, "Message is either truncated or an error occured. nread=%ld, expected=%ld\n", nread, tctx->mp.payload_size);
    perror("read errno");
    exit(EXIT_FAILURE);
  }

  if (tctx->is_measlat) measure_parse (&tctx->mp, &tctx->ms, tctx->verbose);

  nwritten = send(fdinfo->fd, tctx->ms.mp_in, nread, 0);
  // @FIXME don't support EAGAIN on write. Could be intended, you don't think so?
  if (nwritten != nread) {
    fprintf(stderr, "Didn't write the same number of bytes as read - not supported. nread=%ld, nwritten=%ld\n", nread, nwritten);
    perror("write errno");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("~ torecho ~\n");

  int tcp_serv_sock = 0, err, opt;
  struct evt_core_ctx evts = {0};
  uint16_t port = 7500;
  struct tor_os_str tos;
  struct tor_ctl tctl;
  tctl.os_endpoint = "127.0.0.1";
  enum TOR_ONION_FLAGS tof = TOR_ONION_FLAG_NONE;
  char url[1024];
  struct torecho_ctx tctx = {0};
  //tctx.mp.payload_size = 1500;
  measure_params_init (&tctx.mp);

  while ((opt = getopt(argc, argv, "ns:mtp:v")) != -1) {
      switch(opt) {
      case 'v':
        tctx.verbose++;
        break;
      case 't':
        tctx.is_tor = 1;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'n':
        tof |= TOR_ONION_FLAG_NON_ANONYMOUS;
        break;
      case 'm':
        tctx.is_measlat = 1;
        break;
      case 's':
        measure_params_setpl (&tctx.mp, atoi(optarg));
        break;
      default:
        break;
      }
  }

  // 1. Register categories
  struct evt_core_cat tcp_co = {
    .app_ctx = &tctx,
    .free_app_ctx = NULL,
    .cb = te_on_tcp_co,
    .err_cb = NULL,
    .name = "tcp-co",
    .flags = EPOLLIN,
    .socklist = NULL
  };
  struct evt_core_cat tcp_all = {
    .app_ctx = &tctx  ,
    .free_app_ctx = NULL,
    .cb = te_on_tcp,
    .err_cb = NULL,
    .name = "tcp-all",
    .flags = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP,
    .socklist = NULL
  };
  measure_state_init (&tctx.mp, &tctx.ms);

  evt_core_init(&evts, tctx.verbose <= 0 ? 0 : tctx.verbose - 1);
  evt_core_add_cat(&evts, &tcp_co);
  evt_core_add_cat(&evts, &tcp_all);
  printf("--- Categories created\n");

  if (tctx.is_tor) {
    // 2. Create or load onion services
    te_create_onion_services (&tos, &tctl, &port, 1, tof);
    printf("--- Onion services created\n");
  }

  // 3. Create TCP server
  sprintf(url, "%d", port);
  tcp_serv_sock = create_tcp_server ("0.0.0.0", url);
  err = listen(tcp_serv_sock, SOMAXCONN);

  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  fdinfo.fd = tcp_serv_sock;
  sprintf(url, "tcp:co:127.0.0.1:%d", port);
  fdinfo.cat->name = "tcp-co";
  evt_core_add_fd(&evts, &fdinfo);
  printf("--- TCP server is listening\n");

  // 4. Start main loop
  evt_core_loop (&evts);

  return 0;
}
