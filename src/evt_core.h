// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>
#include <glib-2.0/glib-object.h>
#include "net_tools.h"
#include "utils.h"
#include "stopwatch.h"

#define EVT_CORE_MAX_EVENTS 10

enum evt_core_fd_handle {
  EVT_CORE_FD_UNFINISHED = 0,
  EVT_CORE_FD_EXHAUSTED = 1,
};

struct evt_core_ctx;
struct evt_core_cat;
struct evt_core_fdinfo;

typedef void (*evt_core_free_app_ctx)(void*);
typedef int (*evt_core_cb)(struct evt_core_ctx*, struct evt_core_fdinfo*);

struct evt_core_cat {
  void* app_ctx;
  evt_core_free_app_ctx free_app_ctx;
  evt_core_cb cb;
  evt_core_cb err_cb;
  char* name;
  int flags;
  GArray* socklist;
};

struct evt_core_ctx {
  int epollfd;
  uint8_t verbose;
  uint8_t loop;
  int blacklisted_fds[EVT_CORE_MAX_EVENTS];
  int blacklisted_fds_count;
  GHashTable* catlist;  // name -> category
  GHashTable* socklist; // fd -> category
  GHashTable* urltofd;  // url -> fd, like "tcp:127.0.0.1:7500"
};

struct evt_core_fdinfo {
  int fd;
  char* url;
  struct evt_core_cat* cat;
  void* other;
  evt_core_free_app_ctx free_other;
};

void evt_core_init(struct evt_core_ctx* ctx, uint8_t verbose);
void evt_core_add_cat(struct evt_core_ctx* ctx, struct evt_core_cat* cat);
void evt_core_mv_fd(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct evt_core_cat* to_cat);
void evt_core_mv_fd2(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, char* to_cat);
struct evt_core_fdinfo* evt_core_dup(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo, char* new_url);
struct evt_core_fdinfo* evt_core_add_fd(struct evt_core_ctx* ctx, struct evt_core_fdinfo* user_data);
struct evt_core_cat* evt_core_rm_fd(struct evt_core_ctx* ctx, int fd);
void evt_core_free(struct evt_core_ctx* ctx);
void evt_core_loop(struct evt_core_ctx* ctx);
struct evt_core_fdinfo* evt_core_get_from_fd(struct evt_core_ctx* ctx, int fd);
struct evt_core_fdinfo* evt_core_get_from_url(struct evt_core_ctx* ctx, char* url);
void evt_core_free_app_ctx_simple(void* v);
struct evt_core_cat* evt_core_get_from_cat(struct evt_core_ctx* ctx, char* name);
struct evt_core_fdinfo* evt_core_get_first_from_cat(struct evt_core_ctx* ctx, char* name);
gboolean evt_core_fdinfo_url_set(struct evt_core_fdinfo* fdinfo, char* url);
