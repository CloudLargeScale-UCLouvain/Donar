// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "tor_ctl.h"
#include "timer.h"

struct os_connect {
  char host[256];
  char port[6];
  char url[512];
};

enum DONAR_TIMER_DECISION on_os_connect_timeout(struct evt_core_ctx* ctx, void* user_data) {
  struct os_connect *oc = user_data;
  struct evt_core_fdinfo newfdinfo;
  struct evt_core_cat newcat;
  newfdinfo.cat = &newcat;
  newfdinfo.url = oc->url;

  int fd = create_tcp_client(oc->host, oc->port);
  if (fd <= 0) return DONAR_TIMER_CONTINUE;

  newfdinfo.fd = fd;
  newfdinfo.cat->name = "torctl-server-success";
  struct evt_core_fdinfo *reg = evt_core_add_fd (ctx, &newfdinfo);

  printf("[%s][torctl] onion service %s up (cat: %s, fd: %d)\n", current_human_datetime (), reg->url, reg->cat->name, reg->fd);
  free(oc);
  return DONAR_TIMER_STOP;
}

int tor_ctl_connect(struct tor_ctl* ctx, char* addr, char* service) {
  int sock = create_tcp_client (addr, service);
  int sock2 = dup(sock);
  ctx->rsock = NULL;
  ctx->wsock = NULL;

  ctx->rsock = fdopen(sock, "r");
  if (ctx->rsock == NULL) {
    return -1;
  }
  setbuf(ctx->rsock, NULL);

  ctx->wsock = fdopen(sock2, "w");
  if (ctx->wsock == NULL) {
    return -1;
  }
  setbuf(ctx->wsock, NULL);

  fprintf (ctx->wsock, "authenticate \"\"\n");
  int error_code = 0;
  fscanf (ctx->rsock, "%d", &error_code);
  if (error_code != 250) {
    tor_ctl_close (ctx);
    return -1;
  }
  fscanf(ctx->rsock," OK");

  return 0;
}

void tor_ctl_list_onions(struct tor_ctl* ctx) {
  fprintf(ctx->wsock, "getinfo onions/current\n");

  char delimiter, buffer[1024] = {0};
  fscanf(ctx->rsock, " 250%c", &delimiter);
  if (delimiter == '-') {
    fscanf(ctx->rsock, "onions/current=%s", buffer);
    if (strlen(buffer) > 0) printf("key: %s\n", buffer);
  } else if (delimiter == '+') {
    fscanf(ctx->rsock, "onions/current= ");
    while (1) {
      fgets(buffer, 1024, ctx->rsock);
      if (strcmp(buffer, "250 OK\r\n") == 0) break;
      if (strcmp(buffer, ".\r\n") == 0) continue;
      printf("line: %s", buffer);
    }
  } else {
    printf("deli:%c\n", delimiter);
  }
}

int tor_ctl_add_onion(struct tor_ctl* ctx, struct tor_os_str* tos, uint16_t* port, uint64_t port_per_os, enum TOR_ONION_FLAGS flags) {
  int err = 0;
  char buffer1[1024] = {0};
  char buffer2[1024] = {0};
  int to_create = tos->size - tos->filled;

  /* Add onion services loaded from file */
  for (int i = 0; i < tos->filled; i++) {
    fprintf(ctx->wsock, "add_onion %s ", tos->keys[i].priv);
    for (int j = 0; j < port_per_os; j++) {
      fprintf(ctx->wsock, "Port=%d,%s:%d ", port[i*port_per_os+j], ctx->os_endpoint, port[i*port_per_os+j]);
    }
    if (flags == TOR_ONION_FLAG_NONE) fprintf(ctx->wsock, "\n");
    else {
      fprintf(ctx->wsock, "Flags=");
      if (flags & TOR_ONION_FLAG_NON_ANONYMOUS) fprintf(ctx->wsock, "NonAnonymous,");
      fprintf(ctx->wsock, "\n");
    }

    fscanf(ctx->rsock, "%d", &err);
    if (err != 250) {
      printf("err: %d\n", err);
      return -1;
    }
    fscanf(ctx->rsock, "-ServiceID=%s\n", buffer1);
    printf("Added onion service %s.onion from file\n", buffer1);
    fscanf(ctx->rsock, "250 OK");
  }

  /* Complete by creating new onion services */
  for (int i = tos->filled; i < tos->size; i++) {
    fprintf(ctx->wsock, "add_onion NEW:ED25519-V3 ");
    for (int j = 0; j < port_per_os; j++) {
      fprintf(ctx->wsock, "Port=%d,%s:%d ", port[i*port_per_os+j], ctx->os_endpoint, port[i*port_per_os+j]);
    }
    if (flags == TOR_ONION_FLAG_NONE) fprintf(ctx->wsock, "\n");
    else {
      fprintf(ctx->wsock, "Flags=");
      if (flags & TOR_ONION_FLAG_NON_ANONYMOUS) fprintf(ctx->wsock, "NonAnonymous,");
      fprintf(ctx->wsock, "\n");
    }

    //fprintf(ctx->wsock, "add_onion NEW:RSA1024 Port=%d\n", port[i]);

    err = 0;
    fscanf(ctx->rsock, "%d", &err);

    if (err != 250) {
      fprintf(stderr, "Got error %d instead of 250\n", err);
      return -2;
    }
    err = fscanf(ctx->rsock, "-ServiceID=%s\n", buffer1);
    if (err <= 0) return -3;
    printf("Created onion service %s.onion\n", buffer1);
    err = fscanf(ctx->rsock, "250-PrivateKey=%s\n", buffer2);
    if (err <= 0) return -4;
    //printf("Onion service private key: %s\n", buffer);
    if (tor_os_append(tos, buffer1, buffer2) != 0) return -5;
    err = fscanf(ctx->rsock, "250 OK");
    if (err < 0) return -6;
  }

  if (to_create > 0) {
    tor_os_persist(tos);
  }

  return 0;
}

void tor_ctl_close(struct tor_ctl* ctx) {
  fclose(ctx->rsock);
  fclose(ctx->wsock);
}

int on_torctl_server_auth_read(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  char *expected = "authenticate \"\"\n";
  size_t to_read = strlen(expected);
  char buffer[128] = {0};
  ssize_t nread = recv(fdinfo->fd, buffer, to_read, MSG_PEEK);
  if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (nread != to_read) return EVT_CORE_FD_EXHAUSTED;

  recv(fdinfo->fd, buffer, to_read, 0);
  if (strstr(buffer, "authenticate") == NULL) {
    fprintf(stderr, "Unable to find string 'authenticate' in receveived command: '%s'\n", buffer);
    exit(EXIT_FAILURE);
  }

  evt_core_mv_fd2 (ctx, fdinfo, "torctl-server-auth-write");
  return EVT_CORE_FD_EXHAUSTED;
}

int on_torctl_server_auth_write(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  char *response = "250 OK\r\n";
  ssize_t nwrite = send(fdinfo->fd, response, sizeof(response), 0);
  if (nwrite != sizeof(response)) {
    perror("@FIXME: Unproper handling of sockets in torctl_server_auth_write.");
    exit(EXIT_FAILURE);
  }
  evt_core_mv_fd2 (ctx, fdinfo, "torctl-server-add-onion-read");
  return EVT_CORE_FD_EXHAUSTED;
}

int on_torctl_server_add_onion_read(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  char buffer[1024] = {0};
  char *strtok_ptr, *str_target, *token;
  int i, port1;
  char *service_id = "iu7aeep42k5ky3fwcfag5el2raelfcwuilsstqhcz3c6bmxilr2nuayd.onion"; //@FIXME hardcoded url

  ssize_t nread = recv(fdinfo->fd, buffer, sizeof(buffer), MSG_PEEK);
  if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (nread == -1) {
    perror("an error occured...");
    exit(EXIT_FAILURE);
  }
  if (buffer[nread-1] != '\n') return EVT_CORE_FD_EXHAUSTED;
  nread = recv(fdinfo->fd, buffer, sizeof(buffer), 0);
  printf("[%s][torctl] Received command: %s\n", current_human_datetime (), buffer);

  for (i = 0, str_target = buffer ; ; str_target = NULL, i++) {
    token = strtok_r(str_target, " ", &strtok_ptr);
    if (i == 0 && (token == NULL || strcmp(token, "add_onion") != 0)) {
      fprintf(stderr, "wrong command. expected 'add_onion' but got '%s'\n", token);
      exit(EXIT_FAILURE);
    }
    if (i == 1 && token == NULL) {
      fprintf(stderr, "command add_onion requires at least one parameter, specifying key algo\n");
      exit(EXIT_FAILURE);
    }
    if (i < 2) continue;
    if (token == NULL) break;

    struct os_connect *oc = malloc(sizeof(struct os_connect));
    int captured = sscanf(token, "Port=%d,%[^:]:%s", &port1, oc->host, oc->port);
    if (captured != 3) {
      free(oc);
      continue;
    }

    sprintf(oc->url, "torctl:%s:%d", service_id, port1);
    printf("[%s][torctl] will create onion service %s:%d <-> %s:%s in background (%s)\n", current_human_datetime (), service_id, port1, oc->host, oc->port, oc->url);
    set_timeout(ctx, 100, oc, on_os_connect_timeout);
  }

  evt_core_mv_fd2 (ctx, fdinfo, "torctl-server-add-onion-write");
  return EVT_CORE_FD_EXHAUSTED;
}

int on_torctl_server_add_onion_write(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  //@FIXME hardcoded response
  char *answer = "250-ServiceID=iu7aeep42k5ky3fwcfag5el2raelfcwuilsstqhcz3c6bmxilr2nuayd\r\n250-PrivateKey=ED25519-V3:ULk3Q/TFqngKCDDzeM93YC80IDOjz13PKTx718UjE0Svf+u/QZmN9EHzUCqCa1ZkNAXSQJIzcOVeJ8OL8Zg5Xg==\r\n250 OK\r\n";

  ssize_t nwrite;
  nwrite = send(fdinfo->fd, answer, strlen(answer), 0);
  if (nwrite != strlen(answer)) goto error;

  printf("[%s][torctl] Sent add-onion reply\n", current_human_datetime ());
  evt_core_mv_fd2(ctx,fdinfo,"torctl-server-add-onion-read");
  return EVT_CORE_FD_EXHAUSTED;

error:
  perror("@FIXME: unproper handling of non blocking sockets, you have been bitten in torctl_server_add_onion_write\n");
  exit(EXIT_FAILURE);
}

void tor_ctl_server_init(struct evt_core_ctx *ctx) {
  struct evt_core_cat template = {0};

  template.cb = on_torctl_server_auth_read;
  template.err_cb = NULL;
  template.name = "torctl-server-auth-read";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_torctl_server_auth_write;
  template.err_cb = NULL;
  template.name = "torctl-server-auth-write";
  template.flags = EPOLLOUT | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_torctl_server_add_onion_read;
  template.err_cb = NULL;
  template.name = "torctl-server-add-onion-read";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_torctl_server_add_onion_write;
  template.err_cb = NULL;
  template.name = "torctl-server-add-onion-write";
  template.flags = EPOLLOUT | EPOLLET;
  evt_core_add_cat (ctx, &template);
}

void tor_ctl_server_handle(struct evt_core_ctx *ctx, int fd) {
  struct evt_core_fdinfo *reg_fdinfo;
  struct evt_core_fdinfo fdinfo;
  struct evt_core_cat cat;
  char url[256];

  fdinfo.cat = &cat;
  fdinfo.cat->name = "torctl-server-auth-read";
  fdinfo.fd = fd;
  fdinfo.other = NULL;
  fdinfo.free_other = NULL;
  sprintf(url, "tor-ctl-server:%d", fd);
  fdinfo.url = url;

  reg_fdinfo = evt_core_add_fd (ctx, &fdinfo);
}
