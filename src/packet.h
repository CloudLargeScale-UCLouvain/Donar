// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "evt_core.h"
#include "url.h"

#define UDP_MTU 65535

/*
 * man 7 udp about receive operation on UDP sockets:
 *
 * > All  receive  operations  return only one packet.  When the packet is smaller than the passed
 * > buffer, only that much data is returned; when it is bigger, the packet is truncated  and  the
 * > MSG_TRUNC flag is set.  MSG_WAITALL is not supported.
 */

enum FD_STATE {
  FDS_READY,
  FDS_AGAIN,
  FDS_ERR
};

enum BP_MODE {
  BP_READING,
  BP_WRITING
};

enum PKT_CMD {
  CMD_UDP_ENCAPSULATED = 1,
  CMD_LINK_MONITORING_THUNDER = 2,
  CMD_UDP_METADATA_THUNDER = 3,
  CMD_LINK_MONITORING_LIGHTNING = 4,
};

enum PKT_FLAGS {
  FLAG_READ_NEXT = 1 << 0,
  FLAG_RESET = 1 << 1,
};

struct link_info {
  uint16_t delta_t;
};

#pragma pack(1)
union abstract_packet {
  char raw;
  struct {
    struct {
      uint16_t size;
      uint8_t cmd;
      uint8_t flags;
    } headers;

    union {
      struct {
        uint64_t id;
        uint8_t dyn_struct;
      } link_monitoring_lightning;
      struct {
        uint8_t to_increment;
        struct link_info links_status;
      } link_monitoring_thunder;
      struct {
        uint16_t id;
      } udp_metadata_thunder;
      struct {
        uint16_t port;
        char payload;
      } udp_encapsulated;
    } content;
  } fmt;
};
#pragma pack()

struct buffer_packet {
  enum BP_MODE mode;
  uint8_t ap_count;
  uint16_t aread;
  uint16_t awrite;
  struct timespec seen;
  char ip[UDP_MTU];
};

struct udp_target {
  struct sockaddr_in addr;
  socklen_t addrlen;
  int set;
  int ref_count;
};

size_t get_full_size(struct buffer_packet* bp);

union abstract_packet* buffer_append_ap(struct buffer_packet* bp, union abstract_packet* ap);
union abstract_packet* buffer_free_ap(struct buffer_packet* bp);
union abstract_packet* buffer_first_ap(struct buffer_packet* bp);
union abstract_packet* buffer_last_ap(struct buffer_packet* bp);
size_t buffer_full_size(struct buffer_packet* bp);
union abstract_packet* ap_next(union abstract_packet* ap);

enum FD_STATE read_packet_from_tcp(struct evt_core_fdinfo* fd, struct buffer_packet* bp);
enum FD_STATE write_packet_to_tcp(struct evt_core_fdinfo* fd, struct buffer_packet* bp);
enum FD_STATE write_packet_to_udp(struct evt_core_fdinfo* fd, struct buffer_packet* bp, struct udp_target* udp_t);
enum FD_STATE read_packet_from_udp (struct evt_core_fdinfo* fd, struct buffer_packet* bp, struct udp_target* udp_t);

void dump_buffer_packet(struct buffer_packet* bp);
void dump_abstract_packet(union abstract_packet* ap);
