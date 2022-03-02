// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "socks5.h"

void socks5_free_ctx(void* elem) {
  struct socks5_ctx* ctx = elem;
  if (ctx->addr != NULL) free(ctx->addr);
  free(ctx);
}

void socks5_create_dns_client(struct evt_core_ctx* ctx, char* proxy_host, char* proxy_port, char* addr, uint16_t port) {
  struct evt_core_fdinfo fdinfo;
  struct evt_core_cat cat;
  struct socks5_ctx* s5ctx;
  struct evt_core_fdinfo* reg_fdinfo;
  char url[1024];

  // 0. Compute domain length and enforce an upper bound on its size
  size_t domainLength = strlen(addr);
  if (domainLength > 255) {
    fprintf(stderr, "domain is too long\n");
    exit(EXIT_FAILURE);
  }

  // 1. Open connection
  int sock = create_tcp_client (proxy_host, proxy_port);
  if (sock < 0) {
    fprintf(stderr, "Unable to connect to proxy %s:%s\n", proxy_host, proxy_port);
    exit(EXIT_FAILURE);
  }

  // 2. Create fdinfo
  fdinfo.cat = &cat;
  fdinfo.cat->name = "socks5-send-handshake";
  fdinfo.fd = sock;
  fdinfo.other = malloc(sizeof(struct socks5_ctx));
  if (fdinfo.other == NULL) {
    perror("malloc failed");
    exit(EXIT_FAILURE);
  }
  memset(fdinfo.other, 0, sizeof(struct socks5_ctx));
  fdinfo.free_other = socks5_free_ctx;
  sprintf(url, "socks5:send-hs:%s:%d", addr, port);
  fdinfo.url = url;

  // 3. Fill socks5_ctx structures
  s5ctx = fdinfo.other;
  s5ctx->port = port;
  s5ctx->addr = strdup(addr);

  // 3.1 Client handshake to send
  s5ctx->ch.ver = VER_SOCKS5;
  s5ctx->ch.nmethods = 0x01;
  s5ctx->ch.methods[0] = METHOD_NOAUTH;
  s5ctx->ch_size = sizeof(uint8_t) * (2 + s5ctx->ch.nmethods);

  // 3.2 Client request to send
  s5ctx->cr.ver = VER_SOCKS5;
  s5ctx->cr.cmd = CMD_CONNECT;
  s5ctx->cr.rsv = 0x00;
  s5ctx->cr.atyp = ATYP_DOMAINNAME;
  s5ctx->cr.dest_addr.dns.len = domainLength;
  strcpy((char*)&s5ctx->cr.dest_addr.dns.str, addr);
  s5ctx->cr.port = htons(port);

  // 3.3 Generate client request buffer
  s5ctx->cr_size = 0;
  fill_buffer2(&s5ctx->cr_size, s5ctx->cr_buffer, &s5ctx->cr, &s5ctx->cr.dest_addr.dns.str);
  fill_buffer(&s5ctx->cr_size, s5ctx->cr_buffer, &s5ctx->cr.dest_addr.dns.str, s5ctx->cr.dest_addr.dns.len);
  fill_buffer2(&s5ctx->cr_size, s5ctx->cr_buffer, &s5ctx->cr.port, &s5ctx->cr + 1);

  reg_fdinfo = evt_core_add_fd (ctx, &fdinfo);
}

int on_socks5_send_handshake(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;

  size_t written = write(fdinfo->fd, (char*)&s5ctx->ch + s5ctx->ch_cursor, s5ctx->ch_size - s5ctx->ch_cursor);
  if (written == -1 && errno == EAGAIN) return 1;
  if (written < 0) {
    perror("write failed on tcp socket in socks5");
    evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
    return 1;
  }
  s5ctx->ch_cursor += written;
  if (s5ctx->ch_cursor < s5ctx->ch_size) return 0;

  evt_core_mv_fd2(ctx, fdinfo, "socks5-recv-handshake");
  return 1;
}

int on_socks5_recv_handshake(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  int readn = 0;

  readn = read(fdinfo->fd, (char*)&s5ctx->sh + s5ctx->sh_cursor, sizeof(s5ctx->sh) - s5ctx->sh_cursor);
  if (readn == -1 && errno == EAGAIN) return 1;
  if (readn < 0) {
    perror("sock5 handshake failed read");
    evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
    return 1;
  }

  s5ctx->sh_cursor += readn;
  if (s5ctx->sh_cursor < sizeof(s5ctx->sh)) return 0;

  if (s5ctx->ch.ver != s5ctx->sh.ver || s5ctx->sh.method != s5ctx->ch.methods[0]) {
    fprintf(stderr, "Protocol error: client asks for ver=%d, method=%d and server answers with ver=%d, method=%d\n",
              s5ctx->ch.ver, s5ctx->ch.methods[0], s5ctx->sh.ver, s5ctx->sh.method);
    evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
    return 1;
  }
  printf("[socks5_server_handshake] fd=%d, ver=%d, method=%d\n", fdinfo->fd, s5ctx->sh.ver, s5ctx->sh.method);
  evt_core_mv_fd2(ctx, fdinfo, "socks5-send-client-req");
  return 1;
}

int on_socks5_send_client_req(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  int written = 0;

  written = write(fdinfo->fd, (char*)s5ctx->cr_buffer + s5ctx->cr_cursor, s5ctx->cr_size - s5ctx->cr_cursor);
  if (written == -1 && errno == EAGAIN) return 1;
  if (written < 0) {
    fprintf(stderr, "socks5 send client request failed\n");
    evt_core_mv_fd2 (ctx, fdinfo, "socks5-failed");
    return 1;
  }
  s5ctx->cr_cursor += written;
  printf("[socks5_server_send_client_req] sent %ld/%ld\n", s5ctx->cr_cursor, s5ctx->cr_size);
  if (s5ctx->cr_cursor < s5ctx->cr_size) return 0; // Trigger loop to trigger EAGAIN.

  evt_core_mv_fd2 (ctx, fdinfo, "socks5-recv-server-reply");
  return 1;
}

int socks5_server_reply_atyp_ipv4(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  size_t fixed_headers_size = (char*)&s5ctx->sr.bind_addr.ipv4 - (char*)&s5ctx->sr;
  size_t host_size = sizeof(s5ctx->sr.bind_addr.ipv4);
  uint64_t relative_cursor = (s5ctx->sr_cursor - fixed_headers_size);
  int nread = 0;

  nread = read(fdinfo->fd,
       (char*)s5ctx->sr.bind_addr.ipv4 + relative_cursor,
       host_size - relative_cursor);

  if (nread == -1 && errno == EAGAIN) return 1;
  if (nread < 0) {
    perror("write failed on tcp socket in socks5");
    evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
    return 1;
  }

  s5ctx->sr_cursor += nread;
  if (s5ctx->sr_cursor < fixed_headers_size + host_size) return 0;

  s5ctx->sr_host_read = 1;
  return 0;
}

int socks5_server_reply_atyp_ipv6(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  size_t fixed_headers_size = (char*)&s5ctx->sr.bind_addr.ipv6 - (char*)&s5ctx->sr;
  size_t host_size = sizeof(s5ctx->sr.bind_addr.ipv6);
  uint64_t relative_cursor = (s5ctx->sr_cursor - fixed_headers_size);
  int nread = 0;

  nread = read(fdinfo->fd,
       (char*)s5ctx->sr.bind_addr.ipv6 + relative_cursor,
       host_size - relative_cursor);

  if (nread == -1 && errno == EAGAIN) return 1;
  if (nread < 0) {
    perror("write failed on tcp socket in socks5");
    evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
    return 1;
  }
  s5ctx->sr_cursor += nread;
  if (s5ctx->sr_cursor < fixed_headers_size + host_size) return 0;

  s5ctx->sr_host_read = 1;
  return 0;
}

int socks5_server_reply_atyp_dn(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  size_t fixed_headers_size = (char*)&s5ctx->sr.bind_addr.ipv6 - (char*)&s5ctx->sr;
  size_t dn_size_size = sizeof(s5ctx->sr.bind_addr.dns.len);
  uint64_t relative_cursor = 0;
  int nread = 0;

  if (s5ctx->sr_cursor < fixed_headers_size + dn_size_size) {
    relative_cursor = (s5ctx->sr_cursor - fixed_headers_size);
    nread = read(fdinfo->fd, (char*)&s5ctx->sr.bind_addr.dns.len + relative_cursor, dn_size_size - relative_cursor);
    if (nread == -1 && errno == EAGAIN) return 1;
    if (nread < 0) {
      perror("write failed on tcp socket in socks5");
      evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
      return 1;
    }
    s5ctx->sr_cursor += nread;
    return 0;
  }

  relative_cursor = s5ctx->sr_cursor - fixed_headers_size - sizeof(s5ctx->sr.bind_addr.dns.len);
  nread = read(fdinfo->fd, (char*)&s5ctx->sr.bind_addr.dns.str + relative_cursor, s5ctx->sr.bind_addr.dns.len - relative_cursor);
  if (nread == -1 && errno == EAGAIN) return 1;
  if (nread < 0) {
    perror("write failed on tcp socket in socks5");
    evt_core_mv_fd2(ctx, fdinfo, "socks5-failed");
    return 1;
  }
  s5ctx->sr_cursor += nread;
  if (s5ctx->sr_cursor < fixed_headers_size + dn_size_size + s5ctx->sr.bind_addr.dns.len) return 0;

  s5ctx->sr_host_read = 1;
  return 0;
}

size_t socks5_server_reply_size(struct server_reply* sr) {
  size_t fixed_headers_size = (char*)&sr->bind_addr - (char*)sr;
  size_t fixed_tail_size = (char*)(sr + 1) - (char*)&sr->port;
  size_t host_size = 0;

  if (sr->atyp == ATYP_IPV4) {
    host_size = sizeof(sr->bind_addr.ipv4);
  } else if (sr->atyp == ATYP_IPV6) {
    host_size = sizeof(sr->bind_addr.ipv6);
  } else if (sr->atyp == ATYP_DOMAINNAME) {
    host_size = sizeof(sr->bind_addr.dns.len) + sr->bind_addr.dns.len;
  } else {
    fprintf(stderr, "Unsupported ATYP for SOCK5\n");
    exit(EXIT_FAILURE);
  }

  return fixed_headers_size + host_size + fixed_tail_size;
}

int on_socks5_recv_server_reply(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  int readn = 0;
  size_t fixed_headers_size = (char*)&s5ctx->sr.bind_addr - (char*)&s5ctx->sr;

  // Read headers
  if (s5ctx->sr_cursor < fixed_headers_size) {
    printf("  [socks5] read headers (current: %ld/%ld - %ld to read)\n", s5ctx->sr_cursor, fixed_headers_size, fixed_headers_size - s5ctx->sr_cursor);
    readn = read(fdinfo->fd, (char*)&s5ctx->sr + s5ctx->sr_cursor, fixed_headers_size - s5ctx->sr_cursor);
    if (readn == -1 && errno == EAGAIN) return 1;
    if (readn < 0) goto move_to_failed;
    s5ctx->sr_cursor += readn;
    return 0; // Needed as we might have not read enough bytes and free us from writing a loop
  }

  // Read host
  if (!s5ctx->sr_host_read) {
    printf("  [socks5] read host\n");
    if (s5ctx->sr.atyp == ATYP_IPV4) return socks5_server_reply_atyp_ipv4(ctx, fdinfo);
    else if (s5ctx->sr.atyp == ATYP_IPV6) return socks5_server_reply_atyp_ipv6(ctx, fdinfo);
    else if (s5ctx->sr.atyp == ATYP_DOMAINNAME) return socks5_server_reply_atyp_dn(ctx, fdinfo);
    else goto move_to_failed;
  }

  // Read port
  size_t final_size = socks5_server_reply_size(&s5ctx->sr);
  if (s5ctx->sr_cursor < final_size) {
    printf("  [socks5] read port\n");
    size_t relative_cursor = s5ctx->sr_cursor - (final_size - sizeof(s5ctx->sr.port));
    readn = read(fdinfo->fd, (char*)&s5ctx->sr.port + relative_cursor, sizeof(s5ctx->sr.port) - relative_cursor);
    if (readn == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (readn < 0) goto move_to_failed;
    s5ctx->sr_cursor += readn;
    return EVT_CORE_FD_UNFINISHED; // Needed as we might have not read enough bytes and free us from writing a loop
  }

  // Do some checks
  if (s5ctx->sr.rep > 0x08) goto move_to_failed;
  printf("[socks5_server_reply] fd=%d, ver=%d, rep=%s, atyp=%d, port=%d\n", fdinfo->fd, s5ctx->sr.ver, rep_msg[s5ctx->sr.rep], s5ctx->sr.atyp, s5ctx->sr.port);

  if (s5ctx->sr.rep != SOCKS5_REP_SUCCESS) goto move_to_failed;

  evt_core_mv_fd2 (ctx, fdinfo, "socks5-success");
  return 1;
move_to_failed:
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-failed");
  return 1;
}

int on_socks5_err(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-failed");
  return 1;
}

void socks5_init(struct evt_core_ctx* ctx) {
  struct evt_core_cat template = {0};

  template.cb = on_socks5_send_handshake;
  template.err_cb = on_socks5_err;
  template.name = "socks5-send-handshake";
  template.flags = EPOLLOUT | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_socks5_recv_handshake;
  template.err_cb = on_socks5_err;
  template.name = "socks5-recv-handshake";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_socks5_send_client_req;
  template.err_cb = on_socks5_err;
  template.name = "socks5-send-client-req";
  template.flags = EPOLLOUT | EPOLLET;
  evt_core_add_cat(ctx, &template);

  template.cb = on_socks5_recv_server_reply;
  template.err_cb = on_socks5_err;
  template.name =  "socks5-recv-server-reply";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(ctx, &template);
}

char* socks5_rep (enum socks5_rep rep) {
  return rep_msg[rep];
}

void socks5_server_handle_req(struct evt_core_ctx* ctx, int fd) {
  struct evt_core_fdinfo fdinfo;
  struct evt_core_cat cat;
  struct socks5_ctx* s5ctx;
  struct evt_core_fdinfo* reg_fdinfo;
  char url[1024];

  // 1. Create fdinfo
  fdinfo.cat = &cat;
  fdinfo.cat->name = "socks5-server-recv-handshake";
  fdinfo.fd = fd;
  fdinfo.other = malloc(sizeof(struct socks5_ctx));
  if (fdinfo.other == NULL) {
    perror("malloc failed");
    exit(EXIT_FAILURE);
  }
  memset(fdinfo.other, 0, sizeof(struct socks5_ctx));
  fdinfo.free_other = socks5_free_ctx;
  sprintf(url, "socks5:%d", fd);
  fdinfo.url = url;

  // 2. Configure our context
  s5ctx = fdinfo.other;
  s5ctx->port = 0;
  s5ctx->addr = NULL;

  // 3. Set our handshake answer
  s5ctx->sh.ver = VER_SOCKS5;
  s5ctx->sh.method = METHOD_NOAUTH;

  // 4. Set our CONNECT reply (yes we hardcode a lot, shame on me)
  s5ctx->sr.ver = VER_SOCKS5;
  s5ctx->sr.rep = SOCKS5_REP_SUCCESS;
  s5ctx->sr.rsv = 0x00;
  s5ctx->sr.atyp = ATYP_IPV4;
  if (inet_aton("127.0.0.1", (struct in_addr*) &s5ctx->sr.bind_addr.ipv4) == 0) goto error;
  s5ctx->sr.port = htons(0);

  // 5 Generate server reply buffer
  s5ctx->sr_size = 0;
  fill_buffer2(&s5ctx->sr_size, s5ctx->sr_buffer, &s5ctx->sr, &s5ctx->sr.bind_addr);
  fill_buffer2(&s5ctx->sr_size, s5ctx->sr_buffer, &s5ctx->sr.bind_addr.ipv4, &s5ctx->sr.bind_addr.ipv4 + 1);
  fill_buffer2(&s5ctx->sr_size, s5ctx->sr_buffer, &s5ctx->sr.port, &s5ctx->sr + 1);

  reg_fdinfo = evt_core_add_fd (ctx, &fdinfo);

  return;
error:
  perror("failed to init socks5 server.");
  exit(EXIT_FAILURE);
}

int on_socks5_server_recv_handshake(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;

  // 1. We need to read more
  if (s5ctx->ch_cursor < 2 || s5ctx->ch_cursor < s5ctx->ch.nmethods + 2) {
    size_t to_read = 0;
    if (s5ctx->ch_cursor < 2) to_read = 2 - s5ctx->ch_cursor;
    else to_read = s5ctx->ch.nmethods - (s5ctx->ch_cursor - 2);
    ssize_t nread = recv(fdinfo->fd, ((void*)&s5ctx->ch) + s5ctx->ch_cursor, to_read, 0);
    if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (nread == -1) goto serv_handshake_err;
    s5ctx->ch_cursor += nread;
    return EVT_CORE_FD_UNFINISHED;
  }

  printf("[%s][socks5] read client handshake version: %d, nmethods: %d\n", current_human_datetime (), s5ctx->ch.ver, s5ctx->ch.nmethods);
  for (int i = 0; i < s5ctx->ch.nmethods; i++) {
    if (s5ctx->ch.methods[i] == METHOD_NOAUTH) goto success;
  }

  fprintf(stderr, "Unable to find a NOAUTH method in the list of available method\n");
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-failed");
  return EVT_CORE_FD_EXHAUSTED;

success:
  printf("[%s][socks5] received client handshake\n", current_human_datetime ());
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-send-handshake");
  return EVT_CORE_FD_EXHAUSTED; // Success, we are compatible with client, moving to next state

serv_handshake_err:
  perror("[socks5] unable to read handshake from socket.");
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-failed");
  return EVT_CORE_FD_EXHAUSTED;
}

int on_socks5_server_send_handshake(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;

  if (s5ctx->sh_cursor < sizeof(s5ctx->sh)) {
    ssize_t nsent = send(fdinfo->fd, &s5ctx->sh, sizeof(s5ctx->sh) - s5ctx->sh_cursor, 0);
    if (nsent == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (nsent == -1) goto send_hs_error;
    s5ctx->sh_cursor += nsent;
    return EVT_CORE_FD_UNFINISHED;
  }

  printf("[%s][socks5] sent server handshake\n", current_human_datetime ());
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-recv-client-req");
  return EVT_CORE_FD_EXHAUSTED;

send_hs_error:
  perror("[socks5] unable to send handshake to socket.");
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-failed");
  return EVT_CORE_FD_EXHAUSTED;
}

int on_socks5_server_recv_client_req(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  ssize_t nread = 0;
  size_t to_read = 0;

  size_t fixed_size = ((void*)&s5ctx->cr.dest_addr) - ((void*)&s5ctx->cr);
  if (s5ctx->cr.atyp == 0x00) {
    to_read = fixed_size - s5ctx->cr_cursor;
    nread = recv(fdinfo->fd, ((void*)&s5ctx->cr) + s5ctx->cr_cursor, to_read, 0);
    if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (nread == -1) goto recv_client_req_error;
    s5ctx->cr_cursor += nread;
    if (nread == to_read) s5ctx->cr_cursor = 0;
    else return EVT_CORE_FD_UNFINISHED;
  }

  if (!s5ctx->cr_host_read && s5ctx->cr.atyp == ATYP_DOMAINNAME) {
    to_read = sizeof(s5ctx->cr.dest_addr.dns.len) + s5ctx->cr.dest_addr.dns.len - s5ctx->cr_cursor;
    nread = recv(fdinfo->fd, ((void*)&s5ctx->cr.dest_addr.dns) + s5ctx->cr_cursor, to_read, 0);
    if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (nread == -1) goto recv_client_req_error;
    s5ctx->cr_cursor += nread;
    if (s5ctx->cr_cursor >= sizeof(s5ctx->cr.dest_addr.dns.len) + s5ctx->cr.dest_addr.dns.len) {
      s5ctx->cr_host_read = TRUE;
      s5ctx->cr_cursor = 0;
    } else return EVT_CORE_FD_UNFINISHED;
  } else if (!s5ctx->cr_host_read && s5ctx->cr.atyp == ATYP_IPV4) {
    to_read = sizeof(s5ctx->cr.dest_addr.ipv4) - s5ctx->cr_cursor;
    nread = recv(fdinfo->fd, ((void*)&s5ctx->cr.dest_addr.ipv4) + s5ctx->cr_cursor, to_read, 0);
    if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (nread == -1) goto recv_client_req_error;
    s5ctx->cr_cursor += nread;
    if (to_read == nread) {
      s5ctx->cr_host_read = TRUE;
      s5ctx->cr_cursor = 0;
    } else return EVT_CORE_FD_UNFINISHED;
  } else if (!s5ctx->cr_host_read && s5ctx->cr.atyp == ATYP_IPV6) {
    to_read = sizeof(s5ctx->cr.dest_addr.ipv6) - s5ctx->cr_cursor;
    nread = recv(fdinfo->fd, ((void*)&s5ctx->cr.dest_addr.ipv6) + s5ctx->cr_cursor, to_read, 0);
    if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
    if (nread == -1) goto recv_client_req_error;
    s5ctx->cr_cursor += nread;
    if (to_read == nread) {
      s5ctx->cr_host_read = TRUE;
      s5ctx->cr_cursor = 0;
    } else return EVT_CORE_FD_UNFINISHED;
  } else if (!s5ctx->cr_host_read) goto recv_client_req_error;

  to_read = (void*)(&s5ctx->cr + 1) - ((void*)&s5ctx->cr.port) - s5ctx->cr_cursor;
  nread = recv(fdinfo->fd, ((void*)&s5ctx->cr.port) + s5ctx->cr_cursor, to_read, 0);
  if (nread == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (nread == -1) goto recv_client_req_error;
  s5ctx->cr_cursor += nread;
  if (to_read != nread) return EVT_CORE_FD_UNFINISHED;

  char buffer[1024];
  if (s5ctx->cr.atyp == ATYP_DOMAINNAME) {
    sprintf(buffer, "socket:%s:%d", s5ctx->cr.dest_addr.dns.str, ntohs(s5ctx->cr.port));
    evt_core_fdinfo_url_set(fdinfo, buffer);
    printf("new socket name: %s\n", fdinfo->url);
  }

  printf("[%s][socks5] received client request atyp=%s\n", current_human_datetime (), atyp_str[s5ctx->cr.atyp]);
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-send-server-reply");
  return EVT_CORE_FD_EXHAUSTED;

recv_client_req_error:
  perror("[socks5] unable to receive client request from socket.");
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-failed");
  return EVT_CORE_FD_EXHAUSTED;
}

int on_socks5_server_send_server_reply(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct socks5_ctx* s5ctx = fdinfo->other;
  ssize_t nsent = 0;
  size_t to_send = s5ctx->sr_size - s5ctx->sr_cursor;

  nsent = send(fdinfo->fd, s5ctx->sr_buffer + s5ctx->sr_cursor, to_send, 0);
  if (nsent == -1 && errno == EAGAIN) return EVT_CORE_FD_EXHAUSTED;
  if (nsent == -1) goto send_server_rep_error;
  s5ctx->sr_cursor += nsent;

  if (nsent != to_send) return EVT_CORE_FD_UNFINISHED;

  printf("[%s][socks5] sent server reply\n", current_human_datetime ());
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-success");
  return EVT_CORE_FD_EXHAUSTED;

send_server_rep_error:
  perror("[socks5] unable to send server reply to socket.");
  evt_core_mv_fd2 (ctx, fdinfo, "socks5-server-failed");
  return EVT_CORE_FD_EXHAUSTED;
}

void socks5_server_init(struct evt_core_ctx* ctx) {
  struct evt_core_cat template = {0};

  template.cb = on_socks5_server_recv_handshake;
  template.err_cb = on_socks5_err;
  template.name = "socks5-server-recv-handshake";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_socks5_server_send_handshake;
  template.err_cb = on_socks5_err;
  template.name = "socks5-server-send-handshake";
  template.flags = EPOLLOUT | EPOLLET;
  evt_core_add_cat (ctx, &template);

  template.cb = on_socks5_server_recv_client_req;
  template.err_cb = on_socks5_err;
  template.name = "socks5-server-recv-client-req";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(ctx, &template);

  template.cb = on_socks5_server_send_server_reply;
  template.err_cb = on_socks5_err;
  template.name =  "socks5-server-send-server-reply";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(ctx, &template);
}
