#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "chat.h"

#ifndef HEADER_FILE_CHATUTIL
#define HEADER_FILE_CHATUTIL

int send_dynamic_data_tcp (sock_fd sockfd, const void* data);

char* recv_dynamic_data_tcp (sock_fd sockfd);

#endif