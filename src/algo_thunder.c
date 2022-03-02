// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <sys/timerfd.h>
#include "algo_utils.h"
#include "utils.h"
#include "url.h"
#include "proxy.h"
#include "timer.h"
#include "proxy.h"

#define PROBE_EVERY 2
uint64_t compute_delta(struct timespec* prev_time, uint64_t max, uint8_t update) {
  struct timespec curr;
  int secs, nsecs;
  uint64_t mili_sec;

  // 1. We compute the time difference
  if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1){
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }
  secs = curr.tv_sec - prev_time->tv_sec;
  nsecs = curr.tv_nsec - prev_time->tv_nsec;
  if(update) *prev_time = curr;
  mili_sec = secs * 1000 + nsecs / 1000000;
  if (mili_sec > max) mili_sec = max;

  return mili_sec;
}

int is_blacklisted(struct thunder_ctx* thunderc, int link_id) {
  if (thunderc->scheduler_activated)
    return thunderc->blacklisted[link_id] >= thunderc->received_pkts_on_link[link_id];

  return 0;
}

void prepare(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct thunder_ctx* thunderc = app_ctx->misc;

  thunderc->emit_id++;
  union abstract_packet metadata = {
    .fmt.headers.cmd = CMD_UDP_METADATA_THUNDER,
    .fmt.headers.size = sizeof(metadata.fmt.headers) + sizeof(metadata.fmt.content.udp_metadata_thunder),
    .fmt.headers.flags = 0,
    .fmt.content.udp_metadata_thunder.id = thunderc->emit_id,
  };
  buffer_append_ap (bp, &metadata);
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] UDP metadata added\n");
}

void pad(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct thunder_ctx* thunderc = app_ctx->misc;
  uint64_t ref = 0l + thunderc->emit_id;

  dup_buffer_toa (&app_ctx->br, bp, (void *)ref);

  // 1. Clean old buffers (we keep only thunderc->total_links buffer, keeping more would be useless)
  if (ref > thunderc->total_links && get_app_buffer (&app_ctx->br, (void *)(ref - thunderc->total_links))) {
    mv_buffer_atof (&app_ctx->br, (void *)(ref - thunderc->total_links));
  }

  // 2. Append abstract packets stored in our buffers
  uint64_t add_ref = ref;
  while(1) {
    if (add_ref < 1) break;
    add_ref--;
    struct buffer_packet *bp_iter = get_app_buffer (&app_ctx->br, (void *)add_ref);
    if (bp_iter == NULL) break;
    union abstract_packet *ap = buffer_first_ap (bp_iter);
    if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) {
      fprintf(stderr, "Invalid buffer payload!\n");
      exit(EXIT_FAILURE);
    }
    union abstract_packet *ap_meta = ap_next (ap);
    if (ap_meta->fmt.headers.cmd != CMD_UDP_METADATA_THUNDER) {
      fprintf(stderr, "Invalid buffer metadata!\n");
      exit(EXIT_FAILURE);
    }

    if (buffer_full_size (bp) + ap->fmt.headers.size + ap_meta->fmt.headers.size > TOR_CELL_SIZE - thunderc->monit_pkt_size) break;

    buffer_append_ap (bp, ap);
    buffer_append_ap (bp, ap_meta);
    if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] Pad packet (now %ld bytes)\n", buffer_full_size (bp));
  }
}

int schedule(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  char url[256];
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct thunder_ctx* thunderc = app_ctx->misc;
  struct evt_core_fdinfo *to_fdinfo = NULL;
  struct evt_core_cat* cat = evt_core_get_from_cat (ctx, "tcp-write");

  int64_t protect = thunderc->total_links;
  do {
    // 1. We choose the link
    if (cat->socklist->len == 0) {
      if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] No link available, packet will be dropped\n");
      break;
    }

    int64_t protect2 = thunderc->total_links;
    to_fdinfo = NULL;
    do {
      thunderc->selected_link = (thunderc->selected_link + 1) % thunderc->total_links;
      thunderc->to_increment[thunderc->selected_link]++;
      sprintf(url, "tcp:write:127.0.0.1:%d", 7500 + thunderc->selected_link);
      to_fdinfo = evt_core_get_from_url (ctx, url);
    } while (to_fdinfo == NULL && --protect2 >= 0);
    if (protect2 < 0) {
      fprintf(stderr, " [algo_thunder] scheduler/no link available\n");
      goto schedule_release;
    }

    // We let some time for blacklisted links to recover
    if (is_blacklisted (thunderc, thunderc->selected_link) && thunderc->to_increment[thunderc->selected_link] < PROBE_EVERY) continue;

    //printf("URL %s has been retained\n", url);

    // 2. We create the packet template
    union abstract_packet links = {
      .fmt.headers.cmd = CMD_LINK_MONITORING_THUNDER,
      .fmt.headers.size = thunderc->monit_pkt_size,
      .fmt.headers.flags = 0,
      .fmt.content.link_monitoring_thunder.to_increment = thunderc->to_increment[thunderc->selected_link],
      .fmt.content.link_monitoring_thunder.links_status = {}
    };
    thunderc->to_increment[thunderc->selected_link] = 0;

    // 3. We append the template to the buffer
    struct buffer_packet* bp_dup = dup_buffer_tow (&app_ctx->br, bp, to_fdinfo);
    union abstract_packet *new_ap = buffer_append_ap (bp_dup, &links);

    // 4. We compute the time difference
    uint64_t mili_sec = 0;
    if (protect == thunderc->total_links)
      mili_sec = compute_delta (&thunderc->prev_link_time, UINT16_MAX, 1);

    //printf("send packet on link %d with delta=%ld\n", thunderc->selected_link, mili_sec);
    // 5. We create the array
    struct link_info *li = &new_ap->fmt.content.link_monitoring_thunder.links_status;
    for (int i = 0; i < thunderc->total_links; i++) {
      if (thunderc->sent_pkts_on_link[i] == 0) continue;
      thunderc->delta_t_per_link[i] += mili_sec;
      li[i].delta_t = thunderc->delta_t_per_link[i] > UINT16_MAX ? UINT16_MAX : thunderc->delta_t_per_link[i];
    }
    thunderc->delta_t_per_link[thunderc->selected_link] = 0;
    li[thunderc->selected_link].delta_t = 0;
    thunderc->sent_pkts_on_link[thunderc->selected_link]++;

    if (ctx->verbose > 1) {
      dump_buffer_packet(bp_dup);
      fprintf(stderr, "    [algo_thunder] Will send this info\n");
    }
    main_on_tcp_write(ctx, to_fdinfo);

  } while (is_blacklisted (thunderc, thunderc->selected_link) && --protect >= 0);
  
  if (protect < 0) {
    fprintf(stderr, "all links were blacklisted, resetting\n");
    for (int i = 0; i < thunderc->total_links; i++) {
      fprintf(stderr, "  link=%d, blacklisted=%ld, rcved=%ld\n", i, thunderc->blacklisted[i], thunderc->received_pkts_on_link[i]);
      thunderc->received_pkts_on_link[i] = thunderc->blacklisted[i] + 1;
    }
  }

  if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] Packets sent\n");

schedule_release:
  // Release the buffer
  mv_buffer_rtof (&app_ctx->br, fdinfo);

  return 0;
}

struct block_info {
  uint8_t i;
  struct algo_ctx* app_ctx;
  uint64_t missing;
  uint8_t is_timeout;
  char reason[1024];
};

enum DONAR_TIMER_DECISION on_block (struct evt_core_ctx* ctx, void* raw) {
  struct block_info* bi = raw;
  struct thunder_ctx* thunderc = bi->app_ctx->misc;

  if (bi->is_timeout && thunderc->received_pkts_on_link[bi->i] >= bi->missing) goto release;
  if (thunderc->blacklisted[bi->i] >= bi->missing) goto release;

  printf("%s\n", bi->reason);
  thunderc->blacklisted[bi->i] = bi->missing;

release:
  if (bi->is_timeout) free(bi);
  return DONAR_TIMER_STOP;
}

int is_in_order(struct thunder_ctx* thunderc, uint8_t link_id) {
  uint64_t ref = thunderc->received_pkts_on_link[link_id];
  for (int i = 0; i < thunderc->total_links; i++) {
    uint64_t expected = link_id >= i ? ref : ref - 1;
    if (thunderc->received_pkts_on_link[i] > expected) {
      //printf("link_id=%d, i=%d, pkt_i=%ld, pkt_i_expected=%ld, pkt_link_id=%ld\n", link_id, i, thunderc->received_pkts_on_link[i], expected, ref);
      return 0;
    }
  }

  return 1;
}

void classify(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct thunder_ctx* thunderc = app_ctx->misc;
  union abstract_packet* ap = buffer_first_ap (bp);
  while (ap != NULL && ap->fmt.headers.cmd != CMD_LINK_MONITORING_THUNDER) ap = ap_next(ap);
  if (ap == NULL) {
    fprintf(stderr, "Unable to find our packet\n");
    exit(EXIT_FAILURE);
  }

  // 1. Update link info
  int link_id = url_get_port_int(fdinfo->url) - 7500;
  thunderc->received_pkts_on_link[link_id] += ap->fmt.content.link_monitoring_thunder.to_increment;
  //printf("Received %ld packets on link %d\n", thunderc->received_pkts_on_link[link_id], link_id);
  struct link_info *li = &ap->fmt.content.link_monitoring_thunder.links_status;

  uint64_t mili_sec = compute_delta (&thunderc->prev_rcv_link_time, UINT16_MAX, 1);
  for (int i = 0; i < thunderc->total_links; i++) {
    if (thunderc->received_pkts_on_link[i] <= 1) continue;
    thunderc->rcv_delta_t_per_link[i] += mili_sec;
  }
  thunderc->rcv_delta_t_per_link[link_id] = 0;

  // 3. Disable links that miss packets
  for (uint8_t i = 0; i < thunderc->total_links; i++) {
    uint64_t expected = i <= link_id ? thunderc->received_pkts_on_link[link_id] : thunderc->received_pkts_on_link[link_id] - 1;
    if (thunderc->received_pkts_on_link[i] >= expected) continue; // Nothing to do, all packets have been received
    int64_t timeout = thunderc->allowed_jitter_ms - li[i].delta_t;

    struct block_info *bi = malloc(sizeof(struct block_info));
    bi->i = i; bi->app_ctx = app_ctx; bi->missing = expected; bi->is_timeout = 1;

    sprintf(bi->reason, "  Missing Packet - Timeout for link %d after %ldms (expected: %ld, seen: %ld)", i, timeout, expected, thunderc->received_pkts_on_link[i]);
    if (timeout <= 0) {
      on_block(ctx, bi);
      continue;
    }

    set_timeout (ctx, timeout, bi, on_block);
  }
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] Classify done\n");

  if (thunderc->scheduler_activated) {
    uint64_t ts = compute_delta (&thunderc->start_time, UINT64_MAX, 0);
    printf("[%ld] Blacklisted links: ", ts);
    for (int i = 0; i < thunderc->total_links; i++) {
      if (is_blacklisted (thunderc, i)) printf("_");
      else printf("U");
    }
    printf("\n");
  }
}

struct unpad_info {
  union abstract_packet *ap_arr_pl[MAX_LINKS],
  *ap_arr_meta[MAX_LINKS];
  uint8_t ap_arr_vals;
};

void unpad(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp, struct unpad_info *ui) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct thunder_ctx* thunderc = app_ctx->misc;

  for (union abstract_packet* ap = buffer_first_ap (bp); ap != NULL; ap = ap_next(ap)) {
    if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) continue;

    union abstract_packet* ap_meta = ap_next(ap);
    if (ap_meta == NULL || ap_meta->fmt.headers.cmd != CMD_UDP_METADATA_THUNDER) {
      fprintf(stderr, "Unexpected packet, expecting udp metadata\n");
    }

    ui->ap_arr_pl[ui->ap_arr_vals] = ap;
    ui->ap_arr_meta[ui->ap_arr_vals] = ap_meta;
    ui->ap_arr_vals++;
  }
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] Unpad done\n");
}

int compare_int64(const void *a,const void *b) {
  int64_t *x = (int64_t *) a;
  int64_t *y = (int64_t *) b;
  return *x - *y;
}

void adapt(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp, struct unpad_info *ui) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct thunder_ctx* thunderc = app_ctx->misc;
  char url[256];
  struct evt_core_fdinfo *to_fdinfo = NULL;
  uint64_t delivered = 0;


  for (int i = ui->ap_arr_vals-1; i >= 0; i--) {
    //fprintf(stderr, "i=%d, ui->ap_arr_vals=%d\n", i, ui->ap_arr_vals);
    if (ui->ap_arr_meta[i]->fmt.content.udp_metadata_thunder.id <= thunderc->recv_id) continue; // already delivered
    thunderc->recv_id = ui->ap_arr_meta[i]->fmt.content.udp_metadata_thunder.id;

    // Find destination
    sprintf(url, "udp:write:127.0.0.1:%d", ui->ap_arr_pl[i]->fmt.content.udp_encapsulated.port);
    to_fdinfo = evt_core_get_from_url (ctx, url);
    if (to_fdinfo == NULL) {
      fprintf(stderr, "No fd for URL %s in tcp-read. Dropping packet :( \n", url);
    }

    struct buffer_packet *bp_dest = inject_buffer_tow (&app_ctx->br, to_fdinfo);
    bp_dest->mode = BP_WRITING;
    //dump_buffer_packet (bp_dest);
    buffer_append_ap (bp_dest, ui->ap_arr_pl[i]);
    main_on_udp_write(ctx, to_fdinfo);
    delivered++;
  }

  printf("[algo_thunder] Delivered %ld packets (now id=%d)\n", delivered, thunderc->recv_id);
  if (delivered > 4) {
    dump_buffer_packet (bp);
  }

  mv_buffer_rtof (&app_ctx->br, fdinfo);
  if (ctx->verbose > 1) fprintf(stderr, "    [algo_thunder] Adapt done\n");
}

int algo_thunder_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct unpad_info ui = {0};

  classify(ctx, fdinfo, bp);
  unpad(ctx, fdinfo, bp, &ui);
  adapt(ctx, fdinfo, bp, &ui);
  return 0;
}

int algo_thunder_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  prepare(ctx, fdinfo, bp);
  pad(ctx, fdinfo, bp);
  schedule(ctx, fdinfo, bp);
  return 0;
}

void algo_thunder_free(void* v) {
  struct rr_ctx* rr = v;
  free(rr);
}

void algo_thunder_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap) {
  app_ctx->misc = malloc(sizeof(struct thunder_ctx));
  app_ctx->free_misc = algo_thunder_free;
  if (app_ctx->misc == NULL) {
    perror("malloc failed in algo thunder init");
    exit(EXIT_FAILURE);
  }
  memset(app_ctx->misc, 0, sizeof(struct thunder_ctx));
  struct thunder_ctx* thunderc = app_ctx->misc;
  thunderc->recv_id = 1;
  thunderc->emit_id = 1;
  thunderc->total_links = app_ctx->ap.links;
  thunderc->selected_link = thunderc->total_links - 1;
  thunderc->allowed_jitter_ms = 200;
  for (int i = 0; i < thunderc->total_links; i++) {
    thunderc->received_pkts_on_link[i] = 1;
    thunderc->rcv_delta_t_per_link[i] = 0;
  }
  if (clock_gettime(CLOCK_MONOTONIC, &thunderc->start_time) == -1){
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }

  union abstract_packet links = {};
  //fprintf(stderr, "Total links %d\n", thunderc->total_links);
  thunderc->monit_pkt_size = sizeof(links.fmt.headers) + sizeof(links.fmt.content.link_monitoring_thunder) + sizeof(struct link_info) * (thunderc->total_links - 1);

  if (ap->algo_specific_params != NULL) {
    char *parse_ptr, *token, *params;

    for (params = ap->algo_specific_params; ; params = NULL) {
      token = strtok_r(params, ",", &parse_ptr);
      if (token == NULL) break;
      sscanf(token, "jitter=%ld", &thunderc->allowed_jitter_ms);
      sscanf(token, "scheduler=%d", &thunderc->scheduler_activated);
    }
  }

  printf("Allowed jitter set to %ld ms\n", thunderc->allowed_jitter_ms);
  printf("Scheduler is %s\n", thunderc->scheduler_activated ? "activated" : "deactivated");

  init_timer(ctx);
}

int algo_thunder_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo) {
  if (strstr(fdinfo->cat->name, "udp") != NULL) return 1;
  return 0;
}
