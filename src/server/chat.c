#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <signal.h>
#include <pthread.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./globals.h"
#include "./chat_child.h"

int send_chats_list(sock_fd_t client_sock) {
    DIR *chats_dfd;
    if ((chats_dfd = opendir("chats")) == NULL) {
        fprintf(stderr, "error while readling chats/, check init()");
        return -2;
    }

    struct dirent *direntp;
    int cnt = 0;
    char *list = NULL;
    dynamic_string_copy(&list, "[");

    while ((direntp = readdir(chats_dfd)) != NULL) {
        if (strcmp(".", direntp->d_name) == 0 || 
            strcmp("..", direntp->d_name) == 0 ||
            strstr(direntp->d_name, ".json") == NULL
            ) {
            continue;
        }

        int len = strlen(direntp->d_name) - 5; // removing extension name(.json)
        if ((list = (char*)realloc(list, sizeof(char) * (2 + strlen(list) + 1 + len))) == NULL) {
            return -1;
        }
        strcat(list, "\"");
        strncat(list, direntp->d_name, len);
        strcat(list, "\",");

        cnt++;
    }

    if (cnt > 0) {
        list = (char*)realloc(list, sizeof(char) * (strlen(list) + 1));
        list[strlen(list) - 1] = ']';
        list[strlen(list)] = '\0';
    }
    else {
        list = (char*)realloc(list, sizeof(char) * (strlen(list) + 2));
        list[strlen(list)] = ']';
        list[strlen(list) + 1] = '\0';
    }

    struct json_object *chats_list_obj = json_tokener_parse(list);

    send_dynamic_data_tcp(client_sock, (void*)json_object_to_json_string(chats_list_obj));

    closedir(chats_dfd);
    free(list);
    json_object_put(chats_list_obj);
    
    return 0;
}

int create_chat(struct to_server_chat_msg_t msg) {
    if (chdir("./chats") == -1) {
        fprintf(stderr, "there error while entering chats/, check init()");
        return -2;
    }
    int flag = 0, fd;
    char *ext = ".json";
    char *filename =  (char*)malloc(sizeof(char) * (strlen(msg.chat_room_name) + strlen(ext)));

    strcpy(filename, msg.chat_room_name);
    strcat(filename, ext);
    
    if ((fd = open(filename, O_RDONLY)) == -1) {
        time_t now = time(0);
        struct json_object *chat_obj = json_object_new_object();
        json_object_object_add(chat_obj, "name", json_object_new_string(msg.chat_room_name));
        json_object_object_add(chat_obj, "owner_name", json_object_new_string(msg.name));
        json_object_object_add(chat_obj, "created_at", json_object_new_int64(now));
        json_object_object_add(chat_obj, "users", json_tokener_parse("[]"));

        fd = open(filename, O_WRONLY | O_CREAT, 0744);

        if (json_object_to_fd(fd, chat_obj, JSON_C_TO_STRING_PRETTY) == -1) {
            perror(json_util_get_last_err());
            flag = -2;
        }
        else {
            fsync(fd);

            pthread_mutex_lock(&ENV.mutex);

            string_arr_append(&ENV.child_names, msg.chat_room_name);
            child_server(DEFAULT_PORT + ENV.child_names.len);
            flag = DEFAULT_PORT + ENV.child_names.len;

            pthread_mutex_unlock(&ENV.mutex);

        }
        json_object_put(chat_obj);
    }
    else flag = -1;

    free(filename);
    close(fd);
    chdir("..");

    return flag;
}

int join_chat (sock_fd_t client_sock, struct to_server_chat_msg_t msg) {
    int flag = 0;
    char path[FILENAME_MAX];
    sprintf(path, "chats/%s.json", msg.chat_room_name);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    fclose(fp);

    struct json_object *chat_obj = json_object_from_file(path);
    if (!chat_obj) {
        fprintf(stderr, "%s", json_util_get_last_err());
        return -2;
    }

    struct json_object *users_arr;
    json_object_object_get_ex(chat_obj, "users", &users_arr);

    if (json_object_is_type(users_arr, json_type_array)) {
        json_object_array_add(users_arr, json_object_new_string(msg.name));

        json_object_to_file_ext(path, chat_obj, JSON_C_TO_STRING_PRETTY);

        pthread_mutex_lock(&ENV.mutex);

        for (int i = 0 ; i < ENV.child_names.len ; i++) {
            if (strcmp(ENV.child_names.data[i], msg.chat_room_name) == 0) {
                flag = DEFAULT_PORT + i + 1;
                break;
            }
        }

        pthread_mutex_unlock(&ENV.mutex);
    }else flag = -2;

    json_object_put(chat_obj);
    return flag;
}

void json_to_struct_chat(struct json_object *j_obj, struct to_server_chat_msg_t *msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0) {
            msgp->type = json_object_get_int(val);
        }
        else if (strcmp(key, "name") == 0) {
            char *name = (char*)json_object_get_string(val);
            dynamic_string_copy(&(msgp->name), name);
        }
        else if (strcmp(key, "chat_room_name") == 0) {
            char *chat_room_name = (char*)json_object_get_string(val);
            dynamic_string_copy(&(msgp->chat_room_name), chat_room_name);
        }
    }
}

void handle_chat_room_update() {
    printf("received, should print chat room again\n");
    fflush(stdout);
}

void chat_manager(sock_fd_t client_sock, int client_idx) {
    printf("chat manager on\n");
    fflush(stdout);

    signal(SIGUSR1, handle_chat_room_update);

    if (send_chats_list(client_sock) < 0) return;

    tcp_non_block(client_sock);

    int running = 1;

    char *msg_raw = NULL;
    while (running) {
        if (recv_dynamic_data_tcp(client_sock, &msg_raw) == -1) {
            if (send(client_sock, " ", sizeof(char), 0) == -1) {
                break;
            }
            sleep(1);
            continue;
        }

        struct json_object *recv_obj = json_tokener_parse(msg_raw);

        if (recv_obj == NULL) {
            free(msg_raw);
            continue;
        }

        struct to_server_chat_msg_t recv_msg = {-1, NULL, NULL };

        json_to_struct_chat(recv_obj, &recv_msg);

        struct to_client_chat_msg_t send_msg = { -1, -1 };

        if (recv_msg.type == JOIN) {
            int room_port;
            if ((room_port = join_chat(client_sock, recv_msg)) > 0){
                send_msg.type = SUCCESS;
                send_msg.chat_port = room_port;
            }
            else {
                send_msg.type = FAILED;
            }
            struct json_object *send_obj = json_object_new_object();
            json_object_object_add(send_obj, "type", json_object_new_int(send_msg.type));
            json_object_object_add(send_obj, "chat_port", json_object_new_int(send_msg.chat_port));

            send_dynamic_data_tcp(client_sock, (char*)json_object_get_string(send_obj));
            
            json_object_put(send_obj);
        }
        else if (recv_msg.type == CREATE) {
            int room_port;
            if ((room_port = create_chat(recv_msg)) > 0) {
                if (join_chat(client_sock, recv_msg) > 0) {
                    send_msg.type = SUCCESS;
                    send_msg.chat_port = room_port;
                }
                else {
                    send_msg.type = FAILED;
                }
            }
            else {
                send_msg.type = FAILED;
            }

            struct json_object *send_obj = json_object_new_object();
            json_object_object_add(send_obj, "type", json_object_new_int(send_msg.type));
            json_object_object_add(send_obj, "chat_port", json_object_new_int(send_msg.chat_port));

            send_dynamic_data_tcp(client_sock, (char*)json_object_get_string(send_obj));
            
            json_object_put(send_obj);

            int fire_all_update = SIGUSR1;

            // if (send_msg.type == SUCCESS) {
            //     write(ENV.clients_pipe.pipe_arr[client_idx][1], &fire_all_update, sizeof(int));
            // }
        }

        json_object_put(recv_obj);
    }
    if (msg_raw == NULL)
        free(msg_raw);

}