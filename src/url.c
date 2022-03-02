// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "url.h"

char* url_get_port(char* out, char* in) {
  sscanf(in, "%*[a-z]:%*[a-z]:%*[a-zA-Z0-9.]:%[0-9]", out);
  return out;
}

int url_get_port_int(char* in) {
  int out;
  sscanf(in, "%*[a-z]:%*[a-z]:%*[a-zA-Z0-9.]:%d", &out);
  return out;
}
