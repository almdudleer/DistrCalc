//
// Created by almdudleer on 04.05.2020.
//

#ifndef PAS_IPC_UTILS_H
#define PAS_IPC_UTILS_H

#include <z3.h>
#include "entity.h"
#include "stdio.h"

#define WAIT_TIME_NS 200000000
#define TIMEOUT_NS 2000000000

int receive_or_die(Unit* self, local_id from, Message* msg);

int receive_any_or_die(Unit* self, Message* msg);

#include "utils.h"

#endif //PAS_IPC_UTILS_H
