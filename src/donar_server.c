// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "donar_server.h"

void create_onion_services(struct tor_os_str* tos, struct tor_ctl* tctl, char* tor_ip, char *tor_port, uint16_t* ports, int ports_count, enum TOR_ONION_FLAGS tof) {
    tor_os_create (tos, "onion_services.pub", "onion_services.txt", 1);
    tor_os_read (tos);

    int err = 0;
    err = tor_ctl_connect (tctl, tor_ip, tor_port);
    if (err < 0) {
      fprintf(stderr, "Unable to open Tor Socket\n");
      exit(EXIT_FAILURE);
    }
    err = tor_ctl_add_onion (tctl, tos, ports, ports_count, tof);
    if (err != 0) {
      fprintf(stderr, "Unable to create Onion Services (error: %d)\n", err);
      exit(EXIT_FAILURE);
    }
}

void destroy_resources(struct tor_os_str* tos, struct tor_ctl* tctl) {
  tor_ctl_close (tctl);
  tor_os_free (tos);
}

void init_tcp_servers(struct donar_server_ctx* ctx, int nlinks) {
  char url[1024];
  struct evt_core_cat cat = {0};
  struct evt_core_fdinfo fdinfo = {0};
  fdinfo.cat = &cat;
  fdinfo.url = url;

  char buffer[6];
  int err, sock = 0;
  for (int i = 0; i < nlinks; i++) {
    sprintf (buffer, "%d", ctx->ports[i]);
    //@FIXME shouldn't listen on 0.0.0.0 but 127.13.3.7 is not compatible with docker
    sock = create_tcp_server ("0.0.0.0", buffer);
    if (sock < 0) goto socket_create_err;
    err = listen(sock, SOMAXCONN);
    if (err != 0) goto socket_create_err;

    fdinfo.cat->name = "tcp-listen";
    fdinfo.fd = sock;
    sprintf(fdinfo.url, "tcp:listen:127.0.0.1:%d", ctx->ports[i]);
    evt_core_add_fd(&(ctx->evts), &fdinfo);
  }
  return;

socket_create_err:
  fprintf(stderr, "Unable to create a TCP socket\n");
  exit(EXIT_FAILURE);
}

struct tor_ctl* ugly_global_tctl;
int donar_server_stream_repair(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  fprintf(stdout, "[%s][donar-server] %s broke\n", current_human_datetime (), fdinfo->url);

  struct evt_core_fdinfo* fdtarget = NULL;
  int port = url_get_port_int (fdinfo->url);
  int removed = 0;
  char buffer[256];

  sprintf(buffer, "tcp:read:127.0.0.1:%d", port);
  fdtarget = evt_core_get_from_url (ctx, buffer);
  if (fdtarget != NULL) {
    evt_core_rm_fd(ctx, fdtarget->fd);
    removed++;
  }

  sprintf(buffer, "tcp:write:127.0.0.1:%d", port);
  fdtarget = evt_core_get_from_url (ctx, buffer);
  if (fdtarget != NULL) {
    evt_core_rm_fd(ctx, fdtarget->fd);
    removed++;
  }
  printf("[%s][donar-server] removed %d links\n", current_human_datetime (), removed);

  return 1;
}

void donar_server(struct donar_server_ctx* ctx,  struct donar_params* dp) {
  struct algo_params ap = {
    .is_waiting_bootstrap = dp->is_waiting_bootstrap,
    .algo_specific_params = dp->algo_specific_params,
    .algo_name = dp->algo,
    .links = dp->links,
    .fresh_data = dp->fresh_data,
    .redundant_data = dp->redundant_data,
    .capture_file = dp->capture_file,
    .base_port = dp->base_port,
    .sr = donar_server_stream_repair
  };

  evt_core_init (&(ctx->evts), dp->verbose);

  signal_init(&ctx->evts);
  printf("--- Signal initialized\n");

  algo_main_init(&ctx->evts, &ap);
  printf("--- Algorithm initialized\n");

  for (uint16_t i = 0; i < dp->links ; i++) {
    ctx->ports[i] = dp->base_port + i;
  }

  init_tcp_servers(ctx, dp->links);
  printf("--- TCP servers are listening\n");

  ctx->tctl.os_endpoint = dp->my_ip_for_tor;
  create_onion_services (&(ctx->tos), &(ctx->tctl), dp->tor_ip, dp->tor_port, ctx->ports, dp->links, dp->tof);
  ugly_global_tctl = &(ctx->tctl);
  printf("--- Onion services created\n");

  g_ptr_array_foreach (dp->remote_ports, (void(*)(void*, void*))init_udp_remote, &(ctx->evts));
  printf("--- Remote ports are binded locally\n");

  for (int i = 0; i < dp->exposed_ports->len; i++) {
    init_udp_exposed(dp->bound_ip, g_ptr_array_index (dp->exposed_ports, i), &(ctx->evts));
  }
  printf("--- Local UDP services (on %s) are exposed\n", dp->bound_ip);

  evt_core_loop (&(ctx->evts));

  //stop_timer(&(ctx->evts));
  destroy_resources (&(ctx->tos), &(ctx->tctl));
}
