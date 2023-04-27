server
-> main.c
-> usr/ : user 정보를 저장
-> chats/ : 채팅방 정보를 저장
-> logs/ : 채팅방 log를 저장

## Data structure

```c
#define MAX_TYPE_LEN 512

struct user_t {
    char name;
    char password;
}

struct chat_t {
    user_t users[];
}

#define MAX_MSG_LEN 4096

struct chat_msg {
    char msg[MAX_MSG_LEN];
    time_t created_at;
}

struct chat_logs {
    messages chat_msg[];
}
```


### process

1. client는 sign in, sign up 고름


* 서버에 채팅 log저장시키기


* 한 클라이언트의 메세지를 저장하는 동안에는 다른 클라이언트 채팅 불가