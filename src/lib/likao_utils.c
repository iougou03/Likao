#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "./likao_chat.h"
#include "./likao_utils.h"

#define CHUNK_SIZE 1024

void string_arr_append(struct string_arr_t* arr, char* str) {
    int str_size = strlen(str) * sizeof(char);
    char* copy = (char*)malloc(str_size);
    strcpy(copy, str);

    arr->data = realloc(arr->data, sizeof(char**) * (arr->len + 1));

    arr->data[arr->len++] = copy;
}

void string_arr_remove(struct string_arr_t* arr, char* str) {
    int find = 0;

    for (int i = 0 ; i < arr->len ; i++) {
        if (!find && strcmp(arr->data[i], str) == 0) {
            free(arr->data[i]);

            find = i;
        }
        else if (find) {
            arr->data[i - 1] = arr->data[i];
        }
    }
    arr->len--;
}

void string_arr_free(struct string_arr_t* arr) {
    for (int i = 0 ; i < arr->len ; i++) {
        free(arr->data[i]);
    }

    free(arr->data);
}

void dynamic_string_copy(char **destp, char *src) {
    *destp = NULL;
    char *temp = realloc(*destp, sizeof(char) * (strlen(src) + 1));
    if (temp == NULL) {
        fprintf(stderr, "Failed to allocate memory for string copy\n");
        return;
    }
    *destp = temp;
    strcpy(*destp, src);
    (*destp)[strlen(src)] = '\0';
}


int send_dynamic_data_tcp (sock_fd_t fd, void* data) {
    printf("send_dynamic_data_tcp %s\n",(char*)data);
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

        if (send(fd, chunk, sizeof(char) * strlen(chunk), 0) == -1) {
            return -1;
        }
    }

    return 0;
}

/**
 * buffer는 반드시 NULL로 초기화 되어있어야 한다
*/
int recv_dynamic_data_tcp(sock_fd_t fd, char **buffer) {
    int bytes_recv,
        chunk_size = CHUNK_SIZE;
    long int total_size = 0;
    printf("recv_dynamic_data_tcp\n");
    while (1) {
        *buffer = realloc(*buffer, total_size + chunk_size);
        bytes_recv = recv(fd, *buffer + total_size, chunk_size, 0);

        if (bytes_recv == -1) {
            if (total_size > 0) 
                free(*buffer);
            return -1;
        }
        else if (bytes_recv == 0) 
            break;
        
        total_size += bytes_recv;
        if (bytes_recv < chunk_size)
            break;
    }
    
    if (total_size > 0) {
        *buffer = realloc(*buffer, total_size + 1);
        (*buffer)[total_size] = '\0';
        printf("recv_dynamic_data_tcp %s\n", *buffer);
        
        return total_size;
    }
    else return -1;

}

void clean_socket_buffer(sock_fd_t sock) {
    char buffer[CHUNK_SIZE];
    ssize_t bytes_received;
    do {
        bytes_received = recv(sock, buffer, CHUNK_SIZE, 0);
    } while (bytes_received > 0);
}

void tcp_block(sock_fd_t sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
}

void tcp_non_block(sock_fd_t sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}