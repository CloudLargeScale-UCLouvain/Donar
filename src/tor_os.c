// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "tor_os.h"

void tor_os_create(struct tor_os_str* os, char* pub_file, char* priv_file, size_t size) {
    os->size = size;
    os->filled = 0;
    os->priv_file = NULL;

    if (priv_file != NULL) {
      os->priv_file = malloc(sizeof(char) * (strlen(priv_file) + 1));
      if (os->priv_file == NULL) goto mem_error;
      strcpy(os->priv_file, priv_file);
    }

    os->pub_file = malloc(sizeof(char) * (strlen(pub_file) + 1));
    if (os->pub_file == NULL) goto mem_error;
    strcpy(os->pub_file, pub_file);

    os->keys = malloc(size * sizeof(struct keypair));
    if (os->keys == NULL) goto mem_error;
    return;

mem_error:
    fprintf(stderr, "unable to allocate memory\n");
    exit(EXIT_FAILURE);
}

struct keypair* tor_os_append_cursor(struct tor_os_str* os) {
    if (os->filled == os->size) {
        return NULL;
    }
    struct keypair* cur = os->keys + os->filled;
    os->filled++;
    cur->priv = NULL;
    cur->pub = NULL;
    return cur;
}

void tor_os_read (struct tor_os_str* os) {
    FILE* fd = NULL;

    if (os->priv_file != NULL) {
      fd = fopen(os->priv_file, "r");
    } else {
      fd = fopen(os->pub_file, "r");
    }

    if (fd == NULL) {
        return;
    }
    size_t len = 0;
    size_t str_len = 0;
    int n = 0;
    char* last_key = NULL;
    while (1) {
        struct keypair* dst = tor_os_append_cursor(os);
        if (dst == NULL) break;
        if (os->priv_file != NULL) {
          if (fscanf(fd, "%ms %ms", &(dst->pub), &(dst->priv)) == -1) break;
          last_key = dst->priv;
        } else {
          if (fscanf(fd, "%ms", &(dst->pub)) == -1) break;
          last_key = dst->pub;
        }
        str_len = strlen(last_key);
        if (str_len < 1) break;
        if (last_key[str_len - 1] == '\n') {
          last_key[str_len - 1] = '\0';
        }
    }
    fclose(fd);
}

void tor_os_free(struct tor_os_str* os) {
    for (int i = 0; i < os->filled; i++) {
        free(os->keys[i].pub);
        if (os->priv_file != NULL) free(os->keys[i].priv);
    }
    free(os->keys);
    if (os->priv_file != NULL) free(os->priv_file);
    free(os->pub_file);
}

void tor_os_persist(struct tor_os_str* os) {
    FILE* fd = NULL;

    fd = fopen(os->pub_file, "w");
    if (fd == NULL) {
      fprintf(stderr, "unable to open pub file %s for writing\n", os->pub_file);
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < os->filled; i++) {
      fprintf(fd, "%s\n", os->keys[i].pub);
    }

    fclose(fd);

    if (os->priv_file == NULL) return;
    fd = fopen(os->priv_file, "w");
    if (fd == NULL) {
      fprintf(stderr, "unable to open priv file %s for writing\n", os->priv_file);
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < os->filled; i++) {
      fprintf(fd, "%s %s\n", os->keys[i].pub, os->keys[i].priv);
    }

    fclose(fd);
}

int tor_os_append(struct tor_os_str* os, char* pub, char* priv) {
    struct keypair* target = tor_os_append_cursor (os);
    if (target == NULL) return -1;

    target->pub = malloc(sizeof(char) * (strlen(pub) + 1));
    if (target->pub == NULL) return -1;
    strcpy (target->pub, pub);

    if (os->priv_file == NULL) return 0;
    target->priv = malloc(sizeof(char) * (strlen(priv) + 1));
    if (target->priv == NULL) return -1;
    strcpy (target->priv, priv);

    return 0;
}
