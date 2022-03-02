// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "timer.h"

struct timer_ctx {
  timer_cb cb;
  void* user_ctx;
};

void free_timerctx(void* c) {
  free(c);
}

int set_timeout_handle(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  uint64_t ctr;
  ssize_t tmr_rd;
  tmr_rd = read(fdinfo->fd, &ctr, sizeof(ctr));
  if (tmr_rd == -1 && errno == EAGAIN) return 1;
  if (tmr_rd < 0) {
    perror("read on timer");
    fprintf(stderr, "An error occured on timer fd=%d\n", fdinfo->fd);
    exit(EXIT_FAILURE);
  }

  struct timer_ctx* tctx = fdinfo->other;
  enum DONAR_TIMER_DECISION dtd = tctx->cb(ctx, tctx->user_ctx);

  if (dtd == DONAR_TIMER_STOP) evt_core_rm_fd(ctx, fdinfo->fd);
  return EVT_CORE_FD_EXHAUSTED;
}

void init_timer(struct evt_core_ctx* evts) {
  struct evt_core_cat* cat = evt_core_get_from_cat (evts, "set_timeout");
  if (cat != NULL) {
    fprintf(stderr, "timeout category has already been registered\n");
    return;
  }

  struct evt_core_cat timer = {
    .name = "set_timeout",
    .flags = EPOLLIN | EPOLLET,
    .app_ctx = NULL,
    .free_app_ctx = NULL,
    .cb = set_timeout_handle,
    .err_cb = NULL
  };
  evt_core_add_cat(evts, &timer);
}

int set_timeout(struct evt_core_ctx* evts, uint64_t milli_sec, void* ctx, timer_cb cb) {
  struct timespec now;
  struct itimerspec timer_config;
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  //printf("Will add a timeout of %ld ms\n", milli_sec);
  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
    perror("clock_gettime");
    exit(EXIT_FAILURE);
  }

  uint64_t ns = now.tv_nsec + (milli_sec % 1000) * 1000000;
  timer_config.it_value.tv_sec = now.tv_sec + milli_sec / 1000 + ns / 1000000000;
  timer_config.it_value.tv_nsec = ns % 1000000000;
  timer_config.it_interval.tv_sec = 60;
  timer_config.it_interval.tv_nsec = 0;

  fdinfo.fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fdinfo.fd == -1) {
    perror("Unable to timerfd_create");
    exit(EXIT_FAILURE);
  }
  if (timerfd_settime (fdinfo.fd, TFD_TIMER_ABSTIME, &timer_config, NULL) == -1) {
    perror("Unable to timerfd_settime");
    exit(EXIT_FAILURE);
  }
  fdinfo.cat->name = "set_timeout";
  struct timer_ctx* tctx = malloc(sizeof(struct timer_ctx)); // Should put the link number and the id
  if (tctx == NULL) {
    perror("malloc failed in set_timeout");
    exit(EXIT_FAILURE);
  }
  tctx->user_ctx = ctx;
  tctx->cb = cb;
  fdinfo.other = tctx;
  fdinfo.free_other = free_timerctx;
  sprintf(fdinfo.url, "timer:%ld:1", milli_sec);
  evt_core_add_fd (evts, &fdinfo);

  return fdinfo.fd;
}

void stop_timer(struct evt_core_ctx* evts) {
  struct evt_core_cat* cat = evt_core_get_from_cat (evts, "set_timeout");
  if (cat == NULL) {
    fprintf(stderr, "timeout category does not exist\n");
    return;
  }
  while (cat->socklist->len > 0) {
    evt_core_rm_fd (evts, g_array_index (cat->socklist, struct evt_core_fdinfo*, 0)->fd);
  }
}
