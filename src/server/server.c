#include <stdio.h>
#include <json-c/json.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../lib/likao_chat.h"
#include "./auth.h"
#include "./chat.h"
#include "./globals.h"

void server_on() {
    sock_fd_t server_sock, client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    void *client_tcp_handler();

    if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_sock, 40) == -1) {
        perror("listen");
        exit(1);
    }

    printf("The server has opened at port %d\n", DEFAULT_PORT);
    fflush(stdout);

    pthread_t thread_id;

    while (1) {
        client_size = sizeof(client_addr);

        if ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_size)) == -1) {
            perror("accept");
            continue;
        }

        if (pthread_create(&thread_id, NULL, client_tcp_handler, (void*)&client_sock) == -1) {
            perror("pthread_create");
            continue;
        }
        
        pthread_detach(thread_id);
    }

    shutdown(server_sock, SHUT_RDWR);
}

void *client_tcp_handler(void *client_sockp) {
    pthread_t tid = pthread_self();
    printf("[Client connected]: thread %ld\n", tid);
    fflush(stdout);

    sock_fd_t client_sock = *((int*)client_sockp);

    int flag;
    while ((flag = pth_auth(client_sock)) == -1);

    if (flag == 0)
        chat_manager(client_sock);

    shutdown(client_sock, SHUT_RDWR);

    return NULL;
}