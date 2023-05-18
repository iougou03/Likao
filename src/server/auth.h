#include "../lib/chat.h"

#ifndef HEADER_FILE_SERVER_AUTH
#define HEADER_FILE_SERVER_AUTH

int sign_in(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]);

int sign_up(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]);

int pth_auth(pthread_t tid, sock_fd client_sock_fd);

#endif