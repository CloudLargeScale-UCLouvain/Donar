// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "donar_client.h"

void load_onion_services(struct donar_client_ctx* ctx, char* onion_file, int ports_count) {
  tor_os_create (&(ctx->tos), onion_file, NULL, ports_count);
  tor_os_read (&(ctx->tos));
}

void init_socks5_client(struct donar_client_ctx* app_ctx, int pos) {
  char target_host[255];
  if (strlen(app_ctx->tos.keys[0].pub) > 254) {
    fprintf(stderr, "Domain name is too long\n");
    exit(EXIT_FAILURE);
  }
  sprintf(target_host, "%s.onion", app_ctx->tos.keys[0].pub);

  app_ctx->ports[pos] = app_ctx->base_port + pos;
  socks5_create_dns_client (&app_ctx->evts, app_ctx->tor_ip, app_ctx->tor_port, target_host, app_ctx->ports[pos]);
}

int on_socks5_success(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct evt_core_fdinfo fdinfo_n = {0};
  struct evt_core_cat cat_n = {0};
  char url[1024];
  struct socks5_ctx* s5ctx = fdinfo->other;
  fdinfo_n.cat = &cat_n;
  fdinfo_n.url = url;

  fdinfo_n.fd = dup(fdinfo->fd);
  fdinfo_n.cat->name = "tcp-write";
  sprintf(fdinfo_n.url, "tcp:write:127.0.0.1:%d", s5ctx->port);
  evt_core_add_fd (ctx, &fdinfo_n);

  fdinfo_n.fd = dup(fdinfo->fd);
  fdinfo_n.cat->name = "tcp-read";
  sprintf(fdinfo_n.url, "tcp:read:127.0.0.1:%d", s5ctx->port);
  evt_core_add_fd (ctx, &fdinfo_n);

  evt_core_rm_fd (ctx, fdinfo->fd);
  return 1;

failed:
  fprintf(stderr, "Memory allocation failed\n");
  exit(EXIT_FAILURE);
}

enum DONAR_TIMER_DECISION reinit_socks5(struct evt_core_ctx* ctx, void* user_data) {
  // @FIXME: Ugly way to get donar_client_ctx. Shame on me :/
  struct evt_core_cat* cat = evt_core_get_from_cat (ctx, "socks5-failed");
  if (cat == NULL) {
    fprintf(stderr, "Unable to reconnect stream as socks5-failed cat is not available...\n");
    exit(EXIT_FAILURE);
  }
  struct donar_client_ctx* app_ctx = cat->app_ctx;
  int64_t pos =  (int64_t) user_data; // trust me...

  fprintf(stdout, "[%s][donar-client] We have waited enough, retriggering socks5 for port %ld\n", current_human_datetime (), app_ctx->base_port+pos);
  init_socks5_client (app_ctx, pos);
  return DONAR_TIMER_STOP;
}

int on_socks5_failed(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct donar_client_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct socks5_ctx* s5ctx = fdinfo->other;
  int64_t pos = 0;
  pos = s5ctx->port - app_ctx->base_port;
  int64_t to_wait_sec = 30 + pos*3;

  fprintf(stdout, "[%s][donar-client] Retriggering socks5 in %ld seconds for port %d\n", current_human_datetime (), to_wait_sec, s5ctx->port);
  set_timeout(ctx, 1000 * to_wait_sec, (void*) pos, reinit_socks5);
  evt_core_rm_fd (ctx, fdinfo->fd);
  //init_socks5_client (app_ctx, pos);
  return 1;
}

void init_socks5_sinks(struct donar_client_ctx* app_ctx) {
  struct evt_core_cat template = { 0 };

  template.cb = on_socks5_success;
  template.name = "socks5-success";
  template.flags = EPOLLET;
  evt_core_add_cat(&app_ctx->evts, &template);

  template.cb = on_socks5_failed;
  template.app_ctx = app_ctx;
  template.name = "socks5-failed";
  template.flags = EPOLLET;
  evt_core_add_cat(&app_ctx->evts, &template);
}

int donar_client_stream_repair(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo) {
  struct evt_core_cat* cat = evt_core_get_from_cat (ctx, "socks5-failed");
  if (cat == NULL) {
    fprintf(stderr, "Unable to reconnect stream as socks5-failed cat is not available...\n");
    exit(EXIT_FAILURE);
  }
  struct donar_client_ctx* app_ctx = cat->app_ctx;

  fprintf(stdout, "[%s][donar-client] %s broke\n", current_human_datetime (), fdinfo->url);
  struct evt_core_fdinfo* fdtarget = NULL;
  int port = url_get_port_int (fdinfo->url);
  int64_t pos = 0, removed = 0;
  pos = port - app_ctx->base_port;
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

  if (removed == 2) {
    int64_t to_wait_sec = 10 + pos*3;
    fprintf(stdout, "[%s][donar-client] Retriggering socks5 in %ld seconds for port %d\n", current_human_datetime (), to_wait_sec, port);
    set_timeout(ctx, 1000 * to_wait_sec, (void*) pos, reinit_socks5);
    //init_socks5_client (app_ctx, pos);
    return 1;
  } else if (removed == 0) {
    fprintf(stdout, "[%s][donar-client] Socks5 has already been retriggered for port %d\n", current_human_datetime (), port);
    return 1;
  } else {
    fprintf(stderr, "[%s][donar-client] We only removed 1 link and not 2 for port %d\n", current_human_datetime (), port);
    //exit(EXIT_FAILURE);
  }
}

void donar_client(struct donar_client_ctx* ctx, struct donar_params* dp) {
  struct algo_params ap = {
    .is_waiting_bootstrap = dp->is_waiting_bootstrap,
    .algo_specific_params = dp->algo_specific_params,
    .algo_name = dp->algo,
    .links = dp->links,
    .fresh_data = dp->fresh_data,
    .redundant_data = dp->redundant_data,
    .capture_file = dp->capture_file,
    .base_port = dp->base_port,
    .sr = donar_client_stream_repair
  };
  ctx->tor_ip = dp->tor_ip;
  ctx->tor_port = dp->tor_port;
  ctx->base_port = dp->base_port;


  evt_core_init (&(ctx->evts), dp->verbose);

  signal_init(&ctx->evts);
  printf("--- Signal initialized\n");

  algo_main_init(&ctx->evts, &ap);
  printf("--- Algorithm initialized\n");

  socks5_init (&ctx->evts);
  init_socks5_sinks(ctx);
  printf("--- Socks5 connection process started\n");

  load_onion_services (ctx, dp->onion_file, dp->links);
  printf("--- Onion services loaded\n");

  init_timer(&ctx->evts);
  printf("--- Inited Timer\n");

  for (int64_t i = 0; i < dp->links; i++) {
    int64_t to_wait_sec =  i * 3;
    fprintf(stdout, "[%s][donar-client] Triggering socks5 in %ld seconds for port %ld\n", current_human_datetime (), to_wait_sec, i+dp->base_port);
    set_timeout(&ctx->evts, 1000 * to_wait_sec, (void*) i, reinit_socks5);
  }
  printf("--- TCP Clients Connected\n");

  g_ptr_array_foreach (dp->remote_ports, (void(*)(void*, void*))init_udp_remote, &(ctx->evts));
  printf("--- Remote ports are binded locally\n");

  for (int i = 0; i < dp->exposed_ports->len; i++) {
    init_udp_exposed(dp->bound_ip, g_ptr_array_index (dp->exposed_ports, i), &(ctx->evts));
  }
  printf("--- Local UDP services (on %s) are exposed\n", dp->bound_ip);

  evt_core_loop(&(ctx->evts));

  //stop_timer(&(ctx->evts));
  tor_os_free (&(ctx->tos));
}
