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

    tcp_non_block(server_sock);

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

    while (1) {
        fflush(stdout);
        client_size = sizeof(client_addr);
        
        for (int i = 0 ; i < ENV.clients_pipe.len ; i++) {
            struct pipe_msg_t pmsg;
            int read_fd = ENV.clients_pipe.pipe_arr[i][0];
            int bytes = read(read_fd, &pmsg, sizeof(struct pipe_msg_t));

            if (bytes > 0) {
                if (pmsg.type == CREATE_CHILD) {
                    // ENV.child_pids = (int*)malloc(sizeof(int) * ENV.childs_cnt);
                    // ENV.child_ports = (int*)malloc(sizeof(int) * ENV.childs_cnt);

                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork");
                    }
                    else if (pid == 0) {
                        child_server(pmsg.port);
                        exit(0);
                    }
                    break;
                }
            }
        }

        if ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_size)) == -1) {
            continue;
        }

        int plen = ENV.clients_pipe.len;
        ENV.clients_pipe.pipe_arr = realloc(ENV.clients_pipe.pipe_arr, sizeof(int**) * (plen + 1));
        ENV.clients_pipe.pipe_arr[plen] = (int*)malloc(sizeof(int) * 2);
        ENV.clients_pipe.tid_arr = realloc(ENV.clients_pipe.tid_arr, sizeof(int) * (plen + 1));
        ENV.clients_pipe.len++;
        struct client_args args = { plen, client_sock };

        if (pthread_create(&thread_id, NULL, client_tcp_handler, (void*)&args) == -1) {
            perror("pthread_create");
            continue;
        }
        pthread_detach(thread_id);
        
        if (pipe(ENV.clients_pipe.pipe_arr[plen]) == -1) {
            pthread_cancel(thread_id);
        }
        else {
            tcp_non_block(ENV.clients_pipe.pipe_arr[plen][0]);
            ENV.clients_pipe.tid_arr[plen] = thread_id;
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
    int len = 0;
    char *room_name;

    if ((dir_ptr = opendir("chats")) == NULL) {
        fprintf(stderr, "cannot open chats directory\n");
        exit(1);
    }

    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (strstr(direntp->d_name, ".json") != NULL) {
            ENV.childs_cnt++;
            len = strlen(direntp->d_name) - 5;
            room_name = (char*)malloc(sizeof(char) * len);

            strncpy(room_name, direntp->d_name, len);

            string_arr_append(&ENV.child_names, room_name);
        }
    }

    closedir(dir_ptr);

    ENV.child_pids = (int*)malloc(sizeof(int) * ENV.childs_cnt);
    ENV.child_ports = (int*)malloc(sizeof(int) * ENV.childs_cnt);

    for (int i = 0 ; i < ENV.childs_cnt ;) {
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