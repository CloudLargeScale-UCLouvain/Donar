// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "packet.h"

int ap_exists(union abstract_packet* ap) {
  return ap->fmt.headers.cmd != 0;
}

int buffer_has_ap(struct buffer_packet* bp) {
  return ap_exists(buffer_first_ap (bp));
}

union abstract_packet* ap_next(union abstract_packet* ap) {
  if (ap_exists (ap) && ap->fmt.headers.flags & FLAG_READ_NEXT)
    return (union abstract_packet*)(&ap->raw + ap->fmt.headers.size);

  return NULL;
}

union abstract_packet* buffer_first_ap(struct buffer_packet* bp) {
   return (union abstract_packet*) &bp->ip;
}

union abstract_packet* buffer_last_ap(struct buffer_packet* bp) {
  union abstract_packet* ap = buffer_first_ap (bp), *apn = NULL;
  while ((apn = ap_next(ap)) != NULL) ap = apn;

  return ap;
}

union abstract_packet* buffer_free_ap(struct buffer_packet* bp) {
  union abstract_packet* ap = buffer_last_ap (bp);
  ap = (union abstract_packet*)(&ap->raw + ap->fmt.headers.size);

  return ap;
}

size_t buffer_count_ap(struct buffer_packet* bp) {
  size_t s = 1;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;
  while ((ap = ap_next(ap)) != NULL) s++;
  return s;
}

size_t buffer_full_size(struct buffer_packet* bp) {
  return &(buffer_free_ap (bp))->raw - &bp->ip[0];
}

union abstract_packet* buffer_append_ap(struct buffer_packet* bp, union abstract_packet* ap) {
  if (buffer_has_ap (bp))
    buffer_last_ap(bp)->fmt.headers.flags |= FLAG_READ_NEXT;

  union abstract_packet *new_ap = buffer_last_ap(bp);
  memcpy(new_ap, ap, ap->fmt.headers.size);
  bp->ap_count++;
  new_ap->fmt.headers.flags &= ~FLAG_READ_NEXT;
  return new_ap;
}

enum FD_STATE read_packet_from_tcp(struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  ssize_t nread = 0, ap_aread = 0, cur_ap_aread = 0;
  union abstract_packet* ap = buffer_first_ap (bp);
  size_t pkt_size_size = sizeof(ap->fmt.headers.size);
  if (bp->mode != BP_READING) return FDS_ERR;

  //fprintf(stderr, "Entering read_packet_from_tcp\n");
  do {

    //fprintf(stderr, "bp->ap_count=%d\n", bp->ap_count);
    ap = buffer_first_ap (bp);
    ap_aread = 0;
    for (int i = 0; i < bp->ap_count; i++) {
      ap_aread += ap->fmt.headers.size;
      ap = ap_next (ap);
    }
    cur_ap_aread = bp->aread - ap_aread;

    //fprintf(stderr, "[size] bp_aread=%d, prev_ap_aread=%ld, cur_ap_aread=%ld\n", bp->aread, ap_aread, cur_ap_aread);
    while (cur_ap_aread < pkt_size_size) {
      nread = read(fdinfo->fd, &(ap->raw) + cur_ap_aread, pkt_size_size - cur_ap_aread);
      if (nread == 0) return FDS_AGAIN;
      if (nread == -1 && errno == EAGAIN) return FDS_AGAIN;
      if (nread == -1) return FDS_ERR;
      bp->aread += nread;
      cur_ap_aread += nread;
    }

    //fprintf(stderr, "[content] bp_aread=%d, prev_ap_aread=%ld, cur_ap_aread=%ld\n", bp->aread, ap_aread, cur_ap_aread);
    while (cur_ap_aread < ap->fmt.headers.size) {
      nread = read(fdinfo->fd, &(ap->raw) + cur_ap_aread, ap->fmt.headers.size - cur_ap_aread);
      if (nread == 0) return FDS_AGAIN;
      if (nread == -1 && errno == EAGAIN) return FDS_AGAIN;
      if (nread == -1) return FDS_ERR;
      bp->aread += nread;
      cur_ap_aread += nread;
    }

    bp->ap_count++;
    //fprintf(stderr, "bp->ap_count=%d, buffer_count_ap(bp)=%ld\n", bp->ap_count, buffer_count_ap (bp));
    //dump_buffer_packet (bp);
  } while (bp->ap_count != buffer_count_ap (bp));

  bp->mode = BP_WRITING;
  bp->awrite = 0;

  return FDS_READY;
}

enum FD_STATE write_packet_to_tcp(struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp) {
  ssize_t nwrite;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;
  size_t buffs = buffer_full_size(bp);

  //dump_buffer_packet (bp);
  if (bp->mode != BP_WRITING) return FDS_ERR;
  nwrite = send(fdinfo->fd, &(ap->raw) + bp->awrite, buffs - bp->awrite, 0);
  if (nwrite == -1 && errno == EAGAIN) return FDS_AGAIN;
  if (nwrite == -1) return FDS_ERR;
  bp->awrite += nwrite;
  if (bp->awrite < buffs) return FDS_AGAIN;

  bp->mode = BP_READING;
  bp->aread = 0;
  bp->ap_count = 0;

  return FDS_READY;
}

enum FD_STATE write_packet_to_udp(struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp, struct udp_target* udp_t) {
  ssize_t nwrite;
  union abstract_packet* ap = (union abstract_packet*) (&bp->ip + bp->awrite);
  if (bp->mode != BP_WRITING) return FDS_ERR;

  do {
    if (ap->fmt.headers.cmd != CMD_UDP_ENCAPSULATED) continue;

    size_t bytes_to_send;
    size_t pkt_header_size = sizeof(ap->fmt.headers) + sizeof(ap->fmt.content.udp_encapsulated) - sizeof(ap->fmt.content.udp_encapsulated.payload);
    struct sockaddr* addr = NULL;
    socklen_t addrlen = 0;
    if (udp_t->set) {
      addr = (struct sockaddr*) &udp_t->addr;
      addrlen = sizeof(struct sockaddr_in);
    }

    bytes_to_send = ap->fmt.headers.size - pkt_header_size;
    nwrite = sendto(fdinfo->fd,
                  &(ap->fmt.content.udp_encapsulated.payload),
                  bytes_to_send,
                  0,
                  addr,
                  addrlen);

    if (nwrite == -1 && errno == EAGAIN) return FDS_AGAIN;
    if (nwrite != bytes_to_send) return FDS_ERR;
    bp->awrite += nwrite;

  } while((ap = ap_next(ap)) != NULL);

  bp->mode = BP_READING;
  bp->aread = 0;
  bp->ap_count = 0;

  return FDS_READY;
}

enum FD_STATE read_packet_from_udp (struct evt_core_fdinfo* fdinfo, struct buffer_packet* bp, struct udp_target* udp_t) {
  ssize_t nread;
  union abstract_packet* ap = (union abstract_packet*) &bp->ip;

  if (bp->mode != BP_READING) {
    fprintf(stderr, "Buffer packet is not in reading mode (mode: %d)\n", bp->mode);
    return FDS_ERR;
  }

  size_t pkt_header_size = sizeof(ap->fmt.headers) + sizeof(ap->fmt.content.udp_encapsulated) - sizeof(ap->fmt.content.udp_encapsulated.payload);
  size_t udp_packet_size = sizeof(bp->ip) - pkt_header_size;
  socklen_t addrlen = sizeof(struct sockaddr_in);

  nread = recvfrom(fdinfo->fd,
                   &(ap->fmt.content.udp_encapsulated.payload),
                   udp_packet_size,
                   MSG_TRUNC,
                   (struct sockaddr*)&udp_t->addr,
                   &addrlen);

  if ((int)nread > (int)udp_packet_size) {
    fprintf(stderr, "Packet has been truncated (%ld instead of %d)\n", nread, (int)udp_packet_size);
    return FDS_ERR;
  }
  if (nread == -1 && errno == EAGAIN) return FDS_AGAIN;
  if (nread == 0) return FDS_AGAIN;
  if (nread == -1) {
    fprintf(stderr, "A system error occurred\n");
    return FDS_ERR;
  }

  udp_t->set = 1;
  udp_t->addrlen = addrlen;
  ap->fmt.headers.size = nread + pkt_header_size;
  ap->fmt.headers.cmd = CMD_UDP_ENCAPSULATED;
  ap->fmt.content.udp_encapsulated.port = url_get_port_int (fdinfo->url);

  bp->mode = BP_WRITING;
  bp->awrite = 0;
  bp->ap_count = 1;

  return FDS_READY;
}

void dump_buffer_packet(struct buffer_packet* bp) {
  printf("<Buffer Packet>\n");
  printf("  mode=%d, aread=%d, awrite=%d, ap_count=%d, usage=%ld/%ld\n", bp->mode, bp->aread, bp->awrite, bp->ap_count, buffer_full_size (bp), sizeof(bp->ip));
  for (union abstract_packet* ap = buffer_first_ap (bp); ap != NULL; ap = ap_next (ap)) {
    dump_abstract_packet(ap);
  }
  printf("</Buffer Packet>\n");
}

void dump_abstract_packet(union abstract_packet* ap) {
  printf("  <Abstract Packet>\n");
  printf("    size=%d, cmd=%d\n", ap->fmt.headers.size, ap->fmt.headers.cmd);
  switch (ap->fmt.headers.cmd) {
  case CMD_LINK_MONITORING_THUNDER:
    printf("    <LinkMonitoringThunder></LinkMonitoringThunder>\n");
    break;
  case CMD_UDP_METADATA_THUNDER:
    printf("    <UdpMetadataThunder>id=%d</UdpMetadataThunder>\n",
           ap->fmt.content.udp_metadata_thunder.id);
    break;
  case CMD_UDP_ENCAPSULATED:
    printf("    <Payload>port=%d</Payload>\n", ap->fmt.content.udp_encapsulated.port);
    break;
  case CMD_LINK_MONITORING_LIGHTNING:
    printf("    <LinkMonitoringLightning>id=%ld</LinkMonitoringLightning>\n", ap->fmt.content.link_monitoring_lightning.id);
    break;
  default:
    printf("    <Unknown/>\n");
    break;

  }
  printf("  </Abstract Packet>\n");
}
