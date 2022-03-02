// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#ifndef donar_stopwatch
#define donar_stopwatch

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "utils.h"

enum timing_config {
  TIMING_ACTIVATED = 1 << 0,
  TIMING_DISPLAY_START = 1 << 1,
  TIMING_DISPLAY_END = 1 << 2,
  TIMING_DISPLAY_BOTH = 1 << 3
};

struct timing_fx {
  struct timespec start;
  enum timing_config config;
  uint8_t activated_start;
  char start_template[255], end_template[255];
};

struct timing_fx* static_tfx();
void timing_fx_init(struct timing_fx* tfx, enum timing_config conf, char* startt, char* endt);
void timing_fx_start(struct timing_fx* tfx, ...);
double timing_fx_stop(struct timing_fx* tfx, ...);
#endif
