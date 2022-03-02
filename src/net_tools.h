// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int create_tcp_client(char* host, char* service);
int create_udp_client(char* host, char* service);
int create_tcp_server(char* host, char* service);
int create_udp_server(char* host, char* service);
int make_socket_non_blocking(int fd);
void add_fd_to_epoll(int epollfd, int fd, uint32_t flags);
void update_fd_epoll(int epollfd, int fd, uint32_t flags);
int read_entity(int fd, void* entity, int size);
void fill_buffer(size_t* written, char* dest, void *src, size_t n);
void fill_buffer2(size_t* written, char* dest, void *start, void *stop);
