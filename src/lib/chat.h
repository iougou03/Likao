#include <openssl/sha.h>

#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#define DEFAULT_PORT 5500

#define NAME_MAX_LEN 512
#define PASSWORD_MAX_LEN 512

struct user_t
{
    char name[NAME_MAX_LEN];
    char password[SHA256_DIGEST_LENGTH * 2 + 1];
};

#endif