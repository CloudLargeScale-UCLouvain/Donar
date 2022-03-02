// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "net_tools.h"

// some code is inspired by https://github.com/millken/c-example/blob/master/epoll-example.c
// (which seems to be copied from https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c )

int create_ip_client(char* host, char* service, int type) {
    int err, sock, enable;
    struct addrinfo conf;
    struct addrinfo *result, *cursor;

    memset(&conf, 0, sizeof(struct addrinfo));
    conf.ai_family = AF_UNSPEC;
    conf.ai_socktype = type;
    conf.ai_flags = 0;
    conf.ai_protocol = 0;

    enable = 1;
    err = getaddrinfo(host, service, &conf, &result);
    if (err != 0) {
        fprintf(stderr, "Error with getaddrinfo() for %s:%s\n",host,service);
        exit(EXIT_FAILURE);
    }

    for (cursor = result; cursor != NULL; cursor = cursor->ai_next) {
        sock = socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
        if (sock == -1) continue;
        err = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        if (err < 0) {
          fprintf(stderr, "Error setting socket to SO_REUSEADDR\n");
          exit(EXIT_FAILURE);
        }
        if (connect(sock, cursor->ai_addr, cursor->ai_addrlen) != -1) break;
        close(sock);
    }

    if (cursor == NULL) {
        fprintf(stderr, "No connect worked for %s:%s\n", host, service);
        return -1;
        //exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    return sock;
}

int create_tcp_client(char* host, char* service) {
  int sock = create_ip_client (host, service, SOCK_STREAM);
  if (sock < 0) return sock;
  int activate = 1;
  int err;
  err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &activate, sizeof(activate));
  if (err < 0) {
    perror("setsockopt TCP_NODELAY");
    exit(EXIT_FAILURE);
  }
  return sock;
}

int create_udp_client(char* host, char* service) {
  return create_ip_client (host, service, SOCK_DGRAM);
}

int create_ip_server(char* host, char* service, int type) {
  int err, sock, enable;
  struct addrinfo conf;
  struct addrinfo *result, *cursor;

  memset(&conf, 0, sizeof(struct addrinfo));
  conf.ai_family =  AF_INET; // AF_UNSPEC to listen on IPv6 or IPv4
  conf.ai_socktype = type;
  conf.ai_flags = 0; // AI_PASSIVE to listen on 0.0.0.0
  conf.ai_protocol = 0;

  enable = 1;
  err = getaddrinfo(host, service, &conf, &result);
  if (err != 0) {
    fprintf(stderr, "Error with getaddrinfo()\n");
    exit(EXIT_FAILURE);
  }

  for (cursor = result; cursor != NULL; cursor = cursor->ai_next) {
    sock = socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
    if (sock == -1) continue;
    err = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (err < 0) {
      fprintf(stderr, "Error setting socket to SO_REUSEADDR\n");
      exit(EXIT_FAILURE);
    }
    if (bind(sock, cursor->ai_addr, cursor->ai_addrlen) != -1) break;
    perror("Bind failed");
    close(sock);
  }

  if (cursor == NULL) {
    fprintf(stderr, "We failed to create socket or bind for %s:%s (%d)\n", host, service, type);
    exit(EXIT_FAILURE);
  }

  freeaddrinfo (result);

  return sock;
}

int create_tcp_server(char* host, char* service) {
  int sock = create_ip_server (host, service, SOCK_STREAM);
  if (sock < 0) return sock;
  int activate = 1;
  int err;
  err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &activate, sizeof(activate));
  if (err < 0) {
    perror("setsockopt TCP_NODELAY");
    exit(EXIT_FAILURE);
  }

  return sock;
}

int create_udp_server(char* host, char* service) {
  return create_ip_server (host, service, SOCK_DGRAM);
}

/*
 * The idea is to get the file descriptor flags
 * then we add to the bitmask O_NONBLOCK and set the
 * new bitmask to the socket
 */
int make_socket_non_blocking(int fd) {
  int err, flags;
  flags = fcntl (fd, F_GETFL);
  if (flags == -1) {
    perror("Failed to get socket's flags via fcntl");
    return -1;
  }

  flags |= O_NONBLOCK;
  err = fcntl (fd, F_SETFL, flags);
  if (err != 0) {
    perror("Failed to set socket's flags via fcntl");
    return -1;
  }

  return 0;
}

int read_entity(int fd, void* entity, int size) {
    int remaining = size;
    int total_read = 0;

    while (remaining > 0) {
        int nread = read(fd, ((char*)entity)+total_read, remaining);
        if (nread == -1) {
            return nread;
        }
        remaining -= nread;
        total_read += nread;
    }
    return total_read;
}

void fill_buffer(size_t* written, char* dest, void *src, size_t n) {
    memcpy(dest+*written, src, n);
    *written += n;
}

void fill_buffer2(size_t* written, char* dest, void *start, void *stop) {
    memcpy(dest+*written, start, stop - start);
    *written += stop - start;
}

/*
 * Trying with Level Triggered for now --------
 * Be careful, if configured as edge triggered and not level triggered
 * You need to read everything before going back to epoll
 * Which means keeping state too
 */
void add_fd_to_epoll(int epollfd, int fd, uint32_t flags) {
  int err = make_socket_non_blocking (fd);
  if (err == -1) {
    fprintf(stderr, "Unable to set fd=%d to non blocking\n", fd);
    exit(EXIT_FAILURE);
  }

  struct epoll_event current_event = {0};
  //current_event.events = EPOLLIN | EPOLLET;
  current_event.events = flags;
  current_event.data.fd = fd;
  if (epoll_ctl (epollfd, EPOLL_CTL_ADD, fd, &current_event) == -1) {
    perror("Failed to add a file descriptor to epoll with epoll_ctl");
    exit(EXIT_FAILURE);
  }
}

void update_fd_epoll(int epollfd, int fd, uint32_t flags) {
  struct epoll_event current_event = {0};
  current_event.events = flags;
  current_event.data.fd = fd;
  if (epoll_ctl (epollfd, EPOLL_CTL_MOD, fd, &current_event) == -1) {
    perror("Failed to update a file descriptor to epoll with epoll_ctl");
    exit(EXIT_FAILURE);
  }
}
