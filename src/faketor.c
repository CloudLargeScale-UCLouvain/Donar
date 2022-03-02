// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include "evt_core.h"
#include "socks5.h"
#include "tor_ctl.h"
#include "timer.h"

int faketor_socks5_listen(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  int conn_sock1;
  struct sockaddr_in addr;
  socklen_t in_len;

  in_len = sizeof(addr);
  conn_sock1 = accept(fdinfo->fd, (struct sockaddr*)&addr, &in_len);

  if (conn_sock1 == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (conn_sock1 == -1) goto co_error;

  make_socket_non_blocking (conn_sock1);
  printf("[%s][faketor] Accepted a new connection for socks5 \n", current_human_datetime ());
  socks5_server_handle_req (ctx, conn_sock1);

  return EVT_CORE_FD_UNFINISHED;
co_error:
  perror("Failed to handle new socks5 connection");
  exit(EXIT_FAILURE);
}

int faketor_control_listen(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  int conn_sock;
  struct sockaddr_in addr;
  socklen_t in_len;

  in_len = sizeof(addr);
  conn_sock = accept(fdinfo->fd, (struct sockaddr*)&addr, &in_len);

  if (conn_sock == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (conn_sock == -1) goto co_error;

  make_socket_non_blocking (conn_sock);
  printf("[%s][faketor] Accepted a new connection for control port \n", current_human_datetime ());
  tor_ctl_server_handle(ctx, conn_sock);

  return EVT_CORE_FD_UNFINISHED;

co_error:
  perror("Failed to handle new control port connection");
  exit(EXIT_FAILURE);
}

int faketor_socks5_server_success(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  if (strstr(fdinfo->url, ":write") != NULL) return EVT_CORE_FD_EXHAUSTED;
  // ctos-read and stoc-write
  char buffer[512];
  sprintf(buffer, "%s:write", fdinfo->url);
  struct evt_core_fdinfo *fdinfo2 = evt_core_dup(ctx, fdinfo, buffer);
  evt_core_mv_fd2(ctx, fdinfo, "ctos-read");
  evt_core_mv_fd2(ctx, fdinfo2, "stoc-write");
  printf("[%s][faketor] success socks5:\n\t+read: %s\n\t+write: %s\n", current_human_datetime (), fdinfo->url, fdinfo2->url);
  return EVT_CORE_FD_EXHAUSTED;
}

int faketor_torctl_server_success(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  if (strstr(fdinfo->url, ":write") != NULL) return EVT_CORE_FD_EXHAUSTED;
  // ctos-write and stoc-read
  char buffer[512];
  sprintf(buffer, "%s:write", fdinfo->url);
  struct evt_core_fdinfo *fdinfo2 = evt_core_dup(ctx, fdinfo, buffer);
  evt_core_mv_fd2(ctx, fdinfo, "stoc-read");
  evt_core_mv_fd2(ctx, fdinfo2, "ctos-write");
  printf("[%s][faketor] success torctl:\n\t+read: %s\n\t+write: %s\n", current_human_datetime (), fdinfo->url, fdinfo2->url);
  return EVT_CORE_FD_EXHAUSTED;
}

int faketor_socks5_server_failed(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  printf("failure socks5!\n");
  evt_core_rm_fd(ctx, fdinfo->fd);

  return EVT_CORE_FD_EXHAUSTED;
}

void get_fdinfo_couple(struct evt_core_ctx* ctx, struct evt_core_fdinfo *fdinfo, struct evt_core_fdinfo **source, struct evt_core_fdinfo **destination) {
  char *other_prefix, *current_prefix;
  char url_build[256];

  if (strstr(fdinfo->url, "socket:") != NULL) {
    current_prefix = "socket:";
    other_prefix = "torctl:";
  } else {
    current_prefix = "torctl:";
    other_prefix = "socket:";
  }

  sprintf(url_build, "%s%s", other_prefix, fdinfo->url + strlen(current_prefix));

  if (strstr(fdinfo->url, ":write") != NULL) {
    *destination = fdinfo;
    url_build[strlen(url_build) - strlen(":write")] = '\0';
    *source = evt_core_get_from_url(ctx, url_build);
  } else {
    *source = fdinfo;
    strcpy(url_build + strlen(url_build), ":write");
    *destination = evt_core_get_from_url (ctx, url_build);
  }
  /*if (*source == NULL || *destination == NULL) {
    fprintf(stderr, "[%s][faketor][WARN] Called with %s, computed %s but is not in fdpool.\n", current_human_datetime (), fdinfo->url, url_build);
  }*/
}

int faketor_bridge(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  char buffer[1600];
  ssize_t nread, nwrite;
  char url[512];
  struct evt_core_fdinfo *target, *source;

  // 0. find source
  get_fdinfo_couple (ctx, fdinfo, &source, &target);
  if (source == NULL || target == NULL) return EVT_CORE_FD_EXHAUSTED;
  //printf("triggered event: %s, source: %s, target: %s\n", fdinfo->url, source->url, target->url);

  // 1. read some data
  nread = recv(source->fd, buffer, sizeof(buffer), MSG_PEEK);
  if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (nread == -1) goto error;

  // 2. write to target
  nwrite = send(target->fd, buffer, nread, 0);
  if (nwrite == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (nwrite == -1) goto error;
  nread = recv(source->fd, buffer, nwrite, 0);
  if (nread == -1) goto error;

  return EVT_CORE_FD_UNFINISHED;

error:
  perror("oopsie");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  int opt = 0, port_socks5 = 9050, port_control = 9051;
  char str_port_socks5[6], str_port_control[6];

  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch(opt) {
    case 'p':
      port_socks5 = atoi(optarg);
      port_control = port_socks5+1;
      break;
    default:
      break;
    }
  }

  printf("ports socks5:%d control:%d\n", port_socks5, port_control);
  sprintf(str_port_socks5, "%d", port_socks5);
  sprintf(str_port_control, "%d", port_control);

  struct evt_core_ctx evts = {0};

  struct evt_core_cat socks5_listen = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_socks5_listen,
    .err_cb = NULL,
    .name = "socks5-listen",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat control_listen = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_control_listen,
    .err_cb = NULL,
    .name = "control-listen",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat socks5_server_success = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_socks5_server_success,
    .err_cb = NULL,
    .name = "socks5-server-success",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat socks5_server_failed = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_socks5_server_failed,
    .err_cb = NULL,
    .name = "socks5-server-failed",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat torctl_server_success = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_torctl_server_success,
    .err_cb = NULL,
    .name = "torctl-server-success",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat ctos_read = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_bridge,
    .err_cb = NULL,
    .name = "ctos-read",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat ctos_write = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_bridge,
    .err_cb = NULL,
    .name = "ctos-write",
    .flags = EPOLLOUT | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat stoc_read = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_bridge,
    .err_cb = NULL,
    .name = "stoc-read",
    .flags = EPOLLIN | EPOLLET,
    .socklist = NULL
  };

  struct evt_core_cat stoc_write = {
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = faketor_bridge,
    .err_cb = NULL,
    .name = "stoc-write",
    .flags = EPOLLOUT | EPOLLET,
    .socklist = NULL
  };

  evt_core_init(&evts, 0);
  evt_core_add_cat(&evts, &socks5_listen);
  evt_core_add_cat(&evts, &control_listen);
  evt_core_add_cat(&evts, &socks5_server_success);
  evt_core_add_cat(&evts, &socks5_server_failed);
  evt_core_add_cat(&evts, &torctl_server_success);
  evt_core_add_cat(&evts, &ctos_read);
  evt_core_add_cat(&evts, &ctos_write);
  evt_core_add_cat(&evts, &stoc_read);
  evt_core_add_cat(&evts, &stoc_write);

  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  int err, sock = 0;

  sock = create_tcp_server ("0.0.0.0", str_port_socks5);
  if (sock < 0) return EXIT_FAILURE;
  err = listen(sock, SOMAXCONN);
  if (err != 0) return EXIT_FAILURE;
  fdinfo.cat->name = "socks5-listen";
  fdinfo.fd = sock;
  fdinfo.url = "socks5:listen:9050";
  evt_core_add_fd(&evts, &fdinfo);

  sock = create_tcp_server ("0.0.0.0", str_port_control);
  if (sock < 0) return EXIT_FAILURE;
  err = listen(sock, SOMAXCONN);
  if (err != 0) return EXIT_FAILURE;
  fdinfo.cat->name = "control-listen";
  fdinfo.fd = sock;
  fdinfo.url = "control:listen:9051";
  evt_core_add_fd(&evts, &fdinfo);

  init_timer (&evts);
  socks5_server_init(&evts);
  tor_ctl_server_init (&evts);

  evt_core_loop (&evts);

  return EXIT_SUCCESS;
}
