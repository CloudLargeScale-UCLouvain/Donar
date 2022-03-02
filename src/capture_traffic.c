// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "capture_traffic.h"

void dynbuf_init(struct dynbuf* db) {
  db->content = malloc(MEGABYTE);
  if (db->content == NULL) {
    perror("malloc dynbuf failed");
    exit(EXIT_FAILURE);
  }
  db->written = 0;
  db->alloced = MEGABYTE;
}

void dynbuf_check_alloc(struct dynbuf* db, size_t len) {
    if (db->written + len > db->alloced) {
    size_t new_alloced = db->written + len > 2 * db->alloced ? db->written + len : 2 * db->alloced;
    db->content = realloc(db->content, new_alloced);
    if (db->content == NULL) {
      perror("realloc dynbuf failed");
      exit(EXIT_FAILURE);
    }
    db->alloced = new_alloced;
  }
}

void dynbuf_append(struct dynbuf* db, char* ptr, size_t len) {
  dynbuf_check_alloc(db, len);

  memcpy(db->content + db->written, ptr, len);
  db->written += len;
}

void dynbuf_destroy(struct dynbuf* db) {
  free(db->content);
}

void traffic_capture_init(struct capture_ctx* ctx, char* filename) {
  ctx->activated = filename == NULL ? 0 : 1;
  if (!ctx->activated) return;

  ctx->filename = strdup(filename);
  dynbuf_init (&ctx->in);
  dynbuf_init (&ctx->out);
}

void traffic_capture_notify(struct capture_ctx* ctx, struct buffer_packet *bp, char* dest) {
  if (!ctx->activated) return;

  if (clock_gettime(CLOCK_MONOTONIC, &bp->seen) == -1){
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }

  dynbuf_append (
    strcmp(dest, "in") == 0 ? &ctx->in : &ctx->out,
    (char*)bp,
    sizeof(struct buffer_packet));
}

void traffic_capture_stop(struct capture_ctx* ctx) {
  if (!ctx->activated) return;

  FILE* fd = NULL;

  char *exts[] = {"emitted", "received"};
  struct dynbuf *dbs[] = {&ctx->in, &ctx->out};
  for (int i = 0; i < 2; i++) {
    size_t written = 0, ack = 0;
    char *out_file = NULL;

    asprintf(&out_file, "%s.%s", ctx->filename, exts[i]);
    if (out_file == NULL || (fd = fopen(out_file, "w")) == NULL) {
      perror("failed to open file");
      exit(EXIT_FAILURE);
    }
    free(out_file);

    while (written < dbs[i]->written) {
      ack = fwrite(dbs[i]->content+written, sizeof(char), dbs[i]->written, fd);
      if (ack <= 0) {
        perror("unable to write capture file");
        exit(EXIT_FAILURE);
      }
      written += ack;
    }

    fclose(fd);
    dynbuf_destroy (dbs[i]);
  }

  free(ctx->filename);
}
