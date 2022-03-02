// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

  #include <stdlib.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "evt_core.h"
#include "net_tools.h"
#include "socks5.h"
#include "utils.h"
#include "measure.h"
#include "url.h"
#include "tor_os.h"
#include "tor_ctl.h"

struct measlat_ctx {
  struct sockaddr_in addr;
  socklen_t addrlen;
  int verbose, connectionless, tor_flags;
  char *host, *port, *transport, *tor_port;
  enum { MEASLAT_CLIENT = 0, MEASLAT_SERVER = 1 } role;
  struct measure_params mp;
};

void free_ms(void* obj) {
  measure_state_free (obj);
  free(obj);
}

struct measure_state ms_transi = {0};

int streq(char* s1, char* s2) {
  return strcmp(s1, s2) == 0;
}

struct evt_core_fdinfo* register_timer(struct evt_core_ctx* evts, struct measlat_ctx* mctx, struct measure_state* ms, struct timespec* next_tick) {
  struct timespec now;
  struct itimerspec timer_config;
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};

  struct measure_state* msheap = malloc(sizeof(struct measure_state));
  if (ms == NULL) {
    perror("unable to malloc struct timer_measure");
    exit(EXIT_FAILURE);
  }
  memcpy(msheap, ms, sizeof(struct measure_state));

  fdinfo.cat = &cat;
  fdinfo.url = url;
  fdinfo.other = msheap;
  fdinfo.free_other = free_ms;

  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
    perror("clock_gettime");
    exit(EXIT_FAILURE);
  }
  uint64_t micro_sec = mctx->mp.interval;
  timer_config.it_value.tv_sec = next_tick == NULL ? now.tv_sec + 1 : next_tick->tv_sec;
  timer_config.it_value.tv_nsec = next_tick == NULL ? now.tv_nsec : next_tick->tv_nsec;
  timer_config.it_interval.tv_sec = micro_sec / 1000;
  timer_config.it_interval.tv_nsec = micro_sec % 1000 * 1000000;

  printf("timer_config: sec=%ld nsec=%ld \n", (uint64_t) timer_config.it_value.tv_sec, (uint64_t) timer_config.it_value.tv_nsec);

  fdinfo.fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fdinfo.fd == -1) {
    perror("Unable to timerfd_create");
    exit(EXIT_FAILURE);
  }
  if (timerfd_settime (fdinfo.fd, TFD_TIMER_ABSTIME, &timer_config, NULL) == -1) {
    perror("Unable to timerfd_time");
    exit(EXIT_FAILURE);
  }
  fdinfo.cat->name = "timer";
  sprintf(fdinfo.url, "timer:%d", msheap->fd);
  struct evt_core_fdinfo *new_fdinfo = evt_core_add_fd (evts, &fdinfo);
  printf("--- Timer registered\n");
  printf("[states] measurement %d+%d started\n", msheap->fd, fdinfo.fd);
  return new_fdinfo;
}

int on_receive_measure_packet_err(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct measlat_ctx* mctx = fdinfo->cat->app_ctx;
  printf("An error occured with socket %d\n", fdinfo->fd);
  char url[255];

  sprintf(url, "timer:%d", fdinfo->fd);
  struct evt_core_fdinfo* assoc_timer = evt_core_get_from_url (ctx, url);
  if (assoc_timer != NULL && !mctx->mp.probe && !mctx->connectionless) {
    if (mctx->role == MEASLAT_CLIENT && !mctx->mp.probe) measure_summary (&mctx->mp, assoc_timer->other);
    evt_core_rm_fd (ctx, assoc_timer->fd);
    printf("Deleted associated timer %s\n", url);
  } else {
    printf("No associated timer %s\n", url);
  }

  if (mctx->role == MEASLAT_CLIENT && !mctx->mp.probe) exit(EXIT_FAILURE);

  if (mctx->connectionless) return 1; // keep the NET FD
  else return 0; // delete the NET fd
}

void measlat_stop(
    struct evt_core_ctx* ctx,
    struct measlat_ctx* mctx,
    struct measure_state* ms,
    int net_fd, int timer_fd) {
  if (ms->mp_in->counter < mctx->mp.max_measure) return;
  if (ms->mp_out->counter < mctx->mp.max_measure) return;
  printf("[states] measurement %d+%d terminated\n", net_fd, timer_fd);
  if (mctx->role == MEASLAT_CLIENT && !mctx->mp.probe) measure_summary (&(mctx->mp), ms);
  evt_core_rm_fd(ctx, timer_fd);
  if (!(mctx->connectionless && mctx->role == MEASLAT_SERVER))
    evt_core_rm_fd(ctx, net_fd);
  if (mctx->role == MEASLAT_CLIENT)
    exit(EXIT_SUCCESS);
}

int on_receive_measure_packet(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct measlat_ctx* mctx = fdinfo->cat->app_ctx;
  struct measure_state* ms;
  ssize_t nread;
  char url[255];

  // 1. Get our measurement state
  sprintf(url, "timer:%d", fdinfo->fd);
  struct evt_core_fdinfo* assoc_timer = evt_core_get_from_url (ctx, url);
  if (assoc_timer) {
    // Measurement state exists
    ms = assoc_timer->other;
  } else {
    // Does not exist yet, we use a tmp stub.
    ms = &ms_transi;
    ms->fd = fdinfo->fd;
    ms->mp_nin = 0;
  }

  // 2. Read data in our measurement object
  nread = mctx->connectionless && mctx->role == MEASLAT_SERVER ?
    recvfrom(fdinfo->fd, ms->mp_in, mctx->mp.payload_size, MSG_TRUNC, (struct sockaddr*)&mctx->addr, &mctx->addrlen) :
    recv(fdinfo->fd, ((char*) ms->mp_in) + ms->mp_nin, mctx->mp.payload_size - ms->mp_nin, 0);

  if (nread <= 0) return 1;

  ms->mp_nin += nread;
  if (ms->mp_nin < mctx->mp.payload_size) return 0;

  // 3. Process data in our measurement object
  measure_parse (&mctx->mp, ms, mctx->verbose);

  // 4. Detect if it is a probe
  if (ms->mp_in->probe && mctx->role == MEASLAT_SERVER) { // Allow for probing without registering a timer
    int s = mctx->connectionless && mctx->role == MEASLAT_SERVER ?
      sendto(ms->fd, ms->mp_in, mctx->mp.payload_size, 0, (struct sockaddr*)&mctx->addr, mctx->addrlen) :
      send(ms->fd, ms->mp_in, mctx->mp.payload_size, 0);

    if (!mctx->connectionless)
      evt_core_rm_fd (ctx, fdinfo->fd);

    return 1;
  } else if (ms->mp_in->probe && mctx->role == MEASLAT_CLIENT) {
    if (ms->mp_out->probe) exit(EXIT_SUCCESS);
    else {
      fprintf(stderr, "lost probe, ignoring\n");
      return 0;
    }
  }


  // 5. Persist our measurement object if needed
  // It includes starting a timer.
  if (ms == &ms_transi) {
    if (ms->mp_in->counter != 1) { // Guard against rando scanning the IPv4 range
      if (!(mctx->connectionless && mctx->role == MEASLAT_SERVER))
        evt_core_rm_fd (ctx, fdinfo->fd);
      return 1;
    }
    struct timespec next_tick = {0};
    struct measure_state ms_new = {0};
    measure_state_init(&mctx->mp, &ms_new);
    ms_new.fd = fdinfo->fd;
    ms_new.mp_nin = ms->mp_nin;
    memcpy(ms_new.mp_in, ms->mp_in, mctx->mp.payload_size);
    measure_next_tick(&mctx->mp, &ms_new, &next_tick);
    assoc_timer = register_timer (ctx, mctx, &ms_new, &next_tick);
  }

  // 6. Check if our measurements are done
  if (ms->mp_in->counter >= mctx->mp.max_measure && ms->mp_out->counter >= mctx->mp.max_measure) {
    measlat_stop(ctx, mctx, ms, fdinfo->fd, assoc_timer->fd);
    return 1;
  }

  return 0;
}


int on_tcp_co(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
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
  to_fdinfo.cat->name = "tcp-read";
  sprintf(to_fdinfo.url, "tcp:read:127.0.0.1:%s", port);
  evt_core_add_fd (ctx, &to_fdinfo);

  uint16_t myPort = ntohs(addr.sin_port);
  printf("client port is %u\n", myPort);

  return 0;

co_error:
  perror("Failed to handle new connection");
  exit(EXIT_FAILURE);
}

int on_timer(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  ssize_t s;
  uint64_t ticks = 0;
  struct measlat_ctx* mctx = fdinfo->cat->app_ctx;
  struct measure_state* ms = fdinfo->other;

  s = read(fdinfo->fd, &ticks, sizeof(uint64_t));
  if (s == -1 && errno == EAGAIN) return 1;
  if (s != sizeof(uint64_t)) {
    perror("Read error");
    exit(EXIT_FAILURE);
  }

  if (ticks != 1) {
    fprintf(stderr, "Has ticked %lu times, expected 1 time. This is a bug\n", ticks);
  }

  struct measure_packet* head = measure_generate(&mctx->mp, ms);
  //printf("send(id=%ld,is_echo=%d)\n", head->counter, head->is_echo);

  s = mctx->connectionless && mctx->role == MEASLAT_SERVER ?
    sendto(ms->fd, head, mctx->mp.payload_size, 0, (struct sockaddr*)&mctx->addr, mctx->addrlen) :
    send(ms->fd, head, mctx->mp.payload_size, 0);

  // Too bad, we will drop this packet
  if (s == -1 && errno == EAGAIN) return 1;

  if (s < 0 || s != mctx->mp.payload_size) {
    perror("Send error");
    exit(EXIT_FAILURE);
  }

  if (ms->mp_in->counter >= mctx->mp.max_measure && ms->mp_out->counter >= mctx->mp.max_measure) {
    measlat_stop(ctx, mctx, ms, ms->fd, fdinfo->fd);
    return 1;
  }

  return 0;
}

int on_socks5_success_measlat(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo_n = {0};
  struct socks5_ctx* s5ctx = fdinfo->other;
  fdinfo_n.cat = &cat;
  fdinfo_n.url = url;

  struct evt_core_cat* ucat = evt_core_get_from_cat (ctx, "tcp-read");
  if (ucat == NULL) {
    fprintf(stderr, "Category udp-read not found\n");
    exit(EXIT_FAILURE);
  }
  struct measlat_ctx* mctx = ucat->app_ctx;

  fdinfo_n.fd = dup(fdinfo->fd);
  fdinfo_n.cat->name = "tcp-read";
  sprintf(fdinfo_n.url, "tcp:read:%s:%d", s5ctx->addr, s5ctx->port);

  struct evt_core_fdinfo* reg_fdinfo = evt_core_add_fd (ctx, &fdinfo_n);
  printf("--- Tor socket registered\n");

  struct measure_state ms = {0};
  measure_state_init (&mctx->mp, &ms);
  ms.fd = fdinfo_n.fd;
  register_timer (ctx, mctx, &ms, NULL);
  return 1;
}

void spawn_tor_client(struct evt_core_ctx* evts) {
  struct evt_core_cat* ucat = evt_core_get_from_cat (evts, "tcp-read");
  if (ucat == NULL) {
    fprintf(stderr, "Category udp-read not found\n");
    exit(EXIT_FAILURE);
  }
  struct measlat_ctx* mctx = ucat->app_ctx;

  socks5_create_dns_client (evts, "127.0.0.1", mctx->tor_port, mctx->host, atoi(mctx->port));
  printf("--- Tor client SOCKS started\n");
}

int on_socks5_failed_measlat(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  evt_core_rm_fd (ctx, fdinfo->fd);
  spawn_tor_client(ctx);

  return 1;
}

void register_categories(struct evt_core_ctx* evts, struct measlat_ctx* mctx) {
  struct evt_core_cat template = {0};
  template.app_ctx = mctx;
  evt_core_init(evts, mctx->verbose <= 0 ? 0 : mctx->verbose - 1);

  template.cb = on_timer;
  template.name = "timer";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(evts, &template);

  template.cb = on_receive_measure_packet;
  template.err_cb = on_receive_measure_packet_err;
  template.name = "tcp-read";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(evts, &template);

  template.cb = on_receive_measure_packet;
  template.err_cb = on_receive_measure_packet_err;
  template.name = "udp-read";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(evts, &template);

  template.cb = on_tcp_co;
  template.err_cb = NULL;
  template.name = "tcp-co";
  template.flags = EPOLLIN | EPOLLET;
  evt_core_add_cat(evts, &template);

  template.cb = on_socks5_success_measlat;
  template.err_cb = on_socks5_failed_measlat;
  template.name = "socks5-success";
  template.flags = EPOLLET;
  evt_core_add_cat(evts, &template);

  template.cb = on_socks5_failed_measlat;
  template.err_cb = on_socks5_failed_measlat;
  template.name = "socks5-failed";
  template.flags = EPOLLET;
  evt_core_add_cat(evts, &template);

  socks5_init(evts);
  printf("--- Categories registered\n");

}

void spawn_udp_client(struct evt_core_ctx* evts) {
  struct evt_core_cat* ucat = evt_core_get_from_cat (evts, "udp-read");
  if (ucat == NULL) {
    fprintf(stderr, "Category udp-read not found\n");
    exit(EXIT_FAILURE);
  }
  struct measlat_ctx* mctx = ucat->app_ctx;

  int udp_sock = create_udp_client (mctx->host, mctx->port);
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  fdinfo.fd = udp_sock;
  fdinfo.cat->name = "udp-read";
  sprintf(fdinfo.url, "udp:read:%s:%s", mctx->host, mctx->port);
  struct evt_core_fdinfo* reg_fdinfo = evt_core_add_fd (evts, &fdinfo);
  printf("--- UDP client registered\n");

  struct sockaddr_in my_addr;
  socklen_t my_addr_len = sizeof(my_addr);
  getsockname(udp_sock, (struct sockaddr *) &my_addr, &my_addr_len);
  uint16_t myPort = ntohs(my_addr.sin_port);
  printf("client port is %u\n", myPort);

  struct measure_state ms = {0};
  measure_state_init (&mctx->mp, &ms);
  ms.fd = udp_sock;

  register_timer (evts, mctx, &ms, NULL);
}

void spawn_udp_server(struct evt_core_ctx* evts) {
  struct evt_core_cat* ucat = evt_core_get_from_cat (evts, "udp-read");
  if (ucat == NULL) {
    fprintf(stderr, "Category udp-read not found\n");
    exit(EXIT_FAILURE);
  }
  struct measlat_ctx* mctx = ucat->app_ctx;
  int udp_sock = create_udp_server (mctx->host, mctx->port);

  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  fdinfo.fd = udp_sock;
  sprintf(url, "udp:rw:127.0.0.1:%s", mctx->port);
  fdinfo.cat->name = "udp-read";
  evt_core_add_fd(evts, &fdinfo);
  printf("--- UDP server is listening\n");
}

void spawn_tcp_client(struct evt_core_ctx* evts) {
  struct evt_core_cat* ucat = evt_core_get_from_cat (evts, "tcp-read");
  if (ucat == NULL) {
    fprintf(stderr, "Category tcp-read not found\n");
    exit(EXIT_FAILURE);
  }
  struct measlat_ctx* mctx = ucat->app_ctx;

  int tcp_sock = create_tcp_client (mctx->host, mctx->port);
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  fdinfo.fd = tcp_sock;
  fdinfo.cat->name = "tcp-read";
  sprintf(fdinfo.url, "tcp:read:%s:%s", mctx->host, mctx->port);
  struct evt_core_fdinfo* reg_fdinfo = evt_core_add_fd (evts, &fdinfo);
  printf("--- TCP client registered\n");

  struct sockaddr_in my_addr;
  socklen_t my_addr_len = sizeof(my_addr);
  getsockname(tcp_sock, (struct sockaddr *) &my_addr, &my_addr_len);
  uint16_t myPort = ntohs(my_addr.sin_port);
  printf("client port is %u\n", myPort);

  struct measure_state ms = {0};
  measure_state_init (&mctx->mp, &ms);
  ms.fd = tcp_sock;

  register_timer (evts, mctx, &ms, NULL);
}

void spawn_tcp_server(struct evt_core_ctx* evts, uint16_t port) {
  char buffer[1024];
  int tcp_serv_sock, err;

  sprintf(buffer, "%d", port);
  tcp_serv_sock = create_tcp_server ("0.0.0.0", buffer);
  err = listen(tcp_serv_sock, SOMAXCONN);

  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = buffer;

  fdinfo.fd = tcp_serv_sock;
  sprintf(buffer, "tcp:co:127.0.0.1:%d", port);
  fdinfo.cat->name = "tcp-co";
  evt_core_add_fd(evts, &fdinfo);
  printf("--- TCP server is listening\n");
}

void measlat_create_onion_services(struct tor_os_str* tos, struct tor_ctl* tctl, uint16_t port, char* tor_port, enum TOR_ONION_FLAGS tof) {
  char fnpub[64] = {0}, fnpriv[64] = {0};
  sprintf(fnpub, "os_%u.pub", port);
  sprintf(fnpriv, "os_%u.priv", port);
  tor_os_create (tos, fnpub, fnpriv, 1);
  tor_os_read (tos);

  int err = 0;
  err = tor_ctl_connect (tctl, "127.0.0.1", tor_port);
  if (err < 0) {
    fprintf(stderr, "Unable to open Tor Socket\n");
    exit(EXIT_FAILURE);
  }
  err = tor_ctl_add_onion (tctl, tos, &port, 1, tof);
  if (err != 0) {
    fprintf(stderr, "Unable to create Onion Services (error: %d)\n", err);
    exit(EXIT_FAILURE);
  }
  printf("--- Onion services created\n");
}

void spawn_onion_server(struct evt_core_ctx* evts, struct measlat_ctx* mctx, struct tor_os_str* tos, struct tor_ctl* tctl) {
  spawn_tcp_server(evts, atoi(mctx->port));
  measlat_create_onion_services (tos, tctl, atoi(mctx->port), mctx->tor_port, mctx->tor_flags);
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("~ measlat ~\n");

  int opt;
  struct measlat_ctx mctx = {0};
  mctx.mp.tag = "undefined";
  struct evt_core_ctx evts = {0};
  struct tor_os_str tos = {0};
  struct tor_ctl tctl = {0};
  tctl.os_endpoint = "127.0.0.1";
  measure_params_init (&mctx.mp);

  // 1. Parse parameters
  while ((opt = getopt(argc, argv, "vq:h:p:c:s:i:t:lnm:b")) != -1) {
      switch(opt) {
      case 'b':
        mctx.mp.probe = 1;
      case 'v':
        mctx.verbose++;
        break;
      case 'h': // host
        mctx.host = optarg;
        break;
      case 'p': // port
        mctx.port = optarg;
        break;
      case 'q':
        mctx.tor_port = optarg;
        break;
      case 'l':
        mctx.role = MEASLAT_SERVER;
        break;
      case 't': // transport
        mctx.transport = optarg;
        break;
      case 'c': // count
        mctx.mp.max_measure = atoi(optarg);
        break;
      case 's': // size - payload in bytes
        measure_params_setpl(&mctx.mp, atoi(optarg));
        break;
      case 'n':
        mctx.tor_flags |= TOR_ONION_FLAG_NON_ANONYMOUS;
        break;
      case 'i': // interval - every ms
        mctx.mp.interval = atoi(optarg);
        break;
      case 'm':
        mctx.mp.tag = optarg;
        break;
      default:
        goto usage;
      }
  }

  // 2. Check and fix parameters
  measure_state_init (&mctx.mp, &ms_transi);

  mctx.addrlen = sizeof(mctx.addr);
  if (mctx.mp.probe) mctx.mp.max_measure = 1;
  if (mctx.transport == NULL) mctx.transport = "udp";
  if (strcmp(mctx.transport, "udp") == 0) mctx.connectionless = 1;
  if (mctx.host == NULL) mctx.host = "127.0.0.1";
  if (mctx.port == NULL) mctx.port = mctx.connectionless ? "9000" : "7500";
  if (mctx.tor_port == NULL) mctx.tor_port = "9050";

  printf("[measlat_conf] host=%s, port=%s, listen=%d, transport=%s, count=%ld, size=%ld, interval=%ld, tor_port=%s\n",
          mctx.host, mctx.port, mctx.role, mctx.transport, mctx.mp.max_measure, mctx.mp.payload_size, mctx.mp.interval, mctx.tor_port);

  // 3. Create event structure
  register_categories(&evts, &mctx);

  // 4. Register services
  if (mctx.role == MEASLAT_SERVER) {
    if (streq(mctx.transport, "udp")) spawn_udp_server (&evts);
    else if (streq(mctx.transport, "tcp")) spawn_tcp_server(&evts, atoi(mctx.port));
    else if (streq(mctx.transport, "tor")) spawn_onion_server (&evts, &mctx, &tos, &tctl);
  }
  else if (mctx.role == MEASLAT_CLIENT) {
    if (streq(mctx.transport, "udp")) spawn_udp_client(&evts);
    else if (streq(mctx.transport, "tor")) spawn_tor_client(&evts);
    else if (streq(mctx.transport, "tcp")) spawn_tcp_client(&evts);
  }
  else exit(EXIT_FAILURE);

  // 5. Run main loop
  evt_core_loop(&evts);

  return 0;
usage:
  fprintf(stderr, "Usage: %s [-h <host>] [-p <port>] [-l] [-r] [-t <udp|tcp|tor>] [-c <count>] [-i <ms>] [-s <bytes>]\n", argv[0]);
  exit(EXIT_FAILURE);
}
