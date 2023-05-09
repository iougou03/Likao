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

#include "../lib/chat.h"
#include "util.h"
#include "auth.h"

int init(char* execute_name) {
    mode_t default_mode = 01762;

    char* userDir = "usr", 
        * chatsDir = "chats",
        * logsDir = "logs";

    if (opendir(userDir) == NULL) {
        mkdir(userDir, default_mode);
    }

    if (opendir(chatsDir) == NULL) {
        mkdir(chatsDir, default_mode);
    }

    if (opendir(logsDir) == NULL) {
        mkdir(logsDir, default_mode);
    }

    chmod(execute_name, 04711);

    // TODO: craete fork for each chat rooms
}

void server() {
    sock_fd socket_fd, client_socket_fd;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    void* sign_handler();

    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error while creating socket, plz try again later\n");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("error while binding socket with address\n");
        exit(1);
    }

    if (listen(socket_fd, 40) == -1) {
        perror("listening mode failed!\n");
        exit(1);
    }

    printf("The server has opened at port %d!\n", DEFAULT_PORT);

    pthread_t thread_id;

    while (1) {
        client_size = sizeof(client_addr);

        if ((client_socket_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_size)) == -1) {
            perror("Error while accepting client!\n");
            // TODO: tell client connection failed
        }

        if (pthread_create(&thread_id, NULL, sign_handler, (void*)&client_socket_fd) == -1) {
            perror("cannot create thread for client");
            // TODO: server handle thread create error
        }
        else {
            pthread_detach(thread_id);
        }
    }

    close(socket_fd);
}

int send_chats_list(sock_fd client_socket_fd) {
    DIR *chats_dfd;
    int flag = 0;

    if ((chats_dfd = opendir("chats")) == NULL) {
        fprintf(stderr, "error while readling chats/, check init()");
        return -1;
    }
    
    struct dirent *direntp;
    char *list = (char*)malloc(sizeof(char));
    strcpy(list, "[");

    while ((direntp = readdir(chats_dfd)) != NULL) {
        if (strcmp(".", direntp->d_name) == 0 || strcmp("..", direntp->d_name) == 0) {
            continue;
        }

        int len = strlen(direntp->d_name);
        if ((list = (char*)realloc(list, sizeof(char) * (strlen(list) + 1 + len))) == NULL) {
            flag = -2;
            break;
        }
        strcat(list, direntp->d_name);
        strcat(list, ",");
    }

    closedir(chats_dfd);

    list[strlen(list) - 1] = ']';

    printf("send list: %s", list);
    fflush(stdout);
    flag = 0;

    free(list);
    // struct json_object *chats_list_json = json_object_new_object();

    return flag;
}

void* sign_handler(void* client_sock_fdp) {
    pthread_t tid = pthread_self();
    printf("[Client conencted]: thread %ld %d\n", tid, gettid());
    
    sock_fd client_sock_fd = *((int*)client_sock_fdp);
    void* received_msg_raw = malloc(sizeof(struct msg_from_client_t));

    /**
     * TODO: if user termiate, server also terminate
     * 
     * RESOLVE: should not terminate
    */
    if (recv(client_sock_fd, received_msg_raw ,sizeof(struct msg_from_client_t), 0) == -1) {
        perror("error while receiving");
        pthread_cancel(tid);
    }

    struct json_object *j_obj = json_tokener_parse(received_msg_raw);
    struct msg_from_client_t msg;

    json_to_strcut(j_obj, &msg);

    printf("recv message: %d, %s %s\n",msg.type, msg.name, msg.password);

    struct msg_from_server_t msg_server;
    struct json_object *send_json_obj = json_object_new_object();

    // TODO: sign handler
    if (msg.type == SIGN_IN) {
        sign_in(msg.name, msg.password);
    }
    else if (msg.type == SIGN_UP) {
        int flag = sign_up(msg.name, msg.password);
        if (flag == -1) {
            msg_server.type = FAILED;
            strcpy(msg_server.msg, "there already signed user!");

            struct_to_json(send_json_obj, msg_server);

            send(
                client_sock_fd,
                json_object_to_json_string(send_json_obj),
                sizeof(struct msg_from_server_t),
                0
            );
        }
        else if (flag == 0) {
            msg_server.type = SUCCESS;
            strcpy(msg_server.msg, "you success to sign up!");

            struct_to_json(send_json_obj, msg_server);

            if (send(
                client_sock_fd,
                json_object_to_json_string(send_json_obj),
                sizeof(struct msg_from_server_t),
                0
            ) == -1) {
                perror("error at sending sign msg to client");
            }
            else {
                // send_chats_list(client_sock_fd);
            }
        }
        else {
            fprintf(stderr, "error while sign up");
        }

    }
    
    json_object_put(j_obj);
    json_object_put(send_json_obj);
    free(received_msg_raw);
    close(client_sock_fd);
}

int main (int argc, char** argv) {
    init(argv[0]);

    server();

    return 0;
}
