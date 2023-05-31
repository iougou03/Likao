#include <stdio.h>
#include <json-c/json.h>
#include <time.h>
#include <pthread.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"

#ifndef HEADER_FILE_SERVER_UTIL
#define HEADER_FILE_SERVER_UTIL

// void json_to_struct(struct json_object*, struct to_server_msg_t*);

// void struct_to_json(struct json_object*, void*);

struct client_arr_t {
    int **pipe_arr;
    int len;
    pthread_t *tid_arr;
};

struct user_json_t {
    char *name;
    char *password;
    time_t created_at;
};

struct chat_json_t {
    char *name;
    char *owner_name;
    struct json_object *users;
    time_t created_at;
};

struct log_json_t {
    struct json_object *data;
};
struct log_item_json_t {
    time_t created_at;
    char *sende;
    char *msg;
};

#endif