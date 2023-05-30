#include "../lib/likao_utils.h"

#ifndef GLOBAL_H
#define GLOBAL_H

struct env_t {
    int max_password_cnt;
    int *child_pids;
    int *child_ports;
    struct string_arr_t child_names;
};

extern struct env_t ENV;

#endif