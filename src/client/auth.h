#include "../lib/likao_chat.h"
#include "./utils.h"

#ifndef HEADER_CLIENT_AUTH
#define HEADER_CLIENT_AUTH

void auth(sock_fd_t *server_sock, struct user_t *userp);

#endif