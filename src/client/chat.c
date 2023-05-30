#include <stdio.h>
#include <json-c/json.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./utils.h"

void print_chat_list(struct json_object **chat_list_arrp, sock_fd_t server_sock) {
    char *msg_raw = NULL;
    recv_dynamic_data_tcp(server_sock, &msg_raw);
    *chat_list_arrp = json_tokener_parse(msg_raw);

    free(msg_raw);

    int len = json_object_array_length(*chat_list_arrp);

    for (int i = 0 ; i < len ; i++) {
        struct json_object *elem = json_object_array_get_idx(*chat_list_arrp, i);
        printf("[chat %d]: %s\n", i, json_object_get_string(elem));
    }
}

void struct_to_json_chat(struct json_object *j_obj, struct to_server_chat_msg_t msg) {
    json_object_object_add(j_obj, "type", json_object_new_int(msg.type));
    json_object_object_add(j_obj, "name", json_object_new_string(msg.name));
    json_object_object_add(j_obj, "chat_room_name", json_object_new_string(msg.chat_room_name));
}

void chat_program(sock_fd_t server_sock, struct user_t user) {
    // int is_entered = -1;

    // while (1) {
    //     if (is_entered == 0) {

    //     }
        // else {
    //     }   
    // }

    struct json_object *chat_list_arr;
    print_chat_list(&chat_list_arr, server_sock);

    int mode;

    printf("want to join chat room? or create new one (1: join, 2: create)\n>");
    scanf("%d", &mode);

    if (mode == 1) {

        int room_num;
        printf("choose chat room number to enter: ");
        scanf("%d", &room_num);

        if (0 <= room_num && room_num < json_object_array_length(chat_list_arr)) {
            struct to_server_chat_msg_t msg;
            msg.type = JOIN;
            char *room_name = (char*)json_object_get_string(json_object_array_get_idx(chat_list_arr, room_num));
            dynamic_string_copy(&msg.chat_room_name, room_name);
            dynamic_string_copy(&msg.name, user.name);

            struct json_object *send_obj = json_object_new_object();
            struct_to_json_chat(send_obj, msg);

            send_dynamic_data_tcp(server_sock, (void*)json_object_get_string(send_obj));

            free(msg.chat_room_name);
            free(msg.name);
            json_object_put(send_obj);
        }
        else {
            // flag = -1;
            printf("wrong numbering");
        }
    }
    else if (mode == 2) {
        char room_name[BUFSIZ];
        printf("type chat room name: ");
        scanf("%s", room_name);

        // ---- sending create request ----
        struct to_server_chat_msg_t msg;
        msg.type = CREATE;
        dynamic_string_copy(&msg.chat_room_name, room_name);
        dynamic_string_copy(&msg.name, user.name);

        struct json_object *send_obj = json_object_new_object();
        struct_to_json_chat(send_obj, msg);

        send_dynamic_data_tcp(server_sock, (void*)json_object_get_string(send_obj));

        free(msg.chat_room_name);
        free(msg.name);
        json_object_put(send_obj);
        // --------------------------------

    }
    // else flag = -1;

    json_object_put(chat_list_arr);
}
