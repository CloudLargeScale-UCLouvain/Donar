// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gmodule.h>
#include "donar_client.h"
#include "donar_server.h"

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("~ Donar ~\n");

    timing_fx_init (static_tfx(), TIMING_ACTIVATED | TIMING_DISPLAY_END, "", "fn=%s");

    struct donar_params dp = {0};
    donar_init_params (&dp);

    while ((dp.opt = getopt(argc, argv, "nvcse:r:o:a:bl:d:f:i:p:t:q:k:")) != -1) {
      switch(dp.opt) {
      case 'q':
        dp.tor_port = optarg;
        break;
      case 'k':
        dp.base_port = atoi(optarg);
        break;
      case 'n':
        dp.tof |= TOR_ONION_FLAG_NON_ANONYMOUS;
        break;
      case 'v':
        dp.verbose++;
        break;
      case 's':
        dp.is_server = 1;
        break;
      case 'e':
        dp.port = strdup(optarg);
        if (dp.port == NULL) goto terminate;
        g_ptr_array_add (dp.exposed_ports, dp.port);
        break;
      case 'r':
        dp.port = strdup(optarg);
        if (dp.port == NULL) goto terminate;
        g_ptr_array_add (dp.remote_ports, dp.port);
        break;
      case 'o':
        dp.onion_file = strdup(optarg);
        break;
      case 'c':
        dp.is_client = 1;
        break;
      case 'a':
        dp.algo = strdup(optarg);
        break;
      case 'p':
        dp.algo_specific_params = strdup(optarg);
        break;
      case 'b':
        dp.is_waiting_bootstrap = 1;
        break;
      case 'l':
        dp.links = atoi(optarg);
        break;
      case 'd':
        sscanf(optarg, "%d,%d", &dp.fresh_data, &dp.redundant_data);
        break;
      case 'i':
        dp.bound_ip = strdup(optarg);
        break;
      case 'f':
        dp.capture_file = strdup(optarg);
        break;
      case 't':
        sscanf(optarg, "%[^!]!%[^!]", dp.tor_ip, dp.my_ip_for_tor);
        break;
      default:
        goto in_error;
      }
    }

    if (!(dp.is_server ^ dp.is_client)) goto in_error;
    if (dp.bound_ip == NULL) dp.bound_ip = strdup("127.0.0.1");
    if (dp.tor_port == NULL) dp.tor_port = dp.is_client ? "9050" : "9051";
    if (dp.algo == NULL) goto in_error;

    fprintf(stderr, "Passed parameters: is_waiting_bootstrap=%d, client=%d, server=%d, algo=%s, exposed_ports=%d, remote_ports=%d, transfer_base_port=%d, tor_daemon_port=%s, onion_file=%s, links=%d, duplication=%d,%d\n",
      dp.is_waiting_bootstrap, dp.is_client, dp.is_server, dp.algo, dp.exposed_ports->len, dp.remote_ports->len, dp.base_port, dp.tor_port, dp.onion_file, dp.links, dp.fresh_data, dp.redundant_data);


    if (dp.is_server) {
      struct donar_server_ctx ctx;
      if (dp.exposed_ports->len < 1 && dp.remote_ports->len < 1) goto in_error;
      donar_server(&ctx, &dp);
    } else if (dp.is_client) {
      struct donar_client_ctx ctx;
      if ((dp.exposed_ports->len < 1 && dp.remote_ports->len < 1) || dp.onion_file == NULL) goto in_error;
      donar_client(&ctx, &dp);
    }
    goto terminate;

in_error:
    dp.errored = 1;
    fprintf(stderr, "Usage as client : %s -c -a <algo> -o <onion service file> [-h] [-b] [-i <bound ip>] [-f <dump packets>] [-l <links>] [-d <fresh>,<red>] [-e <exposed udp port>]* [-r <remote udp port>]*\n", argv[0]);
    fprintf(stderr, "Usage as server : %s -s -a <algo> [-h] [-b] [-n] [-i <bound ip>] [-l <links>] [-f <dump_packets>] [-d <fresh>,<red>] [-e <exposed udp port>]* [-r <remote udp port>]*\n\n", argv[0]);
    fprintf(stderr, "Passed parameters: client=%d, server=%d, algo=%s, exposed_ports=%d, remote_ports=%d, transfer_base_port=%d, tor_daemon_port=%s, onion_file=%s, links=%d, duplication=%d,%d\n",
            dp.is_client, dp.is_server, dp.algo, dp.exposed_ports->len, dp.remote_ports->len, dp.base_port, dp.tor_port, dp.onion_file, dp.links, dp.fresh_data, dp.redundant_data);

terminate:
    // @FIXME: Should be refactored in free_donar_params()
    if (dp.onion_file != NULL) free(dp.onion_file);
    if (dp.algo != NULL) free(dp.algo);
    if (dp.capture_file != NULL) free(dp.capture_file);
    if (dp.bound_ip != NULL) free(dp.bound_ip);
    if (dp.algo_specific_params != NULL) free(dp.algo_specific_params);

    g_ptr_array_free(dp.exposed_ports, TRUE);
    g_ptr_array_free(dp.remote_ports, TRUE);

    return dp.errored;
}
