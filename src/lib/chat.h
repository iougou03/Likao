//#include <openssl/sha.h>

#ifndef HEADER_FILE_CHAT
#define HEADER_FILE_CHAT
#define DEFAULT_PORT 23345

#define NAME_MAX_LEN 512
#define PASSWORD_MAX_LEN 512

typedef unsigned int sock_fd;
struct user_t
{
    char name[NAME_MAX_LEN];
    // char password[SHA256_DIGEST_LENGTH * 2 + 1]
    char password[PASSWORD_MAX_LEN];
};

typedef enum {
    SIGN_IN=1,
    SIGN_UP=2,
    CREATE = 3,
    JOIN = 4
} msg_client_type;

struct msg_form_t {
    int type;
};

struct msg_from_client_t
{
    msg_client_type type;
    char name[NAME_MAX_LEN];
    char password[PASSWORD_MAX_LEN];
};

typedef enum {
    SUCCESS,
    FAILED
} msg_server_type;

#define MSG_MAX_LEN 4096
struct msg_from_server_t
{
    msg_server_type type;
    char msg[MSG_MAX_LEN];
};


struct chats_from_client_t {
    msg_client_type type;
    char user_name[NAME_MAX_LEN];
    char room_name[NAME_MAX_LEN];
};

#endif
