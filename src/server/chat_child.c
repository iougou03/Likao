#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../lib/likao_chat.h"

void child_server(int port) {
    sock_fd_t child_socket_fd;
    struct sockaddr_in server_addr, client_addr;

    child_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (child_socket_fd == -1) {
        perror("socket");
        return -2;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(child_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))== -1) {
        perror("bind");
        return -2;
    }

    printf("child UDP server open at port %d\n",port);

    // while (1) {

    // }

    close(child_socket_fd);
}