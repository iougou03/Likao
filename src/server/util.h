#include <json-c/json.h>
#include "../lib/chat.h"

void json_to_strcut(struct json_object* j_obj, struct msg_from_client_t* msgp);

void struct_to_json(struct json_object* j_obj, struct msg_from_server_t msg);
