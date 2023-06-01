#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <gtk/gtk.h>
#include <signal.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./utils.h"
#include "./chat.h"
#include "./appwindow.h"

struct user_t auth_userg = { NULL, NULL };
int auth_thread_running = 0, server_sockg;
pthread_t main_thread;

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

void on_button_enter(GtkWidget *button, gpointer user_data) {
    GdkDisplay *display = gdk_display_get_default();
    if (!auth_thread_running) {
        GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_HAND1);
        gdk_window_set_cursor(gtk_widget_get_window(button), cursor);
    }
    else {
        GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_X_CURSOR);
        gdk_window_set_cursor(gtk_widget_get_window(button), cursor);
    }
}

void on_button_leave(GtkWidget *button, gpointer user_data) {
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_LEFT_PTR);
    gdk_window_set_cursor(gtk_widget_get_window(button), cursor);
}

void submit_clicked(GtkButton *button, gpointer user_data) {
    if (auth_thread_running == 1) return;

    sock_fd_t server_sock = GPOINTER_TO_INT(user_data);

    struct to_server_auth_msg_t msg = { -1, NULL, NULL };

    GtkEntry *id_entry = GTK_ENTRY(gtk_builder_get_object(builderg, "id_entry"));
    GtkEntry *password_entry = GTK_ENTRY(gtk_builder_get_object(builderg, "password_entry"));
    
    GtkWidget *sign_up_radio = GTK_WIDGET(gtk_builder_get_object(builderg, "sign_in_radio"));
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

    dynamic_string_copy(&(auth_userg.name), (char*)id);
    dynamic_string_copy(&(auth_userg.password), (char*)password);

    if (msg.name != NULL)
        free(msg.name);
    if (msg.password != NULL)
        free(msg.password);
    json_object_put(msg_obj);

    auth_thread_running = 1;
}

void *async_recv_pth(void* args) {
    int auth_close = 0;
    sock_fd_t server_sock = *((sock_fd_t*)args);

    while (auth_close == 0) {
        if (!auth_thread_running) continue;

        char *buffer = NULL;
        if (recv_dynamic_data_tcp(server_sock, &buffer) == -1) {
            sleep(1);
            continue;
        }

        struct json_object *recv_obj = json_tokener_parse(buffer);
        
        if (recv_obj != NULL) {
            struct to_client_auth_msg_t recv_msg = { -1, NULL };

            json_to_struct_auth(recv_obj, &recv_msg);

            if (recv_msg.type == SUCCESS) {
                auth_close = 1;
            }
            else {
                g_print("auth failed\n");
            }

            auth_thread_running = 0;

            struct json_object *check_msg_obj = json_tokener_parse("{\"status\":\"complete\"}");
            send_dynamic_data_tcp(server_sock, (void*)json_object_get_string(check_msg_obj));
            
            // if (recv_msg.message != NULL) free(recv_msg.message);
            
            json_object_put(check_msg_obj);
        
            free(buffer);
            json_object_put(recv_obj);
    
            clean_socket_buffer(server_sock);
        }
    }    
    pthread_kill(main_thread, SIGUSR1);

    pthread_exit(NULL);
}

void auth_thread_done_callback(int signum) {
    tcp_block(server_sockg);
    chat_program(server_sockg, auth_userg);
    // free(auth_userg.name);
    // free(auth_userg.password);
}

void auth(sock_fd_t *server_sockp, struct user_t *userp) {
    main_thread = pthread_self();
    server_sockg = *server_sockp;

    gtk_stack_set_visible_child_name(GTK_STACK(stackg), "page1");

    GtkWidget *submit_button = GTK_WIDGET(gtk_builder_get_object(builderg, "submit_button"));

    tcp_non_block(*server_sockp);

    g_signal_connect(submit_button, "clicked", G_CALLBACK(submit_clicked), GINT_TO_POINTER(*server_sockp));
    g_signal_connect(submit_button, "enter-notify-event", G_CALLBACK(on_button_enter), NULL);
    g_signal_connect(submit_button, "leave-notify-event", G_CALLBACK(on_button_leave), NULL);

    signal(SIGUSR1, auth_thread_done_callback);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, async_recv_pth, (void*)server_sockp) == -1) {
        perror("pthread_create, please reopen program");
    }
    else {
        pthread_detach(thread_id);
    }
}
