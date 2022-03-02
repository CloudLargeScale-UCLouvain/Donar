// SPDX-FileCopyrightText: 2019 David Bromberg <david.bromberg@irisa.fr>
// SPDX-FileCopyrightText: 2019 Davide Frey <davide.frey@inria.fr>
// SPDX-FileCopyrightText: 2019 Etienne Riviere <etienne.riviere@uclouvain.be>
// SPDX-FileCopyrightText: 2019 Quentin Dufour <quentin@dufour.io>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <sys/timerfd.h>
#include "evt_core.h"

enum DONAR_TIMER_DECISION {
  DONAR_TIMER_STOP,
  DONAR_TIMER_CONTINUE,
};
typedef enum DONAR_TIMER_DECISION (*timer_cb)(struct evt_core_ctx* ctx, void* user_data);
void init_timer(struct evt_core_ctx* evts);
int set_timeout(struct evt_core_ctx* evts, uint64_t milli_sec, void* ctx, timer_cb cb);
void stop_timer(struct evt_core_ctx* evts);
