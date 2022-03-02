// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "cap_utils.h"

void cap_load(struct cap_file *cf, char* filename) {
  if ((cf->fd = fopen(filename, "r")) == NULL) {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }
  cf->filename = filename;
  fseek(cf->fd, 0, SEEK_END);
  cf->sz = ftell(cf->fd);
  fseek(cf->fd, 0, SEEK_SET);

  cf->sz_obj = cf->sz / sizeof(struct buffer_packet);
}

void cap_next_bp(struct cap_file *cf, struct buffer_packet* bp) {
  fread(bp, sizeof(struct buffer_packet), 1, cf->fd);
}

void cap_npeek_bp(struct cap_file *cf, int c, struct buffer_packet* bp) {
  //fprintf(stdout, "c: %d, current pos: %ld, ", c, ftell(cf->fd));

  ssize_t cbytes = c*sizeof(struct buffer_packet);
  fseek(cf->fd, cbytes, SEEK_CUR);
  size_t n = fread(bp, sizeof(struct buffer_packet), 1, cf->fd);
  fseek(cf->fd, -(n*sizeof(struct buffer_packet) + cbytes), SEEK_CUR);
  //fprintf(stdout, "cbytes: %ld, n: %ld, final pos: %ld\n", cbytes, n, ftell(cf->fd));
}

void cap_peek_bp(struct cap_file *cf, struct buffer_packet* bp) {
  cap_npeek_bp(cf, 0, bp);
}

size_t cap_count_bp(struct cap_file *cf) {
  return cf->sz / sizeof(struct buffer_packet);
}

void cap_unload(struct cap_file *cf) {
  fclose(cf->fd);
}
void cap_begin(struct cap_file *cf) {
  fseek(cf->fd, 0L, SEEK_SET);
}
