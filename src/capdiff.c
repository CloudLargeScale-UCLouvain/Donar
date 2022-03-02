// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <stdlib.h>
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>
#include <glib-2.0/glib-object.h>
#include "cap_utils.h"
#include "packet.h"

#define MAX_PKTS_TO_CHECK_FOR_DROP 10

uint8_t are_packets_equal(struct buffer_packet bpread[]) {
  union abstract_packet *ap1 = (union abstract_packet*)&bpread[0].ip, *ap2 = (union abstract_packet*) bpread[1].ip;
  size_t s1 = ap1->fmt.headers.size, s2 = ap2->fmt.headers.size;
  if (s1 != s2) return 0;

  for (size_t idx = &ap1->fmt.content.udp_encapsulated.payload - (char*)ap1; idx < s1; idx++) {
    char e1 = (&ap1->raw)[idx], e2 = (&ap2->raw)[idx];
    if (e1 != e2) return 0;
  }

  return 1;
}

enum pkt_reconstruct_res { PREC_SAME, PREC_DROP, PREC_FAIL };
struct pkt_reconstruct {
  enum pkt_reconstruct_res r;
  int diff[2];
  struct buffer_packet single[2][MAX_PKTS_TO_CHECK_FOR_DROP];
};

struct pkt_stats {
  gint port;
  uint64_t count;
  double cumulated_size;
  struct timespec first;
  struct timespec last;
};

void destroy_pkt_stats(gpointer data) {
  struct pkt_stats* ps = data;
  free(ps);
}

void update_stats(struct buffer_packet *bp, GHashTable* stat_elem) {
  union abstract_packet *ap = (union abstract_packet*)&bp->ip;
  if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) return;

  gint port = ap->fmt.content.udp_encapsulated.port;
  struct pkt_stats *ps = g_hash_table_lookup(stat_elem, &port);
  if (ps == NULL) {
    ps = malloc(sizeof(struct pkt_stats));
    if (ps == NULL) {
      perror("Unable to alloc pkt_stats");
      exit(EXIT_FAILURE);
    }
    ps->port = port;
    ps->count = 0;
    ps->cumulated_size = 0;
    ps->first = bp->seen;
    g_hash_table_insert (stat_elem, &ps->port, ps);
  }
  ps->last = bp->seen;
  ps->count++;
  ps->cumulated_size += ap->fmt.headers.size;
}

void unroll_packets(struct cap_file cf[], struct buffer_packet bpread[], GHashTable* stats[], struct pkt_reconstruct *pr, int m, int i) {
  cap_next_bp (&cf[i], &bpread[i]);
  update_stats(&bpread[i], stats[i]);
  memcpy(&pr->single[i][m], &bpread[i], sizeof(struct buffer_packet));
}

void reconstruct_action(struct cap_file cf[], struct pkt_reconstruct* pr, GHashTable *stats[]) {
  struct buffer_packet bpread[2];
  pr->r = PREC_FAIL;
  for (int m1 = 0; m1 < MAX_PKTS_TO_CHECK_FOR_DROP; m1++) {
    for (int m2 = 0; m2 <= m1; m2++) {
      cap_npeek_bp(&cf[0], m1, &bpread[0]);
      cap_npeek_bp(&cf[1], m2, &bpread[1]);
      if(are_packets_equal(bpread)) {
        pr->r = m1 == 0 && m2 == 0 ? PREC_SAME : PREC_DROP;
        pr->diff[0] = m1;
        pr->diff[1] = m2;
        for (int m = m1; m >= 0; m--) unroll_packets (cf, bpread, stats, pr, m, 0);
        for (int m = m2; m >= 0; m--) unroll_packets (cf, bpread, stats, pr, m, 1);
        return;
      }

      cap_npeek_bp(&cf[0], m2, &bpread[0]);
      cap_npeek_bp(&cf[1], m1, &bpread[1]);
      if(are_packets_equal(bpread)) {
        pr->r = PREC_DROP;
        pr->diff[0] = m2;
        pr->diff[1] = m1;
        for (int m = m2; m >= 0; m--) unroll_packets (cf, bpread, stats, pr, m, 0);
        for (int m = m1; m >= 0; m--) unroll_packets (cf, bpread, stats, pr, m, 1);
        return;
      }
    }
  }
};

double timespec2double (struct timespec* ts) {
  return (long long) ts->tv_sec + (double) ts->tv_nsec / (double)1e9;
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("~ capdiff ~\n");

  if (argc != 3) {
    fprintf(stderr, "Usage %s FILE.IN FILE.OUT\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  uint8_t verbose = 0;

  struct cap_file cf[2];
  for (int i = 0; i < 2; i++) cap_load(&cf[i], argv[i+1]);

  if (cf[0].sz != cf[1].sz) {
    printf("[!!] %s has %ld entries, %s has %ld entries\n",
           argv[1],
           cf[0].sz/sizeof(struct buffer_packet),
           argv[2],
           cf[1].sz/sizeof(struct buffer_packet));
  } else if (verbose) {
    printf("[OK] %s and %s have %ld entries\n",
           argv[1],
           argv[2],
           cf[0].sz/sizeof(struct buffer_packet));
  }


  GHashTable *stats[2];
  stats[0] = g_hash_table_new_full (g_int_hash, g_int_equal, NULL, destroy_pkt_stats);
  stats[1] = g_hash_table_new_full (g_int_hash, g_int_equal, NULL, destroy_pkt_stats);

  struct pkt_reconstruct pr;
  struct buffer_packet bpread[2];
  int c0 = 0, c1 = 0, stop = 0;
  while (c0 < cf[0].sz_obj && c1 < cf[1].sz_obj && !stop) {
    reconstruct_action(cf, &pr, stats);
    c0 += pr.diff[0] + 1;
    c1 += pr.diff[1] + 1;
    switch (pr.r) {
    case PREC_SAME:
      // Nothing
      break;
    case PREC_DROP:
      fprintf(stderr, "[!!] packets (%d,%d), ", c0, c1);
      if (pr.diff[1] > 0) fprintf(stderr, "%s dropped %d pkts, ", cf[0].filename, pr.diff[1]);
      if (pr.diff[0] > 0) fprintf(stderr, "%s dropped %d pkts ", cf[1].filename, pr.diff[0]);
      fprintf(stderr, "\n");

      gboolean both_different = pr.diff[1] > 0 && pr.diff[0] > 0;
      for (int j=0; j < 2; j++) {
        gboolean received_created_packets = strstr(cf[j].filename, "received") != NULL && pr.diff[j] > 0;
        if (!both_different && !received_created_packets) continue;
        fprintf(stderr, "---- %s contains the following packets that are not in %s ----\n", cf[j].filename, cf[(j+1)%2].filename);
        for (int i = pr.diff[j]; i >= 0; i--) {
          dump_buffer_packet (&pr.single[j][i]);
        }
      }

      break;
    case PREC_FAIL:
      fprintf(stderr, "[!!] Unable to remap packets (%d, %d). We should stop\n", c0, c1);
      stop = 1;
      break;
    }
  }

  printf("parsed (%d,%d) packets\n", c0, c1);

  GHashTableIter iter;
  gpointer key, value;

  struct pkt_stats ps_default = {
    .count = 0,
    .cumulated_size = 0
  };
  g_hash_table_iter_init (&iter, stats[0]);
  while (g_hash_table_iter_next (&iter, &key, &value))
  {
    int *port = key;
    struct pkt_stats *ps = value;
    struct pkt_stats *ps2 = g_hash_table_lookup (stats[1], port);
    if (ps2 == NULL) {
      fprintf(stderr, "[!!] No packet received for this port\n");
      ps2 = &ps_default;
    }

    double drop = ps->count > ps2->count ? (double)(ps->count - ps2->count) / (double)ps->count : (double)(ps2->count - ps->count) / (double)ps2->count;
    double delta_time = timespec2double (&ps->last) - timespec2double (&ps->first);

    fprintf(
      stdout,
      "port=%d, avg_size=%lf, count=%ld, drop=%lf, duration=%lf, bandwidth=%lf kB/s, rate=%lf pkt/s\n",
      *port,
      ps->cumulated_size / ps->count,
      ps->count,
      drop,
      delta_time,
      ps->cumulated_size / delta_time / 1024,
      ps->count / delta_time);
  }

  for (int i = 0; i < 2; i++) cap_unload (&cf[i]);
  return 0;
}
