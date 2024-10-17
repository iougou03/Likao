# process

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


## How to

1. dependencies 깔기

    ```bash
    sudo apt install gcc
    sudo apt install git
    sudo apt install make
    sudo apt install libjson-c-dev
    sudo apt install libgtk-3-dev
    ```

1. 서버로 작동시킬 머신의 IP주소를 ./src/dist/main.c 내 아래 줄에 입력할 것
    
    ```c
    72 |    dynamic_string_copy(&SERVER_IP_ADDRESS, /* IP_ADDRESS */);
    ```

2. make 로 빌드 (dist folder 생성)

3. server machine 에서 dist/server/ 로 cd한 후 ./server 실행

4. client machine 에서 dist/client/ 로 cd한 후 ./cliet 실행


### features
    
1. 채팅서비스 가입 및 접속

2. 채팅방 접속 및 채팅


#### etc.

total code lines: 2878 (running git ls-files | xargs wc -)
