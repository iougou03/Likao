
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "../lib/chat.h"

void sign_in(void);


void sign_up(void);

int connect_to_server();

int main(int argc,char **argv){

    int sign_num, sockfd;
    pthread_t rcv_thread;
    void* thread_result;
	struct user_t user;

    sockfd = connect_to_server("192.168.64.5");

    //sign in/ sign pu
    printf("1 : sign in / 2 : sign up\n");
    scanf("%d", sign_num);

    //user_t user 구조체 정보 얻기
    printf("name : ");
    scanf("%s", user.name);
    //printf("password : ");
    //scanf("%s", &user.password);

    if(sign_num == 1){ //sign in
        
    }
    else{ 
        
    }

    return 0;
}

int connect_to_server(char* server_ip) {
    int sockfd;

    struct sockaddr_in serv_addr;
    
    /// server 연결
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket");
	}
    else{
		printf("socket ok\n");
	}

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port=htons(DEFAULT_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
	    perror("connect");
	}
    else{	
	    printf("connection success\n");
	}
    
    return sockfd;
}