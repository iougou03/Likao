#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#include "../lib/chat.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <json-c/json.h>

int sign_in(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]) {
    if (chdir("usr") == -1) {
        fprintf(stderr, "there error while entering usr/, check init()");
        return -3;
    }
    int flag = 0;
    char *ext = ".json";
    char *filename =  (char*)malloc(sizeof(char) * (strlen(name) + strlen(ext)));

    strcpy(filename, name);
    strcat(filename, ext);

    struct json_object *root = json_object_from_file(filename);
    printf("The JSON object is:\n%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        flag = -1; // 이름이 등록되어 있지 않음
    }
    else {
        char buf[1024];

        struct json_object *usr_obj, *passwd_obj;
        fclose(fp);

        usr_obj = json_tokener_parse(buf);
        json_object_object_get_ex(usr_obj, "password", &passwd_obj);
        if (strcmp(password, json_object_get_string(passwd_obj)) == 0)
            flag = 0; // 로그인 성공
        else
            flag = -2; // 비밀번호 틀림

        json_object_put(usr_obj);
    }

    json_object_put(root);
    free(filename);

    return flag;
}

/**
 * error code
 * -1: there already user signed
 * -2: error while finding usr/
 * -3: error while saving json file
*/
int sign_up(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]) {
    if (chdir("usr") == -1) {
        fprintf(stderr, "there error while entering usr/, check init()");
        return -2;
    }
    int flag = 0;
    char *ext = ".json";
    char *filename =  (char*)malloc(sizeof(char) * (strlen(name) + strlen(ext)));

    strcpy(filename, name);
    strcat(filename, ext);
    

    if (open(filename, O_RDONLY) == -1) {
        time_t now = time(0);
        struct json_object *usr_json = json_object_new_object();
        json_object_object_add(usr_json, "name", json_object_new_string(name));
        json_object_object_add(usr_json, "password", json_object_new_string(password));
        json_object_object_add(usr_json, "created_at", json_object_new_int64(now));

        int fd = open(filename, O_WRONLY | O_CREAT);

        if (json_object_to_fd(fd, usr_json, JSON_C_TO_STRING_PRETTY | O_SYNC) == -1) {
            perror(json_util_get_last_err());
            flag = -3;
        }

        flag = 0;

        chmod(filename, 00744);
        close(fd);
        json_object_put(usr_json);
    }
    else flag = -1;

    chdir("..");
    free(filename);
    return flag;
}