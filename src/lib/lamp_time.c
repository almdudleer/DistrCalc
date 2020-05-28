//
// Created by almdudleer on 09.05.2020.
//

#include "ipc.h"

timestamp_t lamport_time = 0;

timestamp_t get_lamport_time() {
    return lamport_time;
}

timestamp_t inc_lamport_time() {
    return ++lamport_time;
}

short compare_and_inc_time(timestamp_t val) {
    if (val > lamport_time) {
        lamport_time = val;
    }
    return inc_lamport_time();
}
