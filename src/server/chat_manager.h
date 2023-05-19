#include <json-c/json.h>

#include "util.h"

#ifndef HEADER_FILE_SERVER_CHAT_MANAGER
#define HEADER_FILE_SERVER_CHAT_MANAGER

int create_chat_room(char* room_name, struct chat_json_t chat);

#endif