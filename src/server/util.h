#include <json-c/json.h>
#include "../lib/chat.h"


void json_to_strcut(struct json_object* j_obj, struct msg_from_client_t* msgp);

void struct_to_json(struct json_object* j_obj, struct msg_from_server_t msg);


#ifndef HEADER_FILE_H
#define HEADER_FILE_H

struct user_json_t {
    char name[NAME_MAX_LEN];
    char password[PASSWORD_MAX_LEN];
    int created_at;
};

#endif