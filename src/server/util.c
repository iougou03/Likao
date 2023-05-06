#include <json-c/json.h>
#include <string.h>

#include "../lib/chat.h"

void json_to_strcut(struct json_object* j_obj, struct msg_from_client_t* msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0) {
            msgp->type = json_object_get_int(val);
        }
        else if (strcmp(key, "name") == 0) {
            strcpy(msgp->name, json_object_get_string(val));
        }
        else if (strcmp(key, "password") == 0) {
            strcpy(msgp->password, json_object_get_string(val));
        }
    }
}

void struct_to_json(struct json_object* j_obj, struct msg_from_server_t msg) {
    json_object_object_add(j_obj, "type", json_object_new_int(msg.type));
    json_object_object_add(j_obj, "msg", json_object_new_string(msg.msg));
}
