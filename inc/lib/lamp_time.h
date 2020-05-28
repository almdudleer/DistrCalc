//
// Created by almdudleer on 09.05.2020.
//

#ifndef PAS_LAMP_TIME_H
#define PAS_LAMP_TIME_H

#include "ipc.h"

timestamp_t inc_lamport_time();

short compare_and_inc_time(timestamp_t val);

#endif //PAS_LAMP_TIME_H
