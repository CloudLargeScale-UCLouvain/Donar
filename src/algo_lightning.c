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
#include "measure.h"
#define HISTORIC_SIZE 2048
#define MAX_LINKS 64

enum ooo_state {
  IN_ORDER,
  OOO_ONGOING,
  OOO_DONE
};

char* ooo_state_str[] = {
  "IN_ORDER",
  "OOO_ONGOING",
  "OOO_DONE"
};


enum schedule_group_target {
  SCHEDULE_BOTH = 0,
  SCHEDULE_FAST = 1,
  SCHEDULE_SLOW = 2
};

int schedule_group_target_trans[] = {
  SCHEDULE_BOTH,
  SCHEDULE_SLOW,
  SCHEDULE_FAST
};

char* schedule_group_target_str[] = {
  "SCHEDULE_BOTH",
  "SCHEDULE_FAST",
  "SCHEDULE_SLOW"
};

enum link_cat {
  LINK_FAST = 0,
  LINK_SLOW = 1,
  LINK_NOT_USED = 2
};

char* link_cat_str[] = {
  "LINK_FAST",
  "LINK_SLOW",
  "LINK_NOT_USED"
};

struct stat_entry {
  uint8_t link_id;
  int64_t ooo;
  int64_t meas_occ;
};

struct timing_entry {
  enum ooo_state state;
  struct timespec detected_at;
  struct timespec finished_at;
  uint8_t link_id;
  uint64_t pkt_id;
};

struct link_status {
  struct timespec last;
  enum link_cat used;
};

struct light_ctx {
  uint8_t prev_links[MAX_LINKS];
  int16_t remote_stats[MAX_LINKS];
  int16_t local_stats[MAX_LINKS];
  struct timing_entry historic[HISTORIC_SIZE];
  struct link_status status[MAX_LINKS];
  uint8_t active;
  uint64_t pkt_rcv_id;
  uint64_t pkt_sent_id;
  uint64_t uniq_pkt_sent_id;
  uint8_t selected_link;
  uint8_t total_links;
  int fast_count;
  int sent_past_links;
  struct timespec window;
  struct timespec last_update_used;
  size_t monit_pkt_size;
  int csv;
  int is_measlat;
  int explain;
  int disable_scheduler;
  int base_port;
  struct stat_entry stats[MAX_LINKS];
  enum schedule_group_target sched_strat;
};

void algo_lightning_free(void* v) {
  struct light_ctx* lightc = v;
  free(lightc);
}

void algo_lightning_init(struct evt_core_ctx* ctx, struct algo_ctx* app_ctx, struct algo_params* ap) {
  app_ctx->misc = malloc(sizeof(struct light_ctx));
  app_ctx->free_misc = algo_lightning_free;
  if (app_ctx->misc == NULL) {
    perror("malloc failed in algo lightning init");
    exit(EXIT_FAILURE);
  }
  memset(app_ctx->misc, 0, sizeof(struct light_ctx));
  struct light_ctx* lightc = app_ctx->misc;
  lightc->total_links = app_ctx->ap.links;
  lightc->selected_link = lightc->total_links - 1;
  lightc->sent_past_links = lightc->total_links / 2;
  lightc->fast_count = lightc->total_links / 4;
  lightc->csv = 0;
  lightc->explain = 0;
  lightc->pkt_sent_id = 1;
  lightc->uniq_pkt_sent_id = 1;
  lightc->disable_scheduler = 0;
  lightc->active = 0;
  lightc->sched_strat = SCHEDULE_BOTH;
  lightc->base_port = ap->base_port;

  uint64_t window = 2000;
  if (ap->algo_specific_params != NULL) {
    char *parse_ptr, *token, *params;

    for (params = ap->algo_specific_params; ; params = NULL) {
      token = strtok_r(params, "!", &parse_ptr);
      if (token == NULL) break;
      sscanf(token, "fast_count=%d", &lightc->fast_count);
      sscanf(token, "window=%ld", &window);
      sscanf(token, "sent_past_links=%d", &lightc->sent_past_links);
      sscanf(token, "csv=%d", &lightc->csv);
      sscanf(token, "measlat=%d", &lightc->is_measlat);
      sscanf(token, "explain=%d", &lightc->explain);
      sscanf(token, "disable_scheduler=%d", &lightc->disable_scheduler);
      sscanf(token, "tick_tock=%d", &lightc->sched_strat);
    }
  }

  for (int i = 0; i < lightc->sent_past_links; i++)
    lightc->prev_links[i] = UINT8_MAX;

  for (int i = 0; i < lightc->total_links; i++) {
    lightc->status[i].used = LINK_NOT_USED;
  }

  union abstract_packet m;
  lightc->monit_pkt_size =
    sizeof(m.fmt.headers) +
    sizeof(m.fmt.content.link_monitoring_lightning) +
    sizeof(uint8_t) * (lightc->sent_past_links - 1) +
    sizeof(int16_t) * lightc->total_links;
  timespec_set_unit (&lightc->window, window, MILISEC);

  printf("fast_count = %d\n", lightc->fast_count);
  printf("window check = %ld ms\n", window);
  printf("sent_past_links = %d\n", lightc->sent_past_links);
  printf("csv = %s\n", lightc->csv ? "yes" : "no");
  printf("measlat = %s\n", lightc->is_measlat ? "yes" : "no");
  printf("explain = %s\n", lightc->explain ? "yes" : "no");
  printf("disable_scheduler = %s\n", lightc->disable_scheduler ? "yes" : "no");
  printf("schedule_group_target = %s\n", schedule_group_target_str[lightc->sched_strat]);
}

void algo_lightning_pad(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct light_ctx* lightc = app_ctx->misc;
  uint64_t ref = lightc->uniq_pkt_sent_id;

  // 0. Store current buffer to application
  //fprintf(stderr, "    [algo_lightning] Store buffer with pointer %p\n", (void*) ref);
  dup_buffer_toa (&app_ctx->br, bp, (void *)ref);

  // 1. Clean old buffers (we keep only thunderc->total_links buffer, keeping more would be useless)
  //fprintf(stderr, "    [algo_lightning] Clean queue\n");
  if (ref > lightc->total_links && get_app_buffer (&app_ctx->br, (void *)(ref - lightc->total_links))) {
    mv_buffer_atof (&app_ctx->br, (void *)(ref - lightc->total_links));
  }

  // 2. Append abstract packets stored in our buffers
  uint64_t add_ref = ref;
  while(1) {
    //fprintf(stderr, "    [algo_lightning] Enter loop with ref %ld\n", add_ref);
    if (add_ref < 1) {
      //fprintf(stderr, "    [algo_lightning] add_ref=%ld < 1\n", add_ref);
      break;
    }
    add_ref--;
    struct buffer_packet *bp_iter = get_app_buffer (&app_ctx->br, (void *)add_ref);
    if (bp_iter == NULL) {
      //fprintf(stderr, "    [algo_lightning] bp_iter=%p == NULL\n", bp_iter);
      break;
    }
    union abstract_packet *ap = buffer_first_ap (bp_iter);
    if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) {
      //fprintf(stderr, "Invalid buffer payload!\n");
      exit(EXIT_FAILURE);
    }

    //fprintf(stderr, "    [algo_lightning] Currently %ld bytes, would be %ld\n", buffer_full_size (bp), buffer_full_size (bp) + ap->fmt.headers.size);

    if (buffer_full_size (bp) + ap->fmt.headers.size > TOR_CELL_SIZE - lightc->monit_pkt_size) break;

    buffer_append_ap (bp, ap);
    if (ctx->verbose > 1) fprintf(stderr, "    [algo_lightning] Pad packet (now %ld bytes)\n", buffer_full_size (bp));
  }
}

void monitoring(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct light_ctx* lightc = app_ctx->misc;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;

  while (ap != NULL && ap->fmt.headers.cmd != CMD_LINK_MONITORING_LIGHTNING) ap = ap_next(ap);
  if (ap == NULL) {
    fprintf(stderr, "[algo_lightning] Unable to find our monitoring information\n");
    exit(EXIT_FAILURE);
  }

  uint8_t *prev_links = &ap->fmt.content.link_monitoring_lightning.dyn_struct;
  int16_t *remote_stats = (int16_t*)(prev_links + sizeof(uint8_t) * lightc->sent_past_links);

  if (lightc->explain) {
    printf("(monitoring.stats) ");
    for (int i = 0; i < lightc->total_links; i++) {
      printf("%d, ", remote_stats[i]);
    }
    printf("\n");
  }

  int64_t pkt_id = ap->fmt.content.link_monitoring_lightning.id;
  int64_t missing = pkt_id - (lightc->pkt_rcv_id + 1);
  if (pkt_id > lightc->pkt_rcv_id) {
    lightc->pkt_rcv_id = pkt_id;
    memcpy(&lightc->remote_stats, remote_stats, sizeof(int16_t) * lightc->total_links);
  }
  //printf("internal packet %ld (%ld)\n", pkt_id, missing);

  struct timespec now;
  set_now(&now);

  // Detect OoO
  for (int i = 0; i < missing && i < lightc->sent_past_links; i++) {
    uint8_t link_id = prev_links[i];
    int64_t miss_id = pkt_id - (i+1);
    struct timing_entry *te = &lightc->historic[miss_id % HISTORIC_SIZE];
    if (te->pkt_id >= miss_id) continue; // Entry already exists
    te->state = OOO_ONGOING;
    te->detected_at = now;
    te->link_id = link_id;
    te->pkt_id = miss_id;
    if (lightc->explain) printf("(monitoring.delay) packet=%ld, link=%d, state=%s\n", miss_id, link_id, ooo_state_str[te->state]);
  }

  // Update current packet status
  int link_id = url_get_port_int(fdinfo->url) - lightc->base_port;
  struct timing_entry *te2 = &lightc->historic[pkt_id % HISTORIC_SIZE];
  te2->state = te2->pkt_id == pkt_id ? OOO_DONE : IN_ORDER;
  te2->pkt_id = pkt_id;
  te2->link_id = link_id;
  te2->finished_at = now;
  if (lightc->explain) printf("(monitoring.rcv) packet=%ld, link=%d, state=%s\n", pkt_id, link_id, ooo_state_str[te2->state]);
}

int deliver(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  char url[256];
  struct evt_core_fdinfo *to_fdinfo = NULL;
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;

  if (ctx->verbose > 1) fprintf(stderr, "    [algo_lightning] 1/2 Find destination\n");
  sprintf(url, "udp:write:127.0.0.1:%d", ap->fmt.content.udp_encapsulated.port);
  to_fdinfo = evt_core_get_from_url (ctx, url);
  if (to_fdinfo == NULL) {
    fprintf(stderr, "No fd for URL %s in tcp-read. Dropping packet :( \n", url);
    mv_buffer_wtof (&app_ctx->br, fdinfo);
    return 1;
  }

  if (ctx->verbose > 1) fprintf(stderr, "    [algo_lightning] 2/2 Move buffer\n");
  mv_buffer_rtow (&app_ctx->br, fdinfo, to_fdinfo);
  main_on_udp_write(ctx, to_fdinfo);

  return 0;
}

int algo_lightning_on_stream(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  monitoring(ctx, fdinfo, bp);
  return deliver(ctx, fdinfo, bp);
}

int compare_stat_entry_max(const void *a, const void *b) {
  const struct stat_entry *sea = a, *seb = b;
  int ra = sea->ooo;
  int rb = seb->ooo;
  if (ra < 0) ra = INT16_MAX + -ra;
  if (rb < 0) rb = INT16_MAX + -rb;
  return ra - rb;
}

void algo_lightning_update_stats (struct light_ctx *lightc, struct evt_core_ctx* ctx) {
  struct timespec now, not_before = {0}, temp_time;
  set_now(&now);
  timespec_diff (&now, &lightc->window, &not_before);

  // Init
  for (int i = 0; i < lightc->total_links; i++) {
    lightc->stats[i].link_id = i;
    lightc->stats[i].meas_occ = 0;
    lightc->stats[i].ooo = -1;
  }

  // Compute local stats
  for (int i = 0; i < HISTORIC_SIZE; i++) {
    if (timespec_lt(&lightc->historic[i].finished_at, &not_before) && lightc->historic[i].state != OOO_ONGOING) continue;
    uint8_t l = lightc->historic[i].link_id;
    if (l >= lightc->total_links) continue;
    int64_t delta = 0;
    switch (lightc->historic[i].state) {
    case IN_ORDER:
      delta = 0;
      break;
    case OOO_ONGOING:
      timespec_diff(&now, &lightc->historic[i].detected_at, &temp_time);
      delta = timespec_get_unit (&temp_time, MILISEC);
      break;
    case OOO_DONE:
      timespec_diff(&lightc->historic[i].finished_at, &lightc->historic[i].detected_at, &temp_time);
      delta = timespec_get_unit (&temp_time, MILISEC);
      break;
    }
    lightc->stats[l].ooo += delta;
    lightc->stats[l].meas_occ += 1;

    if (lightc->explain) printf("(stats.compute) packet=%ld, link=%d, status=%s, delta=%ld\n", lightc->historic[i].pkt_id, l, ooo_state_str[lightc->historic[i].state], delta);
    lightc->stats[l].link_id = l;

    if (lightc->explain) printf("(stats.local) link=%d, delta=%ld\n", l, delta);
    /*if (delta > stats[l].ooo) {
      if (lightc->explain) printf("(stats.local) link=%d, delta=%ld\n", l, delta);
      stats[l].ooo = delta;
    }*/
  }

  // Compute average
  for (int i = 0; i < lightc->total_links; i++) {
    if (lightc->stats[i].meas_occ <= 0) lightc->stats[i].ooo = -1;
    else lightc->stats[i].ooo = lightc->stats[i].ooo / lightc->stats[i].meas_occ;

    lightc->local_stats[i] = lightc->stats[i].ooo;
  }

  // Set my local stats + merge remote stats
  for (int i = 0; i < lightc->total_links; i++) {

    /* AVG */
    if (lightc->remote_stats[i] < 0) continue;
    if (lightc->stats[i].ooo < 0) lightc->stats[i].ooo = lightc->remote_stats[i];
    else lightc->stats[i].ooo = (lightc->remote_stats[i] + lightc->stats[i].ooo) / 2;

    /* MAX
    if (lightc->remote_stats[i] > lightc->stats[i].ooo) {
      if (lightc->explain) printf("(stats.remote) link=%d, delta=%d\n", i, lightc->remote_stats[i]);
      lightc->stats[i].ooo = lightc->remote_stats[i];
    } */
  }

  // Disable broken links
  char url[256];
  for (int i = 0; i < lightc->total_links; i++) {
    sprintf(url, "tcp:write:127.0.0.1:%d", lightc->base_port + i);
    struct evt_core_fdinfo *to_fdinfo = evt_core_get_from_url (ctx, url);
    if (to_fdinfo == NULL) lightc->stats[i].ooo = -2;
  }

  // Sort
  if (!lightc->disable_scheduler) {
    qsort(lightc->stats, lightc->total_links, sizeof(struct stat_entry), compare_stat_entry_max);
  }
}

int send_message(struct evt_core_ctx* ctx, struct buffer_packet* bp) {
  char url[256];
  struct evt_core_cat *cat = evt_core_get_from_cat (ctx, "tcp-write");
  if (cat == NULL) {
    fprintf(stderr, "[algo_lightning] cat tcp-write not found\n");
    exit(EXIT_FAILURE);
  }

  struct algo_ctx* app_ctx = cat->app_ctx;
  struct light_ctx* lightc = app_ctx->misc;

  if (lightc->selected_link >= lightc->total_links) {
    fprintf(stderr, "[algo_lightning] PACKET DROPPED! Selected link id %d is greater than the total number of links %d\n", lightc->selected_link, lightc->total_links);
    return 0;
  }
  set_now(&lightc->status[lightc->selected_link].last);

  sprintf(url, "tcp:write:127.0.0.1:%d", lightc->base_port + lightc->selected_link);
  struct evt_core_fdinfo *to_fdinfo = evt_core_get_from_url (ctx, url);
  if (to_fdinfo == NULL) {
    fprintf(stderr, "[algo_lightning] PACKET DROPPED! We don't have any entry for %s currently\n", url);
    return 0;
  }

  struct buffer_packet* bp_dup = dup_buffer_tow (&app_ctx->br, bp, to_fdinfo);

  union abstract_packet monit = {
    .fmt.headers.cmd = CMD_LINK_MONITORING_LIGHTNING,
    .fmt.headers.size = lightc->monit_pkt_size,
    .fmt.headers.flags = 0,
    .fmt.content.link_monitoring_lightning.id = lightc->pkt_sent_id
  };
  union abstract_packet *ap_buf = buffer_append_ap (bp_dup, &monit);
  uint8_t *links = &ap_buf->fmt.content.link_monitoring_lightning.dyn_struct;
  int16_t *remote_stats = (int16_t*)(links + sizeof(uint8_t) * lightc->sent_past_links);

  for (int i = 0; i < lightc->sent_past_links; i++) {
    links[i] = lightc->prev_links[(lightc->pkt_sent_id - (i + 1)) % MAX_LINKS];
  }
  memcpy(remote_stats, &lightc->local_stats, sizeof(int16_t) * lightc->total_links);

  if (lightc->explain) {
    printf("(send.stats) ");
    for (int i = 0; i < lightc->total_links; i++) {
      printf("%d, ", remote_stats[i]);
    }
    printf("\n");
  }

  lightc->prev_links[lightc->pkt_sent_id % MAX_LINKS] = lightc->selected_link;
  lightc->pkt_sent_id++;

  if (ctx->verbose > 1) {
    dump_buffer_packet(bp_dup);
    fprintf(stderr, "    [algo_lightning] Will send this info\n");
  }
  main_on_tcp_write(ctx, to_fdinfo);

  return 1;
}

void tag_packet_measlat(union abstract_packet* ap, uint8_t link_id, uint8_t is_slow) {
  union abstract_packet* cur = ap;
  uint8_t vanilla = 1;

  while (cur != NULL) {
    if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) {
      cur = ap_next(cur);
      continue;
    }
    struct measure_packet *mp = (void*)&cur->fmt.content.udp_encapsulated.payload;
    mp->flag = 0x3f & link_id;
    mp->flag |= vanilla << 6;
    mp->flag |= is_slow << 7;

    vanilla = 0;
    cur = ap_next(cur);
  }
}

void algo_lightning_update_used(struct light_ctx *lightc, struct timespec *now) {
  struct timespec not_before = {0}, oldest = *now;
  timespec_diff(now, &lightc->window, &not_before);
  if (timespec_gt(&lightc->last_update_used, &not_before)) return;

  int used_to_not = -1, not_to_used = -1;
  int64_t max_ooo = 0;
  for (int i = 0; i < lightc->total_links; i++) {
    if (lightc->status[lightc->stats[i].link_id].used == LINK_FAST || lightc->status[lightc->stats[i].link_id].used == LINK_SLOW) {
      int64_t retained_ooo = lightc->stats[i].ooo < 0 ? INT64_MAX : lightc->stats[i].ooo;
      if (retained_ooo >= max_ooo) {
        max_ooo = retained_ooo;
        used_to_not = lightc->stats[i].link_id;
      }
    } else {
      if (lightc->stats[i].ooo == -2) continue;
      if (timespec_lt(&lightc->status[lightc->stats[i].link_id].last, &oldest)) {
        oldest = lightc->status[lightc->stats[i].link_id].last;
        not_to_used = lightc->stats[i].link_id;
      }
    }
  }

  // Swap them
  //printf("Link %d will be disabled, %d will be enabled\n", used_to_not, not_to_used);
  if (used_to_not < 0 || not_to_used < 0) return;
  lightc->status[used_to_not].used = LINK_NOT_USED;
  lightc->status[not_to_used].used = LINK_SLOW;
  lightc->last_update_used = *now;
}

void algo_lightning_link_cat(struct light_ctx *lightc, int cur_fast_count) {
  uint8_t used = 0;
  //printf("---\n");
  for (int i = 0; i < lightc->total_links; i++) {
    if (lightc->status[lightc->stats[i].link_id].used != LINK_NOT_USED) {
      if (used < cur_fast_count) lightc->status[lightc->stats[i].link_id].used = LINK_FAST;
      else lightc->status[lightc->stats[i].link_id].used = LINK_SLOW;
      used++;
    }
    //printf("Link ID=%d, status=%s, ooo=%ld\n", lightc->stats[i].link_id, link_cat_str[lightc->status[lightc->stats[i].link_id].used], lightc->stats[i].ooo);
  }
}

int algo_lightning_on_datagram(struct evt_core_ctx* ctx, struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  struct light_ctx* lightc = app_ctx->misc;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;
  struct timespec now, sel_link_last;
  set_now(&now);

  // Pad packet
  algo_lightning_pad (ctx, fdinfo, bp);

  // Prepare links
  algo_lightning_update_stats(lightc, ctx);

  // Adapt tags quantity to active links
  struct evt_core_cat *cat = evt_core_get_from_cat (ctx, "tcp-write");
  int target_to_use = lightc->fast_count*2 < cat->socklist->len ? lightc->fast_count*2 : cat->socklist->len;
  int diff = target_to_use - ((int) lightc->active);
  for (int i = 0; i < lightc->total_links && diff > 0; i++) {
    if (lightc->status[lightc->stats[i].link_id].used != LINK_NOT_USED) continue;
    lightc->status[lightc->stats[i].link_id].used = LINK_SLOW;
    diff--;
  }
  for (int i = lightc->total_links-1; i >= 0 && diff < 0; i--) {
    if (lightc->status[lightc->stats[i].link_id].used == LINK_NOT_USED) continue;
    lightc->status[lightc->stats[i].link_id].used = LINK_NOT_USED;
    diff++;
  }

  lightc->active = target_to_use;

  // Update link tags
  algo_lightning_update_used(lightc, &now);
  algo_lightning_link_cat(lightc, target_to_use/2);

  if (ctx->verbose > 1) {
    printf("link ranking (%d fast links, %d total links)\nposition | port |   score   |   class  \n", target_to_use/2, target_to_use);
    for (int i = 0; i < lightc->total_links; i++) {
      printf("%8d | %4d | %9ld | %9s \n", i, lightc->stats[i].link_id + lightc->base_port, lightc->stats[i].ooo, link_cat_str[lightc->status[lightc->stats[i].link_id].used]);
    }
    printf("\n");
  }

  uint64_t now_timestamp = timespec_get_unit(&now, MILISEC);

  // Select fast link
  if (lightc->sched_strat == SCHEDULE_BOTH || lightc->sched_strat == SCHEDULE_FAST) {
    sel_link_last = now;
    lightc->selected_link = UINT8_MAX;
    for (int i = 0; i < lightc->total_links; i++) {
      if (lightc->status[i].used != LINK_FAST) continue;
      if (timespec_lt (&lightc->status[i].last, &sel_link_last)) {
        lightc->selected_link = i;
        sel_link_last = lightc->status[i].last;
      }
    }
    if (lightc->is_measlat) tag_packet_measlat (ap, lightc->selected_link, 0);
    send_message (ctx, bp);
    if (lightc->csv) printf("%ld,%d,fast\n", now_timestamp, lightc->selected_link);
  }

  // Select slow link
  if (lightc->sched_strat == SCHEDULE_BOTH || lightc->sched_strat == SCHEDULE_SLOW) {
    sel_link_last = now;
    lightc->selected_link = UINT8_MAX;
    for (int i = 0; i < lightc->total_links; i++) {
      if (lightc->status[i].used != LINK_SLOW) continue;
      if (timespec_lt (&lightc->status[i].last, &sel_link_last)) {
        lightc->selected_link = i;
        sel_link_last = lightc->status[i].last;
      }
    }
    if (lightc->is_measlat) tag_packet_measlat (ap, lightc->selected_link, 1);
    send_message (ctx, bp);
    if (lightc->csv) printf("%ld,%d,slow\n", now_timestamp, lightc->selected_link);
  }

  // Update our algo context
  lightc->sched_strat = schedule_group_target_trans[lightc->sched_strat];
  lightc->uniq_pkt_sent_id++;
  mv_buffer_rtof (&app_ctx->br, fdinfo);
  return 0;
}

int algo_lightning_on_err(struct evt_core_ctx *ctx, struct evt_core_fdinfo *fdinfo) {
  struct algo_ctx* app_ctx = fdinfo->cat->app_ctx;
  if (strcmp("tcp-read", fdinfo->cat->name) == 0 || strcmp("tcp-write", fdinfo->cat->name) == 0)
    return app_ctx->ap.sr(ctx, fdinfo);

  fprintf(stderr, "%s is not eligible for a reconnect\n", fdinfo->url);
 // We do nothing
  return 1;
}
