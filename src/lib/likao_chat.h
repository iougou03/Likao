#ifndef HEADER_LIKAO_CHAT
#define HEADER_LIKAO_CHAT
#define DEFAULT_PORT 9999

typedef unsigned int sock_fd_t;

typedef enum {
    SIGN_IN,
    SIGN_UP,
    CREATE ,
    JOIN
} client_request_t;

struct to_server_auth_msg_t {
    client_request_t type;
    char *name;
    char *password;
};

struct to_server_chat_msg_t {
    client_request_t type;
    char *name;
    char *chat_room_name;
};

struct to_server_log_msg_t {

};

typedef enum {
    SUCCESS,
    FAILED
} server_respond_t;

struct to_client_auth_msg_t {
    server_respond_t type;
    char *message;
};

struct to_client_chat_msg_t {
    server_respond_t type;
    int chat_port;
};

#endif
