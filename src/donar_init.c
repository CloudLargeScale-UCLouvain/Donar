// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "donar_init.h"

void free_udp_t(void* v) {
  struct udp_target* udp_t = v;
  udp_t->ref_count--;
  if (udp_t->ref_count <= 0) {
    free(udp_t);
  }
}

int on_signal(struct evt_core_ctx* evts, struct evt_core_fdinfo* fdinfo) {
  struct signalfd_siginfo fdsi;

  int sig_rd = read(fdinfo->fd, &fdsi, sizeof(struct signalfd_siginfo));
  if (sig_rd == -1 && errno == EAGAIN) return 1;
  if (sig_rd != sizeof(struct signalfd_siginfo)) {
    fprintf(stderr, "read size: %d / %ld\n", sig_rd, sizeof(struct signalfd_siginfo));
    perror("signal read failed");
    exit(EXIT_FAILURE);
  }
  printf("Signal received: %d\n", fdsi.ssi_signo);

  if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT || fdsi.ssi_signo == SIGTERM) {
    printf("Stop main loop and quit\n");
    evts->loop = 0;
  }
  return 0;
}

void signal_init(struct evt_core_ctx* evts) {
  sigset_t mask = {0};

  struct evt_core_cat signal_read = {
    .name = "signal-read",
    .flags = EPOLLIN | EPOLLET | EPOLLRDHUP,
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = on_signal,
    .err_cb = NULL
  };
  evt_core_add_cat(evts, &signal_read);

  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGQUIT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGPIPE);
  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
    perror("sigprocmask");
    exit(EXIT_FAILURE);
  }
  int sigfd = signalfd(-1, &mask, 0);
  if (sigfd == -1) {
    perror("signalfd failed");
    exit(EXIT_FAILURE);
  }

  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.fd = sigfd;
  fdinfo.cat = &signal_read;
  fdinfo.url = "signal:read:int+quit";
  evt_core_add_fd(evts, &fdinfo);
}

void init_udp_remote(char* port, struct evt_core_ctx* evts) {
  int sock1, sock2;
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};

  // 1. Init shared parameters for the fdinfo structure
  struct udp_target* udp_t = malloc(sizeof(struct udp_target));
  if (udp_t == NULL) goto socket_failed;
  memset(udp_t, 0, sizeof(struct udp_target));
  udp_t->ref_count = 2;

  fdinfo.cat = &cat;
  fdinfo.url = url;
  fdinfo.free_other = free_udp_t;
  fdinfo.other = udp_t;

  // 2. Duplicate sockets
  sock1 = create_udp_server ("127.13.3.7", port);
  if (sock1 < 0) goto socket_failed;
  sock2 = dup(sock1);
  if (sock2 < 0) goto socket_failed;

  // 3. Register them
  fdinfo.cat->name = "udp-read";
  fdinfo.fd = sock1;
  sprintf(fdinfo.url, "udp:read:127.0.0.1:%s", port);
  evt_core_add_fd (evts, &fdinfo);

  fdinfo.cat->name = "udp-write";
  fdinfo.fd = sock2;
  sprintf(fdinfo.url, "udp:write:127.0.0.1:%s", port);
  evt_core_add_fd (evts, &fdinfo);
  return;

socket_failed:
  fprintf(stderr, "UDP socket init failed\n");
  exit(EXIT_FAILURE);
}

void init_udp_exposed(char* bound_ip, char* port, struct evt_core_ctx* evts) {
  int sock1, sock2;
  char url[1024];

  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};

  // 1. Init shared parameters for the fdinfo structure
  struct udp_target* udp_t = malloc(sizeof(struct udp_target));
  if (udp_t == NULL) goto socket_failed;
  memset(udp_t, 0, sizeof(struct udp_target));
  udp_t->ref_count = 2;

  fdinfo.cat = &cat;
  fdinfo.url = url;
  fdinfo.free_other = free_udp_t;
  fdinfo.other = udp_t;

  sock1 = create_udp_client (bound_ip, port);
  if (sock1 < 0) goto socket_failed;
  sock2 = dup(sock1);
  if (sock2 < 0) goto socket_failed;

  fdinfo.fd = sock1;
  fdinfo.cat->name = "udp-read";
  sprintf(fdinfo.url, "udp:read:127.0.0.1:%s", port);
  evt_core_add_fd (evts, &fdinfo);

  fdinfo.fd = sock2;
  fdinfo.cat->name = "udp-write";
  sprintf(fdinfo.url, "udp:write:127.0.0.1:%s", port);
  evt_core_add_fd (evts, &fdinfo);

  return;

socket_failed:
  fprintf(stderr, "UDP socket init failed\n");
  exit(EXIT_FAILURE);
}

void free_port (void* ptr) {
  free(ptr);
}

void donar_init_params(struct donar_params* dp) {
  dp->onion_file = NULL;
  dp->algo = NULL;
  dp->capture_file = NULL;
  dp->bound_ip = NULL;
  dp->is_server = 0;
  dp->is_client = 0;
  dp->algo_specific_params = NULL;
  dp->is_waiting_bootstrap = 0;
  dp->errored = 0;
  dp->links = 8;
  dp->fresh_data = 1;
  dp->redundant_data = 0;
  dp->base_port = 7500;
  strcpy(dp->tor_ip, "127.0.0.1");
  strcpy(dp->my_ip_for_tor, "127.13.3.7");
  dp->remote_ports = g_ptr_array_new_with_free_func (free_port);
  dp->exposed_ports = g_ptr_array_new_with_free_func (free_port);
}
