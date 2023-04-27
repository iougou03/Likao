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

#include "../lib/chat.h"

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
}

void server() {
    int socket_fd;
    int client_socket_fd;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;

    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error while creating socket, plz try again later");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error while binding socket with address\n");
        exit(1);
    }

    if (listen(socket_fd, 40) == -1) {
        printf("listening mode failed!\n");
        exit(1);
    }

    printf("The server has opened at port %d!\n", DEFAULT_PORT);

    while (1) {
        client_size = sizeof(client_addr);

        if ((client_socket_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_size)) == -1) {
            perror("Error while accepting client!\n");
            exit(1);
        }

        printf("[Client conencted]\n");
    }
}


int main (int argc, char** argv) {
    init(argv[0]);

    server();

    return 0;
}
