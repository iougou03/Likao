
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <json-c/json.h>

#include "../lib/chat.h"

void input_ui(char *name, char *password);
sock_fd connect_to_server(char* server_ip);
const char* struct_to_json(struct json_object* j_obj, struct msg_from_client_t clnt);
void json_to_struct(struct json_object* j_obj, struct msg_from_server_t* msgp);
void sign_in(int sockfd);
void sign_up(int sockfd);

int main(int argc,char **argv){
    sock_fd sockfd;
    int sign_num;

    sockfd = connect_to_server("127.0.0.1");

        //sign in/ sign up
    printf("1 : sign in / 2 : sign up\n");
    scanf("%d", &sign_num);

    if(sign_num == 1){ //sign in
        sign_in(sockfd);
    }
    else if (sign_num == 2) { 
        sign_up(sockfd);
    }
    
    close(sockfd);

    return 0;
}

void input_ui(char *name, char *password) {
    printf("name : ");
    scanf("%s", name);
    printf("password : ");
    scanf("%s", password);
}

sock_fd connect_to_server(char* server_ip) {
    sock_fd sockfd;

    struct sockaddr_in serv_addr;
    
    // server 연결
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("socket");
        exit(1);
	}
    else {
		printf("socket ok\n");
	}

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(DEFAULT_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
	    perror("connect error, try again");
        exit(1);
    }
    else {	
	    printf("connection success\n");
	}
    
    return sockfd;
}

const char* struct_to_json(struct json_object* j_obj, struct msg_from_client_t clnt) {
    json_object_object_add(j_obj, "type", json_object_new_int(clnt.type));

    if (clnt.type == SIGN_IN || clnt.type == SIGN_UP) {
        json_object_object_add(j_obj, "name", json_object_new_string(clnt.name));
        json_object_object_add(j_obj, "password", json_object_new_string(clnt.password));
    }
}

void json_to_struct(struct json_object* j_obj, struct msg_from_server_t* msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0)
            msgp->type = json_object_get_int(val);

        else if (strcmp(key, "msg") == 0)
            strcpy(msgp->msg, json_object_get_string(val));
    }
}

void sign_up(int sockfd) {
    struct msg_from_client_t msg_client;
    struct json_object *user_json_obj = json_object_new_object();

    msg_client.type = SIGN_UP;
    input_ui(msg_client.name, msg_client.password);

    struct_to_json(user_json_obj, msg_client);

    const char *msg = json_object_to_json_string(user_json_obj);

    if (send(sockfd, msg, sizeof(struct msg_from_client_t), 0) == -1) {
        perror("send error");
        exit(EXIT_FAILURE);
    }

    json_object_put(user_json_obj);

    void* received_msg_raw = malloc(sizeof(struct msg_from_server_t));
    if (recv(sockfd, received_msg_raw, sizeof(struct msg_from_server_t), 0) == -1) {
        fprintf(stderr, "Error, try again");
        exit(EXIT_FAILURE);
    }

    struct msg_from_server_t msg_server;
    struct json_object *received_msg_json = json_tokener_parse((char*) received_msg_raw);
    json_to_struct(received_msg_json, &msg_server);

    // -------need to remove---------
    printf("Received data %d %s\n",msg_server.type, msg_server.msg);
    // ------------------------------

    if (msg_server.type == SUCCESS)
        printf("Sign UP!");
    else
        printf("Sign up failed");
    

    json_object_put(received_msg_json);
    free(received_msg_raw);
}


void sign_in(int sockfd)
{
    struct msg_from_client_t msg_client;
    struct json_object *user_json_obj = json_object_new_object();

    msg_client.type = SIGN_IN;
    input_ui(msg_client.name, msg_client.password);

    struct_to_json(user_json_obj, msg_client);

    const char* msg = json_object_to_json_string(user_json_obj);

    if (write(sockfd, msg, sizeof(struct msg_from_client_t)) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    json_object_put(user_json_obj);

    void* received_msg_raw = malloc(sizeof(struct msg_from_server_t));
    // 서버 응답받기
    if (recv(sockfd, received_msg_raw, sizeof(struct msg_from_server_t), 0) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    struct msg_from_server_t msg_server;
    struct json_object *received_msg_json = json_tokener_parse(received_msg_raw);
    json_to_struct(received_msg_json, &msg_server);

    if (msg_server.type == SUCCESS) {
        printf("Login success!\n");
    }
    else {
        printf("Login fail\n");
    }

    json_object_put(received_msg_json);
    free(received_msg_raw);
}
