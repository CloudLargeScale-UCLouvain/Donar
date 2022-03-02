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
#include "evt_core.h"
#include "net_tools.h"
#include "measure.h"

struct udpecho_ctx {
  struct measure_params mp;
  struct measure_state ms;
  uint8_t is_measlat;
  uint8_t verbose;
};

int on_udp(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  ssize_t nread, nwritten;
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  struct udpecho_ctx *uctx = fdinfo->cat->app_ctx;

  nread = recvfrom(fdinfo->fd, uctx->ms.mp_in, uctx->mp.payload_size, MSG_TRUNC, (struct sockaddr*)&addr, &addrlen);
  if (nread == -1 && errno == EAGAIN) return 1; // Read done
  if (nread <= 0 || nread > uctx->mp.payload_size) {
    fprintf(stderr, "Message is either truncated or an error occured. nread=%ld, expected=%ld\n", nread, uctx->mp.payload_size);
    perror("read errno");
    exit(EXIT_FAILURE);
  }

  if (uctx->is_measlat) measure_parse (&uctx->mp, &uctx->ms, uctx->verbose);

  nwritten = sendto(fdinfo->fd, uctx->ms.mp_in, nread, 0, (struct sockaddr*)&addr, addrlen);
  // @FIXME don't support EAGAIN on write. Could be intended, you don't think so?
  if (nwritten != nread) {
    fprintf(stderr, "Didn't write the same number of bytes as read. nread=%ld, nwritten=%ld\n", nread, nwritten);
    perror("write errno");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("~ udpecho ~\n");

  struct udpecho_ctx uctx = {0};
  int opt, udp_sock = 0;
  char *port = NULL, *bindhost = NULL;
  struct evt_core_ctx evts = {0};
  //uctx.mp.payload_size = 1500;
  measure_params_init (&uctx.mp);

  // 1. Parse parameters
  while ((opt = getopt(argc, argv, "b:p:vms:")) != -1) {
      switch(opt) {
      case 'v':
        uctx.verbose++;
        break;
      case 'p':
        port = optarg;
        break;
      case 'b':
        bindhost = optarg;
        break;
      case 'm':
        uctx.is_measlat = 1;
        break;
      case 's':
        measure_params_setpl (&uctx.mp, atoi(optarg));
        break;
      default:
        goto usage;
      }
  }
  if (bindhost == NULL) bindhost = "127.0.0.1";
  measure_state_init (&uctx.mp, &uctx.ms);

  // 2. Register category
  struct evt_core_cat udp_read = {
    .app_ctx = &uctx,
    .free_app_ctx = NULL,
    .cb = on_udp,
    .err_cb = NULL,
    .name = "udp-read",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };
  evt_core_init(&evts, uctx.verbose <= 0 ? 0 : uctx.verbose - 1);
  evt_core_add_cat(&evts, &udp_read);

  // 3. Register UDP socket
  udp_sock = create_udp_server (bindhost, port);

  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  fdinfo.fd = udp_sock;
  sprintf(url, "udp:rw:127.0.0.1:%s", port);
  fdinfo.cat->name = "udp-read";
  evt_core_add_fd(&evts, &fdinfo);

  // 4. Start main loop
  evt_core_loop (&evts);

  return 0;
usage:
  fprintf(stderr, "Usage: %s -p <port> [-v] [-b ip]\n", argv[0]);
}
