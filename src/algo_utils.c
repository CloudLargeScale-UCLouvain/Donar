// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "algo_utils.h"

void iterate(int* fd, GQueue* q, int* waiting_count) {
  fprintf(stderr, "Queue for fd=%d has length=%d\n", *fd, q->length);
  *waiting_count += q->length;
}

void iterate2(int* fd, struct buffer_packet *bp, gpointer user_data) {
  fprintf(stderr, "fd=%d has a used_buffer entry\n", *fd);
}

void naive_free_simple(void* v) {
  GQueue* g = v;
  g_queue_free (g);
}

void __push_to_free(struct buffer_resources *app_ctx, struct buffer_packet* bp) {
  memset(bp, 0, sizeof(struct buffer_packet));
  g_queue_push_tail (app_ctx->free_buffer, bp);
}

void debug_buffer(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo) {
  fprintf(stderr, "No more free buffer for fd=%d.\n", fdinfo->fd);
  int waiting_count = 0;
  g_hash_table_foreach(app_ctx->write_waiting, (GHFunc)iterate, &waiting_count);
  g_hash_table_foreach(app_ctx->used_buffer, (GHFunc)iterate2, NULL);
  fprintf(stderr, "total_buffers=%d, free_buffer=%d, used_buffers=%d, app_buffer=%d, write_buffer=%d.\n",
          PACKET_BUFFER_SIZE,
          app_ctx->free_buffer->length,
          g_hash_table_size(app_ctx->used_buffer),
          g_hash_table_size(app_ctx->application_waiting),
          waiting_count);
}

void init_buffer_management(struct buffer_resources* ctx) {
  ctx->free_buffer = g_queue_new ();
  ctx->read_waiting = g_queue_new ();
  ctx->application_waiting = g_hash_table_new (NULL, NULL);
  ctx->used_buffer = g_hash_table_new(g_int_hash, g_int_equal);
  ctx->write_waiting = g_hash_table_new_full (g_int_hash, g_int_equal, NULL, naive_free_simple);
  for (int i = 0; i < sizeof(ctx->bps) / sizeof(ctx->bps[0]); i++) {
    __push_to_free (ctx, &(ctx->bps[i]));
  }
}

void destroy_buffer_management(struct buffer_resources* ctx) {
  g_queue_free(ctx->free_buffer);
  g_queue_free(ctx->read_waiting);
  g_hash_table_destroy (ctx->application_waiting);
  g_hash_table_destroy (ctx->used_buffer);
  g_hash_table_destroy (ctx->write_waiting);
}

/**
 * Returns a buffer if available, NULL otherwise
 */
struct buffer_packet* get_read_buffer(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo) {
  struct buffer_packet* bp;

  // 1. Check if we don't have a buffer
  bp = fdinfo == NULL ? NULL : g_hash_table_lookup (app_ctx->used_buffer, &fdinfo->fd);
  if (bp != NULL) return bp;

  // 2. Get a new buffer otherwise
  bp = g_queue_pop_head(app_ctx->free_buffer);
  if (bp == NULL) {
    debug_buffer(app_ctx, fdinfo);

    // 2.1 If no buffer is available, we subscribe to be notified later
    g_queue_push_tail (app_ctx->read_waiting, &(fdinfo->fd));
    return NULL;
  }

  // 3. Update state
  g_hash_table_insert(app_ctx->used_buffer, &(fdinfo->fd), bp);

  return bp;
}

guint write_queue_len(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo) {
  GQueue* q;

  if ((q = g_hash_table_lookup(app_ctx->write_waiting, &(fdinfo->fd))) == NULL) return 0; // No queue
  return q->length;
}

/**
 * Returns a buffer if available, NULL otherwise
 */
struct buffer_packet* get_write_buffer(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo) {
  struct buffer_packet* bp;
  GQueue* q;

  // 1. Check if we don't have a buffer
  bp = g_hash_table_lookup (app_ctx->used_buffer, &fdinfo->fd);
  if (bp != NULL) return bp;

  // 2. Check our waiting queue otherwise
  if ((q = g_hash_table_lookup(app_ctx->write_waiting, &(fdinfo->fd))) == NULL) return NULL;
  bp = g_queue_pop_head(q);
  if (bp == NULL) return NULL; // No packet to process

  // 3. Update state
  g_hash_table_insert(app_ctx->used_buffer, &(fdinfo->fd), bp);

  return bp;
}

void mv_buffer_rtow(struct buffer_resources *app_ctx, struct evt_core_fdinfo* from, struct evt_core_fdinfo* to) {
  GQueue* q;
  struct buffer_packet* bp;

  // 1. We get the packet buffer
  bp = g_hash_table_lookup (app_ctx->used_buffer, &from->fd);
  if (bp == NULL) {
    fprintf(stderr, "Unable to find a buffer for fd=%d url=%s in rtow\n", from->fd, from->url);
    exit(EXIT_FAILURE);
  }

  // 2. We get the target writing queue
  q = g_hash_table_lookup(app_ctx->write_waiting, &(to->fd));
  if (q == NULL) {
    q = g_queue_new ();
    g_hash_table_insert(app_ctx->write_waiting, &(to->fd), q);
  }

  // 3. We move the data
  g_hash_table_remove(app_ctx->used_buffer, &from->fd);
  g_queue_push_tail(q, bp);
}

void mv_buffer_rtof(struct buffer_resources *app_ctx, struct evt_core_fdinfo* from) {
  struct buffer_packet* bp;

  // 1. We get the packet buffer
  bp = g_hash_table_lookup (app_ctx->used_buffer, &from->fd);
  if (bp == NULL) {
    fprintf(stderr, "Unable to find a buffer for fd=%d url=%s in rtof\n", from->fd, from->url);
    return;
  }
  g_hash_table_remove(app_ctx->used_buffer, &(from->fd));

   __push_to_free (app_ctx, bp);
}

void mv_buffer_wtof(struct buffer_resources *app_ctx, struct evt_core_fdinfo* fdinfo) {
  struct buffer_packet* bp = g_hash_table_lookup (app_ctx->used_buffer, &(fdinfo->fd));
  if (bp == NULL) {
    fprintf(stderr, "Unable to find a buffer for fd=%d url=%s in wtof\n", fdinfo->fd, fdinfo->url);
    return;
  }
  g_hash_table_remove(app_ctx->used_buffer, &(fdinfo->fd));

   __push_to_free (app_ctx, bp);
}

void mv_buffer_rtoa(struct buffer_resources *app_ctx, struct evt_core_fdinfo* from, void* to) {
  struct buffer_packet* bp;
  bp = g_hash_table_lookup (app_ctx->used_buffer, &from->fd);
  if (bp == NULL) {
    fprintf(stderr, "Unable to find a buffer for fd=%d url=%s\n", from->fd, from->url);
  }
  g_hash_table_remove(app_ctx->used_buffer, &from->fd);
  if (g_hash_table_contains(app_ctx->application_waiting, to)) {
    fprintf(stderr, "Data already exists for this entry\n");
    debug_buffer(app_ctx, from);
    exit(EXIT_FAILURE);
  }
  g_hash_table_insert(app_ctx->application_waiting, to, bp);
}

void mv_buffer_atow(struct buffer_resources *app_ctx, void* from, struct evt_core_fdinfo* to) {
  GQueue* q;
  struct buffer_packet* bp;

  // 1. We get the buffer
  bp = g_hash_table_lookup (app_ctx->application_waiting, from);
  if (bp == NULL) {
    fprintf(stderr, "Unable to find this application buffer in atow. Doing nothing...\n");
    return;
  }

  // 2. We get the target writing queue
  q = g_hash_table_lookup(app_ctx->write_waiting, &(to->fd));
  if (q == NULL) {
    q = g_queue_new ();
    g_hash_table_insert(app_ctx->write_waiting, &(to->fd), q);
  }

  // 3. We move the buffer
  g_hash_table_remove (app_ctx->application_waiting, from);
  g_queue_push_tail(q, bp);
}

void mv_buffer_atof(struct buffer_resources *app_ctx, void* from) {
  struct buffer_packet* bp;

  // 1. Remove the buffer
  bp = g_hash_table_lookup (app_ctx->application_waiting, from);
  if (bp == NULL) {
    fprintf(stderr, "Unable to find this application buffer in atof\n");
    exit(EXIT_FAILURE);
  }
  g_hash_table_remove (app_ctx->application_waiting, from);

  // 2. Append it to free list
  __push_to_free (app_ctx, bp);
}

struct buffer_packet* inject_buffer_tow(struct buffer_resources *app_ctx, struct evt_core_fdinfo* to) {
  GQueue* q;

  // 1. We get a free buffer
  struct buffer_packet* bp_dest = g_queue_pop_head(app_ctx->free_buffer);
  if (bp_dest == NULL) {
    debug_buffer(app_ctx, to);
    return NULL;
  }

  // 2. We get the target writing queue
  q = g_hash_table_lookup(app_ctx->write_waiting, &(to->fd));
  if (q == NULL) {
    q = g_queue_new ();
    g_hash_table_insert(app_ctx->write_waiting, &(to->fd), q);
  }

  // 3. We push the content to the appropriate destination
  g_queue_push_tail(q, bp_dest);
  return bp_dest;
}

struct buffer_packet* dup_buffer_tow(struct buffer_resources *app_ctx, struct buffer_packet* bp, struct evt_core_fdinfo* to) {
  // 1. Inject a new buffer
  struct buffer_packet* bp_dest = inject_buffer_tow (app_ctx, to);

  // 2. We duplicate the data
  memcpy(bp_dest, bp, sizeof(struct buffer_packet));

  return bp_dest;
}

struct buffer_packet* dup_buffer_toa(struct buffer_resources *app_ctx, struct buffer_packet* bp, void* to) {
  GQueue* q;

  // 1. We get a free buffer
  struct buffer_packet* bp_dest = g_queue_pop_head(app_ctx->free_buffer);
  if (bp_dest == NULL) {
    debug_buffer(app_ctx, to);
    return NULL;
  }

  // 2. We duplicate the data
  memcpy(bp_dest, bp, sizeof(struct buffer_packet));

  // 3. We put the data
  if (g_hash_table_contains(app_ctx->application_waiting, to)) {
    fprintf(stderr, "Data already exists for this entry\n");
    exit(EXIT_FAILURE);
  }
  g_hash_table_insert(app_ctx->application_waiting, to, bp_dest);
  return bp_dest;
}

struct buffer_packet* get_app_buffer(struct buffer_resources *app_ctx, void* idx) {
  return g_hash_table_lookup (app_ctx->application_waiting, idx);
}

void notify_read(struct evt_core_ctx* ctx, struct buffer_resources* app_ctx) {
  struct evt_core_fdinfo* next_fdinfo = NULL;
  while (next_fdinfo == NULL) {
    int* fd = g_queue_pop_head(app_ctx->read_waiting);
    if (fd == NULL) break;
    next_fdinfo = evt_core_get_from_fd (ctx, *fd);
    if (next_fdinfo == NULL) {
      fprintf(stderr, "Unable to find fdinfo for fd=%d\n", *fd);
      exit(EXIT_FAILURE);
    } else if (strcmp(next_fdinfo->cat->name, "tcp-read") == 0 || strcmp(next_fdinfo->cat->name, "udp-read") == 0) {
      next_fdinfo->cat->cb(ctx, next_fdinfo);
    } else {
      fprintf(stderr, "A fd from category %s can't be stored in read_waiting\n", next_fdinfo->cat->name);
      exit(EXIT_FAILURE);
    }
  }
}
