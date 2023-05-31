#include <stdio.h>
#include <json-c/json.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"

int sign_in(struct to_server_auth_msg_t msg) {
    char path[FILENAME_MAX];
    sprintf(path, "usr/%s.json", msg.name);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -2;
    }

    struct json_object *user_obj = json_object_from_file(path);
    if (user_obj == NULL) {
        fprintf(stderr, "%s", json_util_get_last_err());
        return -3;
    }

    fclose(fp);
    
    json_object *password_obj;
    json_object_object_get_ex(user_obj, "password", &password_obj);

    const char *password = json_object_get_string(password_obj);

    if (strcmp(msg.password, password) == 0) {
        return 0;
    }

    return -1;
}

int sign_up(struct to_server_auth_msg_t msg) {
    if (chdir("./usr") == -1) {
        fprintf(stderr, "there error while entering usr/, check init()");
        return -2;
    }
    int flag = 0, fd;
    char *ext = ".json";
    char *filename =  (char*)malloc(sizeof(char) * (strlen(msg.name) + strlen(ext)));

    strcpy(filename, msg.name);
    strcat(filename, ext);
    
    if ((fd = open(filename, O_RDONLY)) == -1) {
        time_t now = time(0);
        struct json_object *usr_json = json_object_new_object();
        json_object_object_add(usr_json, "name", json_object_new_string(msg.name));
        json_object_object_add(usr_json, "password", json_object_new_string(msg.password));
        json_object_object_add(usr_json, "created_at", json_object_new_int64(now));

        fd = open(filename, O_WRONLY | O_CREAT, 0700);

        if (json_object_to_fd(fd, usr_json, JSON_C_TO_STRING_PRETTY) == -1) {
            json_object_put(usr_json);
            perror(json_util_get_last_err());
            flag = -3;
        }

        flag = 0;
        json_object_put(usr_json);
    }
    else flag = -1;

    free(filename);
    close(fd);
    chdir("..");

    return flag;
}

void json_to_struct(struct json_object* j_obj, struct to_server_auth_msg_t* msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0) {
            msgp->type = json_object_get_int(val);
        }
        else if (strcmp(key, "name") == 0) {
            char *name = (char*)json_object_get_string(val);
            dynamic_string_copy(&msgp->name, name);
        }
        else if (strcmp(key, "password") == 0) {
            char *password = (char*)json_object_get_string(val);
            dynamic_string_copy(&msgp->password, password);
        }
    }
}

int pth_auth(sock_fd_t client_sock) {
    int flag, running = 1;
    char *raw_msg = NULL;

    while (running) {
        tcp_block(client_sock);

        if (recv_dynamic_data_tcp(client_sock, &raw_msg) == -1) {
            fprintf(stderr, "recv_dynamic_data_tcp error\n");
            return -2;
        }

        struct json_object *recv_obj = json_tokener_parse(raw_msg);

        if (recv_obj == NULL) {
            fprintf(stderr, "wrong format message received\n");
            return -2;
        }

        struct to_server_auth_msg_t recv_msg = { -1, NULL, NULL };

        json_to_struct(recv_obj, &recv_msg);

        if ((recv_msg.type == SIGN_IN && sign_in(recv_msg) == 0) ||
            (recv_msg.type == SIGN_UP && sign_up(recv_msg) == 0)) {
            flag = 0;
        }
        else flag = -1;

        free(recv_msg.name);
        free(recv_msg.password);
        json_object_put(recv_obj);

        struct to_client_auth_msg_t send_msg = { -1, NULL };

        if (flag == 0) {
            send_msg.type = SUCCESS;
            dynamic_string_copy(&(send_msg.message), "auth success");
        }
        else {
            send_msg.type = FAILED;
            dynamic_string_copy(&(send_msg.message), "auth failed");
        }


        tcp_non_block(client_sock);

        struct json_object *send_obj = json_object_new_object();

        json_object_object_add(send_obj, "type", json_object_new_int64(send_msg.type));
        json_object_object_add(send_obj, "message", json_object_new_string(send_msg.message));
        
        while (1) {
            char *buffer = NULL;
            
            if (recv_dynamic_data_tcp(client_sock, &buffer) != -1) {
                if (flag == 0) running = 0;
                break;
            }

            if (send_dynamic_data_tcp(client_sock, (char*)json_object_get_string(send_obj)) == -1) {
                fprintf(stderr, "send_dynamic_data_tcp wrong client socket\n");
                running = 0;
                break;
            }
            sleep(1);
        }
        json_object_put(send_obj);
        clean_socket_buffer(client_sock);

        if (send_msg.message != NULL)
            free(send_msg.message);
    }

    return flag;
}