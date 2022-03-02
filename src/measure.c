// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "measure.h"

char* default_tag = "undefined";

void measure_params_init(struct measure_params* mp) {
  mp->interval = 1000;
  mp->max_measure = 3;
  mp->tag = default_tag;
  mp->payload_size = sizeof(struct measure_packet);
}

void measure_params_setpl (struct measure_params* mp, size_t plsize) {
  if (plsize > sizeof(struct measure_packet)) {
    mp->payload_size = plsize;
  }
}

void measure_state_init(struct measure_params* mp, struct measure_state* ms) {
  ms->mp_nin = 0;
  ms->fd = 0;
  uuid_generate (ms->uuid);

  if (ms->log == NULL) {
    if ((ms->log = malloc(sizeof(uint64_t) * mp->max_measure)) == NULL) {
      perror("log malloc failed");
      exit(EXIT_FAILURE);
    }
  }
  memset(ms->log, 0, sizeof(uint64_t) * mp->max_measure);

  if (ms->mp_out == NULL) {
    if ((ms->mp_out = malloc(sizeof(char) * mp->payload_size)) == NULL) {
      perror("payload malloc failed");
      exit(EXIT_FAILURE);
    }
  }
  memset(ms->mp_out, 0, mp->payload_size);

  if (ms->mp_in == NULL) {
    if ((ms->mp_in = malloc(sizeof(char) * mp->payload_size)) == NULL) {
      perror("payload malloc failed");
      exit(EXIT_FAILURE);
    }
  }
  memset(ms->mp_in, 0, mp->payload_size);

  ms->mp_out->probe = mp->probe;
  char *my_msg = "Tu n'es pas tout a fait la misere,\nCar les levres les plus pauvres te denoncent\nPar un sourire.";
  size_t msg_len = strlen(my_msg);
  size_t cursor_msg = 0;
  char* pl = (char*) ms->mp_out;
  for (size_t i = sizeof(struct measure_packet); i < mp->payload_size; i++) {
    pl[i] = my_msg[cursor_msg];
    cursor_msg = (cursor_msg + 1) % msg_len;
  }
}

void measure_parse(struct measure_params* mp, struct measure_state* ms, uint8_t verbose) {
  struct timespec curr;
  uint64_t micro_sec;
  if (ms->mp_nin != mp->payload_size) {
    fprintf(stderr, "read size: %ld, expected: %ld\n", ms->mp_nin, mp->payload_size);
    int i;

    fprintf(stderr, "received buffer:\n");
    for (i = 0; i < mp->payload_size; i++) {
      if (i > 0) fprintf(stderr, ":");
      fprintf(stderr, "%02x", (unsigned char) ((char*)ms->mp_in)[i]);
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "local buffer (reference):\n");
    for (i = 0; i < mp->payload_size; i++) {
      if (i > 0) fprintf(stderr, ":");
      fprintf(stderr, "%02X", (unsigned char) ((char*)ms->mp_out)[i]);
    }
    fprintf(stderr, "\n");


    perror("read error, payload has wrong size");
  }
  if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1){
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }

  micro_sec = elapsed_micros (&ms->mp_in->emit_time, &curr);

  if (ms->mp_in->counter <= mp->max_measure && !ms->mp_in->probe) {
    // we log only the delay of the first received packet, the fastest
    if (ms->log[ms->mp_in->counter-1] == 0) ms->log[ms->mp_in->counter-1] = micro_sec;
  } else {
    verbose = 1;
    fprintf(stderr, "measure will be ignored: probe=%d, counter=%lu, max_measure=%lu\n", ms->mp_in->probe, ms->mp_in->counter, mp->max_measure);
  }

  if (verbose) {
    uint8_t is_slow = ms->mp_in->flag >> 7;
    uint8_t is_vanilla = (ms->mp_in->flag & 0x40) >> 6;
    uint8_t link_id = ms->mp_in->flag & 0x3f;
    printf(
      "[%s] src=%d, id=%lu, owd=%luµs, flag=%d, link=%d, vanilla=%d\n",
      current_human_datetime(),
      ms->fd,
      ms->mp_in->counter,
      micro_sec,
      is_slow,
      link_id,
      is_vanilla);
  }

  ms->mp_nin = 0;
}

struct measure_packet* measure_generate(struct measure_params* mp, struct measure_state* ms) {
  ms->mp_out->counter++;
  ms->mp_out->flag = 0;
  if (clock_gettime(CLOCK_MONOTONIC, &ms->mp_out->emit_time) == -1) {
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }
  return ms->mp_out;
}

void measure_next_tick(struct measure_params *mp, struct measure_state* ms, struct timespec *next) {
  //struct measure_packet *head = (struct measure_packet*) ms->payload_rcv;
  struct timespec now = {0}, *sent_at = &ms->mp_in->emit_time;

  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }
  memcpy(next, sent_at, sizeof(struct timespec));

  while(!timespec_gt (next, &now)) {
    next->tv_nsec += mp->interval * 1000000L;
    if (next->tv_nsec > ONE_SEC) {
      next->tv_sec += next->tv_nsec / ONE_SEC;
      next->tv_nsec = next->tv_nsec % ONE_SEC;
    }
    ms->mp_out->counter++;
  }
  ms->mp_out->counter--;
  printf("interval: %ld\n", mp->interval);
  printf("sent_at: sec=%ld nsec=%ld \n", (uint64_t) sent_at->tv_sec, (uint64_t) sent_at->tv_nsec);
  printf("now: sec=%ld nsec=%ld \n", (uint64_t) now.tv_sec, (uint64_t) now.tv_nsec);
  printf("next: sec=%ld nsec=%ld \n", (uint64_t) next->tv_sec, (uint64_t) next->tv_nsec);
}

void measure_state_free (struct measure_state* ms) {
  free(ms->mp_in);
  free(ms->mp_out);
  free(ms->log);
  uuid_clear(ms->uuid);
}

int cmpuint64t(const void* u1, const void* u2) {
  const uint64_t *i1 = u1;
  const uint64_t *i2 = u2;
  return *i1 < *i2 ? -1 : 1;
}

void measure_param_print(struct measure_params* mp) {
  fprintf(stderr,
          "measure_params {\n\tmax_measure: %lu\n\tpayload_size: %lu\n\tinterval: %lu\n\tis_server: %u\n\ttag: %s\n}\n",
          mp->max_measure, mp->payload_size, mp->interval, mp->is_server, mp->tag);
}

void measure_summary(struct measure_params* mp, struct measure_state* ms) {
  char uuidstr[37] = {0};
  uuid_unparse (ms->uuid, uuidstr);

  char bin[41] = {0}, txt[41] = {0};
  FILE *fbin, *ftxt;
  size_t res;

  sprintf(bin, "%s.bin", uuidstr);
  if ((fbin = fopen(bin, "a+")) == NULL) goto measurement_io_error;
  size_t tlen = strlen(mp->tag);
  if (fwrite(&tlen, sizeof(size_t), 1, fbin) != 1) goto measurement_io_error;
  if (fwrite(mp->tag, sizeof(char), tlen, fbin) != tlen) goto measurement_io_error;
  if (fwrite(ms->uuid, sizeof(uuid_t), 1, fbin) != 1) goto measurement_io_error;
  if (fwrite(&mp->max_measure, sizeof(uint64_t), 1, fbin) != 1) goto measurement_io_error;
  if (fwrite(ms->log, sizeof(uint64_t), mp->max_measure, fbin) != mp->max_measure) goto measurement_io_error;
  if (fclose(fbin) != 0) goto measurement_io_error;
  printf("saved raw data as %s\n", bin);

  uint64_t* real_log = ms->log;
  uint64_t real_log_size = mp->max_measure;

  // cut beginning
  while (real_log[0] == 0 && real_log_size > 0) {
    real_log = &(real_log[1]);
    real_log_size--;
  }
  printf("[summary] cutted %lu values at beginning\n", mp->max_measure - real_log_size);

  // cut end
  while (real_log[real_log_size-1] == 0 && real_log_size > 0) {
    real_log_size--;
  }
  printf("[summary] cutted %lu values in total\n", mp->max_measure - real_log_size);
  if (real_log_size == 0) return;

  // AVERAGE
  double avg = 0;
  for (int i = 0; i < real_log_size; i++) {
    avg += ((double) real_log[i]) / ((double) real_log_size);
  }

  // DISTRIBUTION
  qsort (real_log, real_log_size, sizeof(uint64_t), cmpuint64t);
  uint64_t min = real_log[0];
  uint64_t max = real_log[real_log_size-1];
  uint64_t med = real_log[(int)(0.50 * real_log_size) - 1];
  uint64_t q25 = real_log[(int)(0.25 * real_log_size) - 1];
  uint64_t q75 = real_log[(int)(0.75 * real_log_size) - 1];
  uint64_t q99 = real_log[(int)(0.99 * real_log_size) - 1];


  sprintf(txt, "%s.txt", uuidstr);
  if ((ftxt = fopen(txt, "a+")) == NULL) goto measurement_io_error;
  if (fprintf(ftxt, "%s,%s,count,%lu\n", mp->tag, uuidstr, real_log_size) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,avg,%f\n",  mp->tag, uuidstr, avg) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,min,%lu\n", mp->tag, uuidstr, min) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,max,%lu\n", mp->tag, uuidstr, max) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,med,%lu\n", mp->tag, uuidstr, med) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,q25,%lu\n", mp->tag, uuidstr, q25) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,q75,%lu\n", mp->tag, uuidstr, q75) < 0) goto measurement_io_error;
  if (fprintf(ftxt,  "%s,%s,q99,%lu\n", mp->tag, uuidstr, q99) < 0) goto measurement_io_error;
  if (fclose(ftxt) != 0) goto measurement_io_error;

  printf("saved aggregated data as %s\n", txt);
  return;

measurement_io_error:
  perror("an io error occured while writing measurement results");
  exit(EXIT_FAILURE);
}
