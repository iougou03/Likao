
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

![image](https://user-images.githubusercontent.com/53176902/234831911-071c449d-1536-472c-af76-5bf4e1110274.png)



#### 유의사항

* 서버에 채팅 log저장시키기

* 한 클라이언트의 메세지를 저장하는 동안에는 다른 클라이언트 채팅 불가

## dependencies

* [json-c](https://github.com/json-c/json-c)

* [gtk+-3](https://docs.gtk.org/gtk3/)

## in Debugging

password, name을 길게 입력하는 error handling