#include <openssl/sha.h>

#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#define DEFAULT_PORT 5500

#define NAME_MAX_LEN 512
#define PASSWORD_MAX_LEN 512

struct user_t
{
    char name[NAME_MAX_LEN];
    // char password[SHA256_DIGEST_LENGTH * 2 + 1]
    char password[PASSWORD_MAX_LEN];
};

typedef const enum {
    SIGN_IN,
    SIGN_UP
} msg_client_type;

struct msg_from_client_t
{
    msg_client_type type;
    char name[NAME_MAX_LEN];
    char password[PASSWORD_MAX_LEN];
};

typedef const enum {
    SUCCESS,
    FAILED
} msg_server_type;

struct msg_from_server_t
{
    msg_server_type type;
    char* msg;
};

#endif