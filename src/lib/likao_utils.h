#ifndef HEADER_LIKAO_UTILS
#define HEADER_LIKAO_UTILS

struct string_arr_t {
    char** data;
    int len;
};

void string_arr_append(struct string_arr_t* arr, char* str);

void string_arr_remove(struct string_arr_t* arr, char* str);

void string_arr_free(struct string_arr_t* arr);

void dynamic_string_copy(char **destp, char *src);

int send_dynamic_data_tcp (sock_fd_t fd, void* data);

int recv_dynamic_data_tcp(sock_fd_t fd, char **buffer);

void clean_socket_buffer(sock_fd_t fd);

void tcp_block(sock_fd_t);

void tcp_non_block(sock_fd_t);


#endif