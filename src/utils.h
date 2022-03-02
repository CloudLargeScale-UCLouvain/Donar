// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// 1500 = internet MTU
#define RING_BUFFER_SIZE 1500*10
#define ONE_SEC 1000000000L

enum time_units {
  NANOSEC  =          1L,
  MICROSEC =       1000L,
  MILISEC  =    1000000L,
  SEC      = 1000000000L
};

struct ring_buffer {
  char buffer[RING_BUFFER_SIZE];
  int head;
  int tail;
};

int ring_buffer_read(struct ring_buffer* rb, char* dest, int size);
void ring_buffer_ack_read(struct ring_buffer* rb, int size);
int ring_buffer_write(struct ring_buffer* rb, char* source, int size);
int ring_buffer_free_space(struct ring_buffer* rb);
int ring_buffer_used_space(struct ring_buffer* rb);

int ring_gt(uint16_t v1, uint16_t v2);
int ring_ge(uint16_t v1, uint16_t v2);
int ring_lt(uint16_t v1, uint16_t v2);
int ring_le(uint16_t v1, uint16_t v2);

uint64_t elapsed_micros(struct timespec* t1, struct timespec* t2);
void set_now(struct timespec *dest);

uint8_t timespec_eq(struct timespec *t1, struct timespec *t2);
uint8_t timespec_gt(struct timespec *t1, struct timespec *t2);
uint8_t timespec_ge(struct timespec *t1, struct timespec *t2);
uint8_t timespec_le(struct timespec *t1, struct timespec *t2);
uint8_t timespec_lt(struct timespec *t1, struct timespec *t2);
void timespec_diff(struct timespec *end, struct timespec *begin, struct timespec *result);
void timespec_set_unit(struct timespec *t, uint64_t value, enum time_units unit);
uint64_t timespec_get_unit(struct timespec *t, enum time_units unit);

char* current_human_datetime();
