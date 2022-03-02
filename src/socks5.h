// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include "net_tools.h"
#include "evt_core.h"

enum socks5_state {
  SOCKS5_STATE_NEW,
  SOCKS5_STATE_ACK,
  SOCKS5_STATE_RDY,
  SOCKS5_STATE_ERR
};

enum cmd {
    CMD_CONNECT = 0x01,
    CMD_BIND = 0x02,
    CMD_UDP_ASSOCIATE = 0x03
};

enum atyp {
    ATYP_IPV4 = 0x01,
    ATYP_DOMAINNAME = 0x03,
    ATYP_IPV6 = 0x04
};

static char *atyp_str[] = {
  "INVALID",
  "IPV4",
  "INVALID",
  "DOMAIN_NAME",
  "IPV6"
};

enum ver {
    VER_SOCKS5 = 0x05
};

enum methods {
    METHOD_NOAUTH = 0x00,
    METHOD_GSSAPI = 0x01,
    METHOD_USERPASS = 0x02,
    METHOD_NOACCEPT = 0xff
};

union socks5_addr {
    struct {
        uint8_t len;
        char str[256];
    } dns;
    uint8_t ipv4[4];
    uint8_t ipv6[16];
};

enum socks5_rep {
  SOCKS5_REP_SUCCESS,
  SOCKS5_REP_GENERAL_FAILURE,
  SOCKS5_REP_CONOTALLOWED,
  SOCKS5_REP_NETUNREACH,
  SOCKS5_REP_HOSTUNREACH,
  SOCKS5_REP_COREFUSED,
  SOCKS5_REP_TTLEXP,
  SOCKS5_REP_CMDNOTSUP,
  SOCKS5_REP_ADDRNOTSUP
};

static char* rep_msg[] = {
  "Succeeded",
  "General SOCKS server failure",
  "Connection not allowed by ruleset",
  "Network unreachable",
  "Host unreachable",
  "Connection refused",
  "TTL expired",
  "Command not supported",
  "Address type not supported"
};

/*
 * RFC 1928 Messages
 * https://tools.ietf.org/html/rfc1928
 */

#pragma pack(1)
struct client_handshake {
    uint8_t ver;
    uint8_t nmethods;
    uint8_t methods[255];
};

struct server_handshake {
    uint8_t ver;
    uint8_t method;
};

struct client_request {
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t atyp;
    union socks5_addr dest_addr;
    uint16_t port;
};

struct server_reply {
    uint8_t ver;
    uint8_t rep;
    uint8_t rsv;
    uint8_t atyp;
    union socks5_addr bind_addr;
    uint16_t port;
};
#pragma pack()

struct socks5_ctx {
  uint16_t port;
  char* addr;

  struct client_handshake ch;
  struct server_handshake sh;
  struct client_request cr;
  struct server_reply sr;
  uint64_t ch_cursor;
  uint64_t sh_cursor;
  uint64_t cr_cursor;
  uint64_t sr_cursor;
  char cr_buffer[262];
  char sr_buffer[263];
  size_t ch_size;
  size_t cr_size;
  size_t sr_size;
  uint8_t sr_host_read;
  uint8_t cr_host_read;
};

void socks5_init(struct evt_core_ctx* ctx);
void socks5_create_dns_client(struct evt_core_ctx* ctx, char* proxy_host, char* proxy_port, char* addr, uint16_t port);
char* socks5_rep (enum socks5_rep rep);

void socks5_server_init(struct evt_core_ctx* ctx);
void socks5_server_handle_req(struct evt_core_ctx* ctx, int fd);
