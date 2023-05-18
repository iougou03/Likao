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
#include "../lib/chatutil.h"
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

/**
 * TODO: if user termiate, server also terminate
 * 
 * RESOLVE: should not terminate
*/
void server() {
    sock_fd socket_fd, client_socket_fd;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    void * client_tcp_handler();

    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error while creating socket, plz try again later\n");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

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

        if (pthread_create(&thread_id, NULL, client_tcp_handler, (void*)&client_socket_fd) == -1) {
            perror("cannot create thread for client");
            // TODO: server handle thread create error
        }
        else {
            pthread_detach(thread_id);
        }
    }

    close(socket_fd);
}

int send_chats_list(sock_fd client_sock_fd) {
    DIR *chats_dfd;
    int flag = 0;

    if ((chats_dfd = opendir("chats")) == NULL) {
        fprintf(stderr, "error while readling chats/, check init()");
        return -1;
    }
    
    struct dirent *direntp;
    int cnt = 0;
    char *list = (char*)malloc(sizeof(char) * 3);
    list[0] = '['; list[1] = '\0';

    while ((direntp = readdir(chats_dfd)) != NULL) {
        if (strcmp(".", direntp->d_name) == 0 || strcmp("..", direntp->d_name) == 0) {
            continue;
        }

        int len = strlen(direntp->d_name);
        if ((list = (char*)realloc(list, sizeof(char) * (2 + strlen(list) + 1 + len))) == NULL) {
            flag = -2;
            break;
        }
        char chat_name[NAME_MAX_LEN];
        strncpy(chat_name, direntp->d_name, strlen(direntp->d_name) - 5);
        
        strcat(list, "\"");
        strcat(list, chat_name);
        strcat(list, "\"");
        strcat(list, ",");

        cnt++;
    }
    
    if (cnt == 0) {
        list[strlen(list)] = ']';
        list[strlen(list) + 1] = '\0';
    }
    else {
        list[strlen(list) - 1] = ']';
        list[strlen(list)] = '\0';
    }

    struct json_object *chats_list_json = json_tokener_parse(list);

    const char* msg = json_object_to_json_string(chats_list_json);

    send_dynamic_data_tcp(client_sock_fd, msg);
    flag = 0;

    closedir(chats_dfd);
    free(list);
    json_object_put(chats_list_json);

    return flag;
}

void chats_manager(sock_fd client_sock_fd) {
    struct chats_from_client_t msg_client;
    struct json_object* msg_client_json;

    void* raw_msg = NULL;
    
    raw_msg = recv_dynamic_data_tcp(client_sock_fd);

    if ((msg_client_json = json_tokener_parse((char*)raw_msg)) == NULL) {
        perror("json parse failed");
    }

    json_object_object_foreach(msg_client_json, key, val) {
        if (strcmp(key, "type") == 0)
            msg_client.type = json_object_get_int(val);

        else if (strcmp(key, "room_name") == 0)
            strcpy(msg_client.room_name, json_object_get_string(val));
    }

    if (msg_client.type == CREATE) {
        struct chat_json_t chat;

        time_t now = time(0);
        
        chat.created_at = now;
        strcpy(chat.name, msg_client.room_name);
        // chat.users = json_tokener_parse();

    }
    else if (msg_client.type == JOIN) {

    }
    printf("%d %s\n", msg_client.type, msg_client.room_name);
    fflush(stdout);

    free(raw_msg);
    json_object_put(msg_client_json);
}

void *client_tcp_handler(void* client_sock_fdp) {
    int pth_auth(pthread_t, sock_fd);

    pthread_t tid = pthread_self();
    printf("[Client conencted]: thread %ld %d\n", tid, gettid());
    
    sock_fd client_sock_fd = *((int*)client_sock_fdp);

    if (pth_auth(tid, client_sock_fd) == 0) {
        send_chats_list(client_sock_fd);
        
        chats_manager(client_sock_fd);
    }

    close(client_sock_fd);
}

int main (int argc, char** argv) {
    init(argv[0]);

    server();

    return 0;
}
