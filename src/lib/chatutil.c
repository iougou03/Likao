#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "chat.h"

#define CHUNK_SIZE 1024

int send_dynamic_data_tcp (sock_fd sockfd, const void* data) {
    char buffer[CHUNK_SIZE];
    size_t len = strlen(data);
    
    int offset,
        chunk_len,
        num_chunks = len / CHUNK_SIZE + 1;

    for (int i = 0 ; i < num_chunks ; i++) {
        offset = i * CHUNK_SIZE;
        chunk_len = CHUNK_SIZE;

        if (offset + chunk_len > len) {
            chunk_len = len - offset;
        }

        char chunk[CHUNK_SIZE + 1];
        strncpy(chunk, data + offset, chunk_len);
        chunk[chunk_len] = '\0';

        if (send(sockfd, chunk, sizeof(char) * strlen(chunk), 0) == -1) {
            perror("error while sending");
            return -1;
        }
    }

    return 0;
}

char* recv_dynamic_data_tcp (sock_fd sockfd) {
    char* buffer = NULL;
    int bytes_recv,
        chunk_size = CHUNK_SIZE;
    long int total_size = 0;

    while (1) {
        buffer = realloc(buffer, total_size + chunk_size);
        bytes_recv = recv(sockfd, buffer + total_size, chunk_size, 0);

        if (bytes_recv == -1) {
            perror("error in recv");
            free(buffer);
            return NULL;
        }
        else if (bytes_recv == 0) {
            break;
        }

        total_size += bytes_recv;
        if (bytes_recv < chunk_size) {
            break;
        }
    }

    buffer = realloc(buffer, total_size + 1);
    buffer[total_size] = '\0';

    return buffer;
}