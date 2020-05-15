//
// Created by almdudleer on 04.05.2020.
//

#ifndef PAS_IPC_UTILS_H
#define PAS_IPC_UTILS_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "entity.h"

#define WAIT_TIME_NS (200000000 / 2)
#define TIMEOUT_NS 2000000000
#define TIMEOUTS 0

int receive_or_die(Unit* self, local_id from, Message* msg);

int receive_any_or_die(Unit* self, Message* msg);

#include "utils.h"

#endif //PAS_IPC_UTILS_H
