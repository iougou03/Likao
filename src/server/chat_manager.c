#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <time.h>

#include "../lib/chat.h"
#include "util.h"

/**
 * error code
 * -1: there already user signed
 * -2: error while finding chats/
 * -3: error while saving json file
*/
int create_chat_room(char* room_name, struct chat_json_t chat) {
    if (chdir("chats") == -1) {
        fprintf(stderr, "there error while entering chats/, check init()");
        return -2;
    }

    int flag = 0, fd;
    char *ext = ".json";
    char *filename = (char*)malloc(sizeof(char) * strlen(room_name));

    strcpy(filename, room_name);
    strcat(filename, ext);

    if ((fd = open(filename, O_RDONLY)) == -1) {
        fd = open(filename, O_WRONLY | O_CREAT);

        struct json_object* chat_j_obj = json_object_new_object();

        json_object_object_add(chat_j_obj, "name", json_object_new_string(chat.name));
        json_object_object_add(chat_j_obj, "users", chat.users);
        json_object_object_add(chat_j_obj, "created_at", json_object_new_int(chat.created_at));
    
        if (json_object_to_fd(fd, chat_j_obj, JSON_C_TO_STRING_PRETTY  | O_SYNC) == -1) {
            perror(json_util_get_last_err());
            flag = -3;
        }
    }
    else flag = -1;

    chmod(filename, 00744);

    close(fd);
    chdir("..");
    free(filename);

    return flag;
}