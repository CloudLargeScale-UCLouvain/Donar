// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "stopwatch.h"

struct timing_fx _static_tfx;

struct timing_fx* static_tfx() { return &_static_tfx; }

void timing_fx_init(struct timing_fx* tfx, enum timing_config conf, char* startt, char* endt) {
  tfx->config = conf;
  strncpy (tfx->start_template, startt, sizeof(tfx->start_template) - 1);
  strncpy (tfx->end_template, endt, sizeof(tfx->end_template) - 1);
  tfx->start_template[sizeof(tfx->start_template) - 1] = 0; // Enforce null terminated string
  tfx->end_template[sizeof(tfx->end_template) - 1] = 0;
}

void timing_fx_start(struct timing_fx* tfx, ...) {
  va_list args;
  if (!(tfx->config & TIMING_ACTIVATED)) return;
  if (tfx->config & (TIMING_DISPLAY_START | TIMING_DISPLAY_BOTH)) {
    va_start(args, tfx);
    vfprintf(stderr, tfx->start_template, args);
    va_end(args);
  }

  if (clock_gettime(CLOCK_MONOTONIC_RAW, &tfx->start) == -1) {
    perror("clock_gettime");
    exit(EXIT_FAILURE);
  }
}

double timing_fx_stop(struct timing_fx* tfx, ...) {
  va_list args;
  struct timespec stop;
  double elapsed_in_cb;

  if (!(tfx->config & TIMING_ACTIVATED)) return 0.;

  if (clock_gettime(CLOCK_MONOTONIC_RAW, &stop) == -1) {
    perror("clock_gettime");
    exit(EXIT_FAILURE);
  }

  elapsed_in_cb = (double)elapsed_micros (&tfx->start, &stop) / 1000000.;

  if (tfx->config & (TIMING_DISPLAY_END | TIMING_DISPLAY_BOTH)) {
    va_start(args, tfx);
    vfprintf(stderr, tfx->end_template, args);
    fprintf(stderr, ": done in %f sec\n", elapsed_in_cb);
    va_end(args);
  }
  return elapsed_in_cb;
}

