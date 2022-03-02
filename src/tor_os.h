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
#include <fcntl.h>
#include <string.h>

struct keypair {
  char* priv;
  char* pub;
};

struct tor_os_str {
    size_t filled;
    size_t size;
    char* pub_file;
    char* priv_file;
    struct keypair *keys;
};

void tor_os_create(struct tor_os_str* os, char* pub_file, char* priv_file, size_t size);
struct keypair* tor_os_append_cursor(struct tor_os_str* os);
int tor_os_append(struct tor_os_str* os, char* pub, char* priv);
void tor_os_read (struct tor_os_str* os);
void tor_os_persist(struct tor_os_str* os);
void tor_os_free(struct tor_os_str* os);
