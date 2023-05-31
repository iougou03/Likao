#include <stdio.h>
#include <json-c/json.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/select.h>

#include "../lib/likao_chat.h"
#include "./auth.h"
#include "./chat.h"
#include "./utils.h"
#include "./globals.h"
#include "./chat_child.h"

struct client_args {
    int idx;
    sock_fd_t sock;
};

void chat_room_updated() {
    for (int i = 0 ; i < ENV.clients_pipe.len ; i++) {
        printf("send from main\n");
        pthread_kill(ENV.clients_pipe.tid_arr[i], SIGUSR1);
    }
}

void server_on() {
    sock_fd_t server_sock, client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    void *client_tcp_handler();
    void create_chats_process();

    if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int flags = fcntl(server_sock, F_GETFL);
    if (flags == -1) {
        perror("Failed to get socket flags");
        exit(1);
    }
    if (fcntl(server_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Failed to set socket to non-blocking mode");
        exit(1);
    }

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_sock, ENV.max_password_cnt) == -1) {
        perror("listen");
        exit(1);
    }

    printf("The server has opened at port %d\nmax people: %d\n", DEFAULT_PORT, ENV.max_password_cnt);
    fflush(stdout);

    create_chats_process();

    pthread_t thread_id;

    fd_set read_set;
    FD_ZERO(&read_set);

    int signal_code, bytes_received;

    while (1) {
        client_size = sizeof(client_addr);
        
        // for (int i = 0 ; i < ENV.clients_pipe.len ; i++) {
        //     bytes_received = read(ENV.clients_pipe.pipe_arr[i][0], &signal_code, sizeof(int));
        //     if (bytes_received > 0) {
        //         chat_room_updated();
        //         break;
        //     }
        //     if (bytes_received == -1) {
                // close(ENV.clients_pipe.data[i][0]);
                // close(ENV.clients_pipe.data[i][1]);

                // free(ENV.clients_pipe.data[i]);
                // for (int j = i + 1 ; j < ENV.clients_pipe.len ; j++) {
                //     ENV.clients_pipe.data[j - 1] = ENV.clients_pipe.data[j];
                // }
                // ENV.clients_pipe.data= realloc(ENV.clients_pipe.data, sizeof(int**) * --ENV.clients_pipe.len); 
                // break;
        //     }
        // }

        if ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_size)) == -1) {
            sleep(1);
            continue;
        }

        ENV.clients_pipe.pipe_arr = realloc(ENV.clients_pipe.pipe_arr, sizeof(int**) * ++ENV.clients_pipe.len);
        ENV.clients_pipe.pipe_arr[ENV.clients_pipe.len - 1] = (int*)malloc(sizeof(int) * 2);
        ENV.clients_pipe.tid_arr = realloc(ENV.clients_pipe.pipe_arr, sizeof(int) * ENV.clients_pipe.len);

        struct client_args args = { ENV.clients_pipe.len - 1, client_sock };

        if (pthread_create(&thread_id, NULL, client_tcp_handler, (void*)&args) == -1) {
            perror("pthread_create");
            continue;
        }
        
        if (pipe(ENV.clients_pipe.pipe_arr[ENV.clients_pipe.len - 1]) == -1) {
            pthread_cancel(thread_id);
        }
        else {
            tcp_non_block(ENV.clients_pipe.pipe_arr[ENV.clients_pipe.len - 1][0]);
            FD_SET(ENV.clients_pipe.pipe_arr[ENV.clients_pipe.len - 1][0], &read_set);
            ENV.clients_pipe.tid_arr[ENV.clients_pipe.len - 1] = thread_id;
            pthread_detach(thread_id);
        }
        
    }

    shutdown(server_sock, SHUT_RDWR);
}

void *client_tcp_handler(void *args) {
    struct client_args *argsp = (struct client_args*)args;

    pthread_t tid = pthread_self();
    printf("[Client connected]: thread %ld\n", tid);
    fflush(stdout);

    sock_fd_t client_sock = argsp->sock;

    if (pth_auth(client_sock) == 0)
        chat_manager(client_sock, argsp->idx);

    shutdown(client_sock, SHUT_RDWR);

    return NULL;
}

void create_chats_process() {
    DIR *dir_ptr;
    struct dirent *direntp;
    int chats_cnt = 0, len = 0;
    char *room_name;

    if ((dir_ptr = opendir("chats")) == NULL) {
        fprintf(stderr, "cannot open chats directory\n");
        exit(1);
    }

    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (strstr(direntp->d_name, ".json") != NULL) {
            chats_cnt++;
            len = strlen(direntp->d_name) - 5;
            room_name = (char*)malloc(sizeof(char) * len);

            strncpy(room_name, direntp->d_name, len);

            string_arr_append(&ENV.child_names, room_name);
        }
    }

    closedir(dir_ptr);

    ENV.child_pids = (int*)malloc(sizeof(int) * chats_cnt);
    ENV.child_ports = (int*)malloc(sizeof(int) * chats_cnt);

    for (int i = 0 ; i < chats_cnt ;) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        else if (pid == 0) {
            ENV.child_pids[i] = getpid();
            ENV.child_ports[i] = DEFAULT_PORT + (i + 1);
            
            child_server(DEFAULT_PORT + (i + 1));

            exit(0);
        }
        else i++;
    }
}