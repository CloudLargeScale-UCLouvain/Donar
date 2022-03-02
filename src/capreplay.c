// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>
#include <glib-2.0/glib-object.h>
#include "cap_utils.h"
#include "net_tools.h"

void get_ports(struct cap_file *cf) {
  struct buffer_packet bp;
  size_t entry_count = cap_count_bp (cf);
  for (int c = 0; c < entry_count; c++) {
    cap_next_bp (cf, &bp);
    union abstract_packet* ap = (union abstract_packet*) &bp.ip;
    if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) continue;
    int a = ap->fmt.content.udp_encapsulated.port;
  }
  cap_begin(cf);
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("~ capreplay ~\n");

  int opt, verbose = 0, is_active = 0;
  char *outf, *inf;
  while ((opt = getopt(argc, argv, "r:s:m:v")) != -1) {
      switch(opt) {
      case 'v':
        verbose++;
        break;
      case 'r':
        inf = optarg;
        break;
      case 's':
        outf = optarg;
        break;
      case 'm':
        is_active = strcmp(optarg, "active") == 0;
        break;
      default:
        goto usage;
      }
  }

  struct cap_file cf_in, cf_out;
  cap_load(&cf_in, inf);
  cap_load(&cf_out, outf);

  size_t nbp_in = cap_count_bp (&cf_in);
  size_t nbp_out = cap_count_bp (&cf_out);
  if (nbp_in < 1 || nbp_out < 1) {
    fprintf(stderr, "No buffer packet to read\n");
    exit(EXIT_FAILURE);
  }

  struct timespec started_rcv, started_snd;
  struct buffer_packet bp;

  // 1. init listening
  // 2. check if I should start
  // 3.

  cap_peek_bp (&cf_in, &bp); started_rcv = bp.seen;
  cap_peek_bp (&cf_out, &bp); started_snd = bp.seen;
  if (started_rcv.tv_sec < started_snd.tv_sec ||
      (started_rcv.tv_sec == started_snd.tv_sec && started_rcv.tv_nsec < started_snd.tv_nsec)) {
    // We need to wait to receive some packets before emitting as we are a server
  } else {
    // We are a client and we must emit packets now
  }

  //int fd = create_udp_client ("127.0.0.1", "5000");

  for (int c = 0; c < nbp_in; c++) {
    cap_next_bp (&cf_in, &bp);
    //sleep(bp.seen);
    // send on UDP to 127.13.3.7
    // sleep for given time
    //sendto(fd, bp.ip.ap.fmt.content.clear.payload);
  }

  return 0;

usage:
  fprintf(stderr, "Usage: %s -r FILE.0 -s FILE.1 -m [active|passive] [-v]\n", argv[0]);
  return 1;
}
