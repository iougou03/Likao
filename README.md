
# architecture

## server file tree

    main.c

    usr/ : user 정보를 저장

    chats/ : 채팅방 정보를 저장

    logs/ : 채팅방 log를 저장

## client file tree

    main.c

## Data structure

```c
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
```

### process

![image](https://user-images.githubusercontent.com/53176902/234831911-071c449d-1536-472c-af76-5bf4e1110274.png)


Server는 main process에서 client 1:1 대응으로 thread 생성
    thread는  접속, 채팅방 참가 및 나가기 는 TCP 통신

Server는 main process에서 chat방 1:1 대응으로 process 생성
    fork 된 chatting은 main에서 받은 udp socket fd를 이용
        echo기능
        접속 echo기능
    client는 udp 로 채팅

#### 유의사항

* 서버에 채팅 log저장시키기

* 한 클라이언트의 메세지를 저장하는 동안에는 다른 클라이언트 채팅 불가

## dependencies

* [json-c](https://github.com/json-c/json-c)

* [gtk+-3](https://docs.gtk.org/gtk3/)

## in Debugging

password, name을 길게 입력하는 error handling