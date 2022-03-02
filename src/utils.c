// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "utils.h"

int ring_buffer_read(struct ring_buffer* rb, char* dest, int size) {
  int slice1 = size;
  int slice2 = 0;

  int used_space = ring_buffer_used_space (rb);
  if (used_space < slice1)
    slice1 = used_space;

  if (RING_BUFFER_SIZE - rb->head < slice1) {
    slice1 = RING_BUFFER_SIZE - rb->head;
    slice2 = size - slice1;
    if (used_space - slice1 < slice2)
      slice2 = used_space - slice1;
  }

  printf("max_buffer=%d, head=%d, tail=%d, size=%d, slice1=%d, slice2=%d\n", RING_BUFFER_SIZE, rb->head, rb->tail, size, slice1, slice2);
  memcpy(dest, rb->buffer + rb->head, slice1);
  memcpy(dest+slice1, rb->buffer, slice2);

  return slice1 + slice2;
}

void ring_buffer_ack_read(struct ring_buffer* rb, int size) {
  if (size > ring_buffer_used_space (rb)) {
    fprintf(stderr, "You try to ack more data than contained in the ring buffer\n");
    exit(EXIT_FAILURE);
  }
  rb->head = (rb->head + size) % RING_BUFFER_SIZE;
}

int ring_buffer_write(struct ring_buffer* rb, char* source, int size) {
  if (size > ring_buffer_free_space (rb)) {
    fprintf(stderr, "You try to write more data than available space in the buffer\n");
    exit(EXIT_FAILURE);
  }

  int slice1 = size;
  int slice2 = 0;

  if (RING_BUFFER_SIZE - rb->tail < slice1) {
    slice1 = RING_BUFFER_SIZE - rb->tail;
    slice2 = size - slice1;
  }

  memcpy(rb->buffer + rb->tail, source, slice1);
  memcpy(rb->buffer, source + slice1, slice2);

  rb->tail = (rb->tail + slice1 + slice2) % RING_BUFFER_SIZE;

  return slice1 + slice2;
}

int ring_buffer_free_space(struct ring_buffer* rb) {
  if (rb->head > rb->tail) return rb->head - rb->tail;
  return RING_BUFFER_SIZE - (rb->tail - rb->head);
}

int ring_buffer_used_space(struct ring_buffer* rb) {
  return RING_BUFFER_SIZE - ring_buffer_free_space (rb);
}

// Why we are using modulo, plus and modulo again:
// https://stackoverflow.com/a/1907585
int ring_ge(uint16_t v1, uint16_t v2) {
  int64_t vv1 = (int64_t) v1, vv2 = (int64_t) v2;
  return (((vv1 - vv2) % UINT16_MAX) + UINT16_MAX) % UINT16_MAX <= UINT16_MAX / 2;
}

int ring_gt(uint16_t v1, uint16_t v2) {
  if (v1 == v2) return 0;
  return ring_ge(v1,v2);
}

int ring_le(uint16_t v1, uint16_t v2) {
  return ring_ge(v2, v1);
}

int ring_lt(uint16_t v1, uint16_t v2) {
  return ring_gt(v2, v1);
}

uint64_t elapsed_micros(struct timespec* t1, struct timespec* t2) {
  int secs, nsecs;
  uint64_t micro_sec;

  secs = t2->tv_sec - t1->tv_sec;
  nsecs = t2->tv_nsec - t1->tv_nsec;
  micro_sec = secs * 1000000 + nsecs / 1000;

  return micro_sec;
}

void set_now(struct timespec *dest) {
  if (clock_gettime(CLOCK_MONOTONIC, dest) == -1){
    perror("clock_gettime error");
    exit(EXIT_FAILURE);
  }
}

uint8_t timespec_eq(struct timespec *t1, struct timespec *t2) {
  return t1->tv_sec == t2->tv_sec && t1->tv_nsec == t2->tv_nsec;
}

uint8_t timespec_gt(struct timespec *t1, struct timespec *t2) {
  return t1->tv_sec > t2->tv_sec ||
    (t1->tv_sec == t2->tv_sec && t1->tv_nsec > t2->tv_nsec);
}

uint8_t timespec_ge(struct timespec *t1, struct timespec *t2) {
  return timespec_gt (t1, t2) || timespec_eq(t1, t2);
}

uint8_t timespec_le (struct timespec *t1, struct timespec *t2) {
  return !timespec_gt (t1, t2);
}

uint8_t timespec_lt(struct timespec *t1, struct timespec *t2) {
  return !timespec_ge(t1, t2);
}

void timespec_diff(struct timespec *stop, struct timespec *start, struct timespec *result) {
  if (timespec_gt(start, stop)) {
    fprintf(stderr, "start is greater than stop in timespec_diff, this is an error\n");
    exit(EXIT_FAILURE);
  }

  if (stop->tv_nsec < start->tv_nsec) {
    result->tv_sec = stop->tv_sec - start->tv_sec - 1;
    result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
  } else {
    result->tv_sec = stop->tv_sec - start->tv_sec;
    result->tv_nsec = stop->tv_nsec - start->tv_nsec;
  }

  return;
}

void timespec_set_unit(struct timespec *t, uint64_t value, enum time_units unit) {
  uint64_t sec = value / (SEC / unit);
  uint64_t ns = value * unit;
  t->tv_sec = sec;
  t->tv_nsec = ns;
}

uint64_t timespec_get_unit(struct timespec *t, enum time_units unit) {
  return t->tv_sec * (SEC / unit) + t->tv_nsec / unit;
}

char* current_human_datetime() {
  time_t now;
  time(&now);
  char* ctime_no_newline = strtok(ctime(&now), "\n");
  return ctime_no_newline;
}
