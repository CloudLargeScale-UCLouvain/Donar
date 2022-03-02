// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "packet.h"

struct cap_file {
  FILE *fd;
  char* filename;
  size_t sz;
  size_t sz_obj;
};

void cap_load(struct cap_file *cf, char* filename);
void cap_next_bp(struct cap_file *cf, struct buffer_packet* bp);
void cap_peek_bp(struct cap_file *cf, struct buffer_packet* bp);
void cap_npeek_bp(struct cap_file *cf, int c, struct buffer_packet* bp);
size_t cap_count_bp(struct cap_file *cf);
void cap_begin(struct cap_file *cf);
void cap_unload(struct cap_file *cf);
