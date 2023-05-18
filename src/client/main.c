
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
#include "../lib/chatutil.h"



void input_ui(char *name, char *password);
sock_fd connect_to_server(char* server_ip);
const char* struct_to_json(struct json_object* j_obj, void* clnt);
void json_to_struct(struct json_object* j_obj, struct msg_from_server_t* msgp);
int sign_in(int sockfd, struct user_t* userp);
int sign_up(int sockfd, struct user_t* userp);
void print_chat_list(struct json_object* j_obj);
int make_chats_list(int sockfd, char* user_name);
int join_chat_room(int sockfd, char* user_name);

int main(int argc,char **argv){
    sock_fd sockfd;
    struct user_t user_info;
    int sign_num, state = -1;
    int chat_num;

    sockfd = connect_to_server("127.0.0.1");

        //sign in/ sign up
    printf("1 : sign in / 2 : sign up\n>");
    scanf("%d", &sign_num);

    if(sign_num == 1){ //sign in
        state = sign_in(sockfd, &user_info);
    }
    else if (sign_num == 2) { 
        state = sign_up(sockfd, &user_info);
    }

    if (state == 0) {
        char* recv_mem = recv_dynamic_data_tcp(sockfd);
        struct json_object* msg_j_obj;

        if ((msg_j_obj = json_tokener_parse(recv_mem)) == NULL) {
            perror("error while convert recv data");
        }

        print_chat_list(msg_j_obj);

        json_object_put(msg_j_obj);
        free(recv_mem);

        printf("1: Create the chat room / 2: Join the chat room\n>");
        scanf("%d", &chat_num);

        if (chat_num == 1) { // Create the chat room
            state = make_chats_list(sockfd, user_info.name);
        }
        else if (chat_num == 2) { // Enter the chat room
            state = join_chat_room(sockfd, user_info.name);
        }


        char* msg_raw = recv_dynamic_data_tcp(sockfd);
        struct json_object * server_msg_j_obj = json_tokener_parse(msg_raw);
        struct msg_from_server_t server_msg;

        json_to_struct(server_msg_j_obj, &server_msg);

        printf("%d %s\n", server_msg.type, server_msg.msg);

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

const char* struct_to_json(struct json_object* j_obj, void *clnt) {
    struct msg_form_t* p = (struct msg_form_t*) clnt;

    json_object_object_add(j_obj, "type", json_object_new_int(p->type));

    if (p->type == SIGN_IN || p->type == SIGN_UP) {
        struct msg_from_client_t *msgcp = (struct msg_from_client_t*) clnt;

        json_object_object_add(j_obj, "name", json_object_new_string(msgcp->name));
        json_object_object_add(j_obj, "password", json_object_new_string(msgcp->password));
    }
    else if (p->type == CREATE || p->type == JOIN) {
        struct chats_from_client_t *chats_client = (struct chats_from_client_t*) clnt;

        json_object_object_add(j_obj, "user_name", json_object_new_string(chats_client->user_name));
        json_object_object_add(j_obj, "room_name", json_object_new_string(chats_client->room_name));
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

int sign_up(int sockfd, struct user_t* userp) {
    struct msg_from_client_t msg_client;
    struct json_object *user_json_obj = json_object_new_object();

    msg_client.type = SIGN_UP;
    input_ui(msg_client.name, msg_client.password);

    struct_to_json(user_json_obj, &msg_client);

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

    int flag;
    if (msg_server.type == SUCCESS) {
        printf("Sign UP!\n");
        strcpy(userp->name, msg_client.name);
        flag = 0;
    }
    else {
        printf("Sign up failed\n");
        flag = -1;
    }

    json_object_put(received_msg_json);
    free(received_msg_raw);

    return flag;
}

int sign_in(int sockfd, struct user_t* userp)
{
    struct msg_from_client_t msg_client;
    struct json_object *user_json_obj = json_object_new_object();

    msg_client.type = SIGN_IN;
    input_ui(msg_client.name, msg_client.password);

    struct_to_json(user_json_obj, &msg_client);

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

    int flag;
    if (msg_server.type == SUCCESS) {
        flag = 0;
        strcpy(userp->name, msg_client.name);
    }
    else {
        flag = -1;
    }
    printf("%s\n", msg_server.msg);

    json_object_put(received_msg_json);
    free(received_msg_raw);

    return flag;
}

void print_chat_list(struct json_object *j_obj) {
    int array_len = json_object_array_length(j_obj);
    int i;

    for (i = 0; i < array_len; i++) {
        json_object *json_elem = json_object_array_get_idx(j_obj, i);

        // TODO: json extension 없애기
        printf("[chat %d]: %s\n", i + 1, json_object_get_string(json_elem));
    }
    printf("\n");
}

int make_chats_list(int sockfd, char* user_name) {
    struct chats_from_client_t chats_client;
    struct json_object *chats_json_obj = json_object_new_object();

    chats_client.type = CREATE;
    printf("Enter the room name to create: ");
    scanf("%s", chats_client.room_name);

    strcpy(chats_client.user_name, user_name);
    struct_to_json(chats_json_obj, &chats_client);

    const char* msg = json_object_to_json_string(chats_json_obj);

    send_dynamic_data_tcp(sockfd, msg);

    json_object_put(chats_json_obj);
}

int join_chat_room(int sockfd, char* user_name) {
    struct chats_from_client_t chats_client;
    struct json_object *chats_json_obj = json_object_new_object();

    chats_client.type = JOIN;
    printf("Enter the room name to join: ");
    scanf("%s", chats_client.room_name);

    strcpy(chats_client.user_name, user_name);
    struct_to_json(chats_json_obj, &chats_client);

    const char* msg = json_object_to_json_string(chats_json_obj);

    send_dynamic_data_tcp(sockfd, msg);

    json_object_put(chats_json_obj);
}