#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <gtk/gtk.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./utils.h"
#include "./appwindow.h"
#include "./chat.h"

struct user_t auth_userg = { NULL, NULL };

int get_input(struct user_t *userp) {
    int type, flag;
    printf("choose mode (1 = sign in / 2 = sign up)\n>");
    fflush(stdout);
    scanf("%d", &type);

    if (type == 1) {
        flag = SIGN_IN;
    }
    else if (type == 2) {
        flag = SIGN_UP;
    }
    else {
        flag = -1;
    }

    if (flag == SIGN_IN || flag == SIGN_UP) {
        char input[BUFSIZ];
        printf("type name: ");
        scanf("%s", input);
        
        userp->name = (char*)malloc(sizeof(char) * (strlen(input) + 1));
        strcpy(userp->name, input);
        userp->name[strlen(input)] = '\0';

        printf("type password: ");
        scanf("%s", input);
        
        userp->password = (char*)malloc(sizeof(char) * (strlen(input) + 1));
        strcpy(userp->password, input);
        userp->password[strlen(input)] = '\0';
    }

    return flag;
}

void struct_to_json_auth(struct json_object *j_obj, struct to_server_auth_msg_t msg) {
    json_object_object_add(j_obj, "type", json_object_new_int(msg.type));

    json_object_object_add(j_obj, "name", json_object_new_string(msg.name));
    json_object_object_add(j_obj, "password", json_object_new_string(msg.password));
}

void json_to_struct_auth(struct json_object *j_obj, struct to_client_auth_msg_t *msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0) {
            msgp->type = json_object_get_int(val);
        }
        else if (strcmp(key, "message") == 0) {
            char *message = (char*)json_object_get_string(val);
            dynamic_string_copy(&(msgp->message), message);
        }
    }
}


void submit_clicked(GtkButton *button, gpointer user_data) {
    sock_fd_t server_sock = GPOINTER_TO_INT(user_data);

    struct to_server_auth_msg_t msg = { -1, NULL, NULL };

    GtkEntry *id_entry = GTK_ENTRY(gtk_builder_get_object(builder, "id_entry"));
    GtkEntry *password_entry = GTK_ENTRY(gtk_builder_get_object(builder, "password_entry"));
    
    GtkWidget *sign_up_radio = GTK_WIDGET(gtk_builder_get_object(builder, "sign_up_radio"));
    GSList *group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sign_up_radio));
    GSList *iter = group;
    
    while (iter != NULL) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iter->data))) {
            const gchar *label = gtk_button_get_label(GTK_BUTTON(iter->data));
            
            if (strcmp(label, "sign up") == 0) {
                msg.type = SIGN_UP;
            }
            else if (strcmp(label, "sign in") == 0) {
                msg.type = SIGN_IN;
            }
            break;
        }
        
        iter = g_slist_next(iter);
    }

    const gchar *id = gtk_entry_get_text(id_entry);
    const gchar *password = gtk_entry_get_text(password_entry);
    
    dynamic_string_copy(&msg.name, (char*)id);
    dynamic_string_copy(&msg.password, (char*)password);
    
    struct json_object *msg_obj = json_object_new_object();

    struct_to_json_auth(msg_obj, msg);
    send_dynamic_data_tcp(server_sock, (void*)json_object_get_string(msg_obj));

    free(msg.name);
    free(msg.password);
    json_object_put(msg_obj);

    dynamic_string_copy(&(auth_userg.name), (char*)id);
    dynamic_string_copy(&(auth_userg.password), (char*)password);
}

void *async_recv_pth(void* args) {
    int auth_success = 0;

    sock_fd_t server_sock = *((sock_fd_t*)args);

    while (!auth_success) {
        sleep(1);

        char *buffer = NULL;
        if (recv_dynamic_data_tcp(server_sock, &buffer) == -1)
            continue;
        
        struct json_object *msg_obj = json_tokener_parse(buffer);

        if (msg_obj == NULL) continue;

        struct json_object *recv_obj = json_tokener_parse(buffer);
        struct to_client_auth_msg_t recv_msg = { -1, NULL };

        json_to_struct_auth(recv_obj, &recv_msg);
        printf("%s\n", json_object_get_string(recv_obj));
        if (recv_msg.type == SUCCESS) {
            auth_success = 1;
        }
        
        clean_socket_buffer(server_sock);
        
        if (recv_msg.message != NULL)
            free(recv_msg.message);

        free(buffer);
        json_object_put(msg_obj);
    }

    tcp_block(server_sock);

    struct json_object *msg_obj = json_tokener_parse("{ \"status\":\"complete\"}");
    send_dynamic_data_tcp(server_sock, (void*)json_object_get_string(msg_obj));

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "page2");

    printf("finally, %s %s\n", auth_userg.name, auth_userg.password);
    // chat_program(server_sock, user);

    pthread_exit(NULL);
}

void auth(sock_fd_t *server_sockp, struct user_t *userp) {
    GtkWidget *submit_button = GTK_WIDGET(gtk_builder_get_object(builder, "submit_button"));

    tcp_non_block(*server_sockp);

    g_signal_connect(submit_button, "clicked", G_CALLBACK(submit_clicked), GINT_TO_POINTER(*server_sockp));

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, async_recv_pth, (void*)server_sockp) == -1) {
        perror("pthread_create, please reopen program");
    }
    else {
        pthread_detach(thread_id);
    }
}
