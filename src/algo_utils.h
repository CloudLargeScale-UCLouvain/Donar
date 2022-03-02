// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>
#include <glib-2.0/glib-object.h>
#include "packet.h"
#include "evt_core.h"
#define PACKET_BUFFER_SIZE 1024

struct buffer_resources {
  struct buffer_packet bps[PACKET_BUFFER_SIZE];
  GQueue* free_buffer;              // Available buffers
  GHashTable* used_buffer;          // Buffers used for reading or writing
  GQueue* read_waiting;             // Who wait to be notified for a read
  GHashTable* application_waiting;  // Structure that can be used by the algo for its internal logic
  GHashTable* write_waiting;        // Structure to track packets waiting to be written
};

void init_buffer_management(struct buffer_resources* br);
void destroy_buffer_management(struct buffer_resources* br);
void mv_buffer_rtow(struct buffer_resources* app_ctx, struct evt_core_fdinfo* from, struct evt_core_fdinfo* to);
void mv_buffer_rtof(struct buffer_resources* app_ctx, struct evt_core_fdinfo* from);
void mv_buffer_wtof(struct buffer_resources* app_ctx, struct evt_core_fdinfo* from);
void mv_buffer_rtoa(struct buffer_resources* app_ctx, struct evt_core_fdinfo* from, void* to);
void mv_buffer_atow(struct buffer_resources* app_ctx, void* from, struct evt_core_fdinfo* to);
void mv_buffer_atof(struct buffer_resources* app_ctx, void* from);

struct buffer_packet* inject_buffer_tow(struct buffer_resources *app_ctx, struct evt_core_fdinfo* to);
struct buffer_packet* dup_buffer_tow(struct buffer_resources* app_ctx, struct buffer_packet* bp, struct evt_core_fdinfo* to);
struct buffer_packet* dup_buffer_toa(struct buffer_resources* app_ctx, struct buffer_packet* bp, void* to);
guint write_queue_len(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo);

struct buffer_packet* get_write_buffer(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo);
struct buffer_packet* get_read_buffer(struct buffer_resources *app_ctx, struct evt_core_fdinfo *fdinfo);
struct buffer_packet* get_app_buffer(struct buffer_resources *app_ctx, void* idx);

void notify_read(struct evt_core_ctx* ctx, struct buffer_resources* app_ctx);

