#include <json-c/json.h>

#include "../lib/chat.h"

#ifndef HEADER_FILE_SERVER_CHAT_MANAGER
#define HEADER_FILE_SERVER_CHAT_MANAGER

int create_chat_room(char* room_name, struct json_object* msg_client_json);

#endif