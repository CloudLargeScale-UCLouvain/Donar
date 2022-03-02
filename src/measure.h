// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <uuid/uuid.h>
#include "utils.h"

struct measure_params {
  uint64_t max_measure;
  uint64_t payload_size;
  uint64_t interval;
  uint8_t probe;
  uint8_t is_server;
  char* tag;
};

struct measure_state {
  uuid_t uuid;
  uint64_t* log;
  struct measure_packet* mp_out;
  struct measure_packet* mp_in;
  ssize_t mp_nin;
  int fd;
};

#pragma pack(1)
struct measure_packet {
  uint64_t counter;
  uint8_t flag;
  uint8_t probe;
  struct timespec emit_time;
};
#pragma pack()

void measure_params_init(struct measure_params* mp);
void measure_params_setpl (struct measure_params* mp, size_t plsize);
void measure_param_print(struct measure_params* mp);

void measure_state_init(struct measure_params* mp, struct measure_state* ms);
void measure_state_free(struct measure_state* ms);

void measure_parse(struct measure_params* mp, struct measure_state* ms, uint8_t verbose);
struct measure_packet* measure_generate(struct measure_params* mp, struct measure_state* ms);

void measure_next_tick(struct measure_params *mp, struct measure_state* ms, struct timespec *next);
void measure_summary(struct measure_params* mp, struct measure_state* ms);

