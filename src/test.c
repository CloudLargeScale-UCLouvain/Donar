// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "proxy.h"
#include "stopwatch.h"
#include <unistd.h>

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("~ test ~\n");

  timing_fx_init (static_tfx (), TIMING_ACTIVATED|TIMING_DISPLAY_END, "", "info=%s");
  timing_fx_start (static_tfx());
  sleep(1);
  timing_fx_stop(static_tfx(), "sleep(1)");

}
