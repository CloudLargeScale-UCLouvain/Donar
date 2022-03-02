// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>
#include <glib-2.0/glib-object.h>
#include <errno.h>
#include "packet.h"
#include "evt_core.h"

#define KILOBYTE 1024l
#define MEGABYTE 1024l * KILOBYTE
#define GIGABYTE 1024l * MEGABYTE

struct captured_packet {
  struct timeval* captured_time;
  char* pkt;
};

struct dynbuf {
  char* content;
  size_t written;
  size_t alloced;
};

struct capture_ctx {
  uint8_t activated;
  char* filename;
  struct timeval* start_time;
  struct dynbuf in;
  struct dynbuf out;
};

void traffic_capture_init(struct capture_ctx* ctx, char* filename);
void traffic_capture_stop(struct capture_ctx* ctx);
void traffic_capture_notify(struct capture_ctx* ctx, struct buffer_packet *bp, char* dest);
