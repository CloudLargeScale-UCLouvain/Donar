// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "evt_core.h"

void free_fdinfo(void* v) {
  struct evt_core_fdinfo* fdinfo = (struct evt_core_fdinfo*)v;
  fprintf(stdout, "[%s][evt_core] Freeing fdinfo url=%s fd=%d\n", current_human_datetime (), fdinfo->url, fdinfo->fd);
  if (close(fdinfo->fd) != 0) { // We close the file descriptor here
    fprintf(stderr, "[%s][evt_core] Failed to close fd for url=%s fd=%d. ",  current_human_datetime (), fdinfo->url, fdinfo->fd);
    perror("Error");
  }
  if (fdinfo->free_other != NULL) {
    //fprintf(stderr, "Freeing fdinfo->other for %s\n", fdinfo->url);
    fdinfo->free_other(fdinfo->other);
  }
  if (fdinfo->url != NULL) {
    //fprintf(stderr, "Freeing fdinfo->url for %s\n", fdinfo->url);
    free(fdinfo->url); // We free the URL here;
  }
  free(v);
}

void free_simple(void* s) {
  free(s);
}

void free_cat(void* vcat) {
  struct evt_core_cat* cat = (struct evt_core_cat*) vcat;
  if (cat->free_app_ctx != NULL) cat->free_app_ctx(cat->app_ctx);
  g_array_free(cat->socklist, TRUE);
  free(cat->name);
  free(cat);
}

void evt_core_init(struct evt_core_ctx* ctx, uint8_t verbose) {
  ctx->epollfd = epoll_create1(0);
  if (ctx->epollfd == -1) {
    perror("Failed to create epoll file descriptor epoll_create1");
    exit(EXIT_FAILURE);
  }
  ctx->verbose = verbose;
  ctx->loop = 1;
  ctx->blacklisted_fds_count = 0;
  memset(ctx->blacklisted_fds, 0, EVT_CORE_MAX_EVENTS*sizeof(int));
  ctx->catlist = g_hash_table_new_full(g_str_hash, g_str_equal,NULL, free_cat);
  ctx->socklist = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, free_fdinfo);
  ctx->urltofd = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
}

void evt_core_add_cat(struct evt_core_ctx* ctx, struct evt_core_cat* cat) {
  if (cat->socklist != NULL) {
    fprintf(stderr, "cat->socklist must be null. What have you done?\n");
    exit(EXIT_FAILURE);
  }

  // 1. Create category structure
  struct evt_core_cat* dyn = NULL;
  dyn = malloc(sizeof(struct evt_core_cat));
  if (dyn == NULL) {
    fprintf(stderr, "Failed to alloc memory\n");
    exit(EXIT_FAILURE);
  }

  // 2. Populate category structure
  dyn->app_ctx = cat->app_ctx;
  dyn->free_app_ctx = cat->free_app_ctx;
  dyn->cb = cat->cb;
  dyn->name = strdup(cat->name);
  dyn->flags = cat->flags;
  dyn->err_cb = cat->err_cb;
  dyn->socklist = g_array_new (FALSE, FALSE, sizeof(struct evt_core_fdinfo*));

  if (dyn->name == NULL) {
    perror("Unable to allocate memory for category name via strdup");
    exit(EXIT_FAILURE);
  }

  // 3. Insert category structure in our context
  g_hash_table_insert (ctx->catlist, dyn->name, dyn);
}

void evt_core_mv_fd(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct evt_core_cat* to_cat) {
  if (ctx->verbose) fprintf(stderr, "  Moving fd=%d from cat=%s to cat=%s\n",fdinfo->fd, fdinfo->cat->name, to_cat->name);

  // 1. Update old category
  for (int i = 0; i < fdinfo->cat->socklist->len; i++) {
    if (g_array_index(fdinfo->cat->socklist, struct evt_core_fdinfo*, i) == fdinfo) {
      g_array_remove_index(fdinfo->cat->socklist, i);
    }
  }

  // 2. Set new cat for fdinfo
  fdinfo->cat = to_cat;

  // 3. Update new category
  g_array_append_val (fdinfo->cat->socklist, fdinfo);

  // 4. Update epoll flags
  update_fd_epoll (ctx->epollfd, fdinfo->fd, fdinfo->cat->flags);

  // 5. Handle cases where data arrived before registering the file descriptor
  fdinfo->cat->cb(ctx, fdinfo);
}

gboolean evt_core_fdinfo_url_set(struct evt_core_fdinfo* fdinfo, char* url) {
  char *new_url = strdup(url);
  if (new_url == NULL) return FALSE;

  free(fdinfo->url);
  fdinfo->url = new_url;
  return TRUE;
}

void evt_core_mv_fd2(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, char* to_cat) {
  struct evt_core_cat* cat = evt_core_get_from_cat (ctx, to_cat);
  if (cat == NULL) {
    fprintf(stderr, "Category %s does not exist\n", to_cat);
    exit(EXIT_FAILURE);
  }
  evt_core_mv_fd (ctx, fdinfo, cat);
}

struct evt_core_fdinfo* evt_core_add_fd(struct evt_core_ctx* ctx, struct evt_core_fdinfo* user_data) {
  // 1. Fetch fd category
  struct evt_core_cat* cat = g_hash_table_lookup(ctx->catlist, user_data->cat->name);
  if (cat == NULL) {
    fprintf(stderr, "Category %s should be defined before inserting a file descriptor in it.\n", user_data->cat->name);
    exit(EXIT_FAILURE);
  }

  // 2. Create fdinfo struct
  struct evt_core_fdinfo* fdinfo;
  if ((fdinfo = malloc(sizeof (struct evt_core_fdinfo))) == NULL) {
    perror("Unable to allocate memory for fdinfo via malloc");
    exit(EXIT_FAILURE);
  }

  // 3. Populate fdinfo struct
  fdinfo->fd = user_data->fd;
  fdinfo->cat = cat;
  fdinfo->url = strdup(user_data->url);
  fdinfo->other = user_data->other;
  fdinfo->free_other = user_data->free_other;

  if (fdinfo->url == NULL) {
    perror("Unable to allocate memory via malloc for fdinfo->url");
    exit(EXIT_FAILURE);
  }

  // 4. Insert structure in our context
  g_array_append_val (cat->socklist, fdinfo);
  g_hash_table_insert(ctx->socklist, &(fdinfo->fd), fdinfo);
  g_hash_table_insert(ctx->urltofd, fdinfo->url, fdinfo);

  // 5. Add file descriptor to epoll
  add_fd_to_epoll(ctx->epollfd, user_data->fd, cat->flags);
  if (ctx->verbose) fprintf(stderr, "  Added fd=%d with url=%s in cat=%s\n", fdinfo->fd, fdinfo->url, fdinfo->cat->name);

  // 6. Ensure that events arrived before epoll registering are handled
  fdinfo->cat->cb(ctx, fdinfo);

  return fdinfo;
}

struct evt_core_cat* evt_core_rm_fd(struct evt_core_ctx* ctx, int fd) {
  struct evt_core_cat* cat;

  // 1. Fetch fdinfo structure
  struct evt_core_fdinfo* fdinfo = g_hash_table_lookup (ctx->socklist, &fd);
  if (fdinfo == NULL) return NULL;
  cat = fdinfo->cat;
  if (ctx->verbose) fprintf(stderr, "  Closing url=%s, fd=%d from cat=%s\n", fdinfo->url, fdinfo->fd, fdinfo->cat->name);

  // 2. Update category
  for (int i = 0; i < cat->socklist->len; i++) {
    if (g_array_index(cat->socklist, struct evt_core_fdinfo*, i) == fdinfo) {
      g_array_remove_index(cat->socklist, i);
    }
  }

  // 3. Remove structure from urltofd and socklist
  g_hash_table_remove(ctx->urltofd, fdinfo->url);
  g_hash_table_remove(ctx->socklist, &fd); // Will be freed here

  // 4. List the fd as blacklisted for the end of the current loop
  ctx->blacklisted_fds[ctx->blacklisted_fds_count] = fd;
  ctx->blacklisted_fds_count++;

  // 4. Close and remove file descriptor
  // Done in free_fdinfo
  //epoll_ctl(ctx->epollfd, EPOLL_CTL_DEL, fd, NULL);
  //close(fd);

  // 5. Return file descriptor's category
  return cat;
}

void evt_core_free(struct evt_core_ctx* ctx) {
  g_hash_table_destroy(ctx->socklist);
  g_hash_table_destroy(ctx->catlist);
  g_hash_table_destroy (ctx->urltofd);
}

int evt_core_fd_bl(struct evt_core_ctx* ctx, int fd) {
  for (int j = 0; j < ctx->blacklisted_fds_count; j++) {
    if (ctx->blacklisted_fds[j] == fd) {
      fprintf(stderr, "fd=%d has been deleted, skipping epoll notification\n", ctx->blacklisted_fds[j]);
      return 1;
    }
  }
  return 0;
}

void evt_core_loop(struct evt_core_ctx* ctx) {
  struct epoll_event events[EVT_CORE_MAX_EVENTS];
  struct evt_core_fdinfo* fdinfo;
  struct evt_core_cat* cat;
  int return_code;
  clock_t start_timing, end_timing;
  double elapsed_in_cb;
  struct timing_fx tfx_epoll, tfx_err, tfx_cb, tfx_loop;

  enum timing_config base = ctx->verbose ? TIMING_ACTIVATED : 0;
  timing_fx_init (&tfx_epoll, base | TIMING_DISPLAY_END, "", "[SLEPT]()");
  timing_fx_init (&tfx_cb, base | TIMING_DISPLAY_BOTH, "[BEGIN CB](name=%s, url=%s, fd=%d)\n", "[END CB]()");
  timing_fx_init (&tfx_err, base | TIMING_DISPLAY_BOTH, "[BEGIN ERR](name=%s, url=%s, fd=%d)\n", "[END ERR]()");
  timing_fx_init (&tfx_loop, base | TIMING_DISPLAY_END, "", "[LOOP]()");

  printf("--- Start main loop\n");
  int num_fd, n = 0;
  while(ctx->loop) {
    ctx->blacklisted_fds_count = 0;
    timing_fx_start(&tfx_epoll);
    num_fd = epoll_wait(ctx->epollfd, events, EVT_CORE_MAX_EVENTS, -1);
    timing_fx_stop(&tfx_epoll);

    timing_fx_start(&tfx_loop);
    if (num_fd == -1) {
      perror("Failed to epoll_wait");
      continue;
    }

    // 1. Handle errors
    for (n = 0 ; n < num_fd; n++) {
      if (!(events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) continue;
      if (evt_core_fd_bl(ctx, events[n].data.fd)) continue;

      int err_fd = events[n].data.fd;
      int evt = events[n].events;
      if (evt & EPOLLRDHUP) fprintf(stderr, "Epoll Read Hup Event fd=%d.\n", err_fd);
      if (evt & EPOLLHUP) fprintf(stderr, "Epoll Hup Event fd=%d.\n", err_fd);
      if (evt & EPOLLERR) {
        int error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(err_fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0) {
          fprintf(stderr, "Socket %d error = %s\n", err_fd, strerror(error));
        } else if (errno == EBADF) {
          fprintf(stderr, "fd=%d is invalid. Probably closed\n", err_fd);
          continue;
        } else {
          fprintf(stderr, "Using fd=%d as socket produced error: ", err_fd);
          perror("getsockopt");
        }
        fprintf(stderr, "Epoll Err Event. ");
      }

      fdinfo = evt_core_get_from_fd(ctx, err_fd);
      if (fdinfo != NULL) {
        if (fdinfo->cat->err_cb != NULL) {

          timing_fx_start (&tfx_err, fdinfo->cat->name, fdinfo->url, fdinfo->fd);
          return_code = fdinfo->cat->err_cb(ctx, fdinfo);
          timing_fx_stop(&tfx_err);

          if (return_code == 1) {
            fprintf(stderr, "Error on fd=%d on cat=%s is handled by app, not clearing it\n", err_fd, fdinfo->cat->name);
            continue;
          }
        }
        fprintf(stderr, "Clearing fd=%d on cat=%s\n", err_fd, fdinfo->cat->name);
        evt_core_rm_fd (ctx, err_fd);
      } else {
        fprintf(stderr, "The file descriptor %d is not registered in a category, this is probably a logic error\n", err_fd);
        close (err_fd);
      }
    }

    // 2. Fetch info and call appropriate function
    for (n = 0 ; n < num_fd; n++) {
      if (events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) continue;
      if (evt_core_fd_bl(ctx, events[n].data.fd)) continue;

      fdinfo = g_hash_table_lookup(ctx->socklist, &(events[n].data.fd));
      if (fdinfo == NULL) {
        fprintf(stderr, "Ignoring file descriptor %d as it is not registered. This is a bug.\n", events[n].data.fd);
        continue;
      }

      timing_fx_start(&tfx_cb, fdinfo->cat->name, fdinfo->url, fdinfo->fd);
      while(fdinfo->cat->cb(ctx, fdinfo) == 0);
      timing_fx_stop(&tfx_cb);
    }

    timing_fx_stop(&tfx_loop);
  }

  evt_core_free(ctx);
  return;
}

struct evt_core_fdinfo* evt_core_get_from_fd(struct evt_core_ctx* ctx, int fd) {
  return g_hash_table_lookup (ctx->socklist, &fd);
}

struct evt_core_fdinfo* evt_core_get_from_url(struct evt_core_ctx* ctx, char* url) {
  return g_hash_table_lookup (ctx->urltofd, url);
}

struct evt_core_cat* evt_core_get_from_cat(struct evt_core_ctx* ctx, char* name) {
  return g_hash_table_lookup (ctx->catlist, name);
}

struct evt_core_fdinfo* evt_core_get_first_from_cat(struct evt_core_ctx* ctx, char* name) {
  struct evt_core_cat* cat = g_hash_table_lookup (ctx->catlist, name);
  if (cat == NULL) return NULL;
  if (cat->socklist == NULL || cat->socklist->len <= 0) return NULL;
  return g_array_index(cat->socklist, struct evt_core_fdinfo*, 0);
}

void evt_core_free_app_ctx_simple(void* v) {
  free(v);
}

struct evt_core_fdinfo* evt_core_dup(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo, char *newurl) {
  struct evt_core_fdinfo newfdinfo;
  struct evt_core_cat newcat;
  newfdinfo.cat = &newcat;

  newfdinfo.cat->name = fdinfo->cat->name;
  newfdinfo.fd = dup(fdinfo->fd);
  newfdinfo.url = strdup(newurl);

  return evt_core_add_fd (ctx, &newfdinfo);
}
