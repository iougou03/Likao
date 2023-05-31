#include "../lib/likao_utils.h"
#include "./utils.h"

#ifndef GLOBAL_H
#define GLOBAL_H

struct env_t {
    int max_password_cnt;
    int *child_pids;
    int *child_ports;
    struct string_arr_t child_names;
    struct client_arr_t clients_pipe;
    pthread_mutex_t mutex;
};

extern struct env_t ENV;

#endif