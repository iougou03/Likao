
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
    thread는  
        접속, 
        채팅방 생성,
        채팅방 참가 및 나가기 요청은 TCP 통신

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

* password, name을 길게 입력하는 error handling

* sign_in, sign_up error code마다 다른 메세지 내용 (more detailed error handling needed)

* sign in/up에서 write/send 에서 buffer size기준이 구조체로 되어있음. 
    
    원래는 json_object에서 변환된 string사이즈가 들어가야함

* sign in/up에서 dyanmic size인 구조체로 송수신하기

* server내 auth.c 코드 스타일 수정, 모든 error code에 대해서 send가 일어나야함

* chat.h 에서 client enum들 통합하기
    그에 따라 클라이언트 코드 수정

* 서버에서 pthead를 잘 지정해서 SIGPIPE를 핸들해줘야함