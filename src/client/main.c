
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <json-c/json.h>

#include "../lib/chat.h"

void sign_in();
void sign_up();
int connect_to_server();

int main(int argc,char **argv){

    int sign_num, sockfd;
    pthread_t rcv_thread;
    void* thread_result;
	struct user_t user;

    sockfd = connect_to_server("192.168.64.5");

    return 0;
}

int connect_to_server(char* server_ip) {
    int sockfd;

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
	    perror("connect");
    else {	
	    printf("connection success\n");
	}
    
    return sockfd;
}

const char* struct_to_json(struct msg_from_client_t clnt) {
    struct json_object *user_json_obj = json_object_new_object();

    json_object_object_add(user_json_obj, "name", json_object_new_string(clnt.name));
    json_object_object_add(user_json_obj, "password", json_object_new_string(clnt.password));
    json_object_object_add(user_json_obj, "type", json_objet_new_int(clnt.type));

    return json_object_to_json_string(user_json_obj);
}

void json_to_struct(struct json_object* j_obj, struct msg_from_server_t* msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0)
            msgp->type = json_object_get_int(val);

        else if (strcmp(key, "name") == 0)
            strcpy(msgp->msg, json_object_get_string(val));
    }
}

void sign_up(int sockfd) {
    struct msg_from_client_t msg_client;
    struct msg_from_server_t msg_server;
    struct json_object *user_json_obj = json_object_new_object();

    printf("Enter your name: ");
    scanf("%s", msg_client.name);
    printf("Enter your password: ");
    scanf("%s", msg_client.password);
    msg_client.type = SIGN_UP;

    const char *msg = struct_to_json(msg_client);
    send(sockfd, msg, sizeof(msg), 0);

    void* received_msg_raw = malloc(sizeof(struct msg_from_server_t));
    recv(sockfd, reveived_msg_raw, sizeof(struct msg_from_server_t), 0);
    struct json_object *j_obj = json_tokener_parse(received_msg_raw);
    jason_to_struct(j_obj, msg_server);
    if (msg_server.type == SUCCESS)
        printf("");
    else
        printf("");
}