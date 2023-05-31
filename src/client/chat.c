#include <stdio.h>
#include <unistd.h>
#include <json-c/json.h>
#include <gtk/gtk.h>
#include <pthread.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./utils.h"
#include "./appwindow.h"

int chat_thread_running = 0;
struct user_t chat_userg;
sock_fd_t chat_server_sockg;
pthread_t main_thread;

void struct_to_json_chat(struct json_object *j_obj, struct to_server_chat_msg_t msg) {
    json_object_object_add(j_obj, "type", json_object_new_int(msg.type));
    json_object_object_add(j_obj, "name", json_object_new_string(msg.name));
    json_object_object_add(j_obj, "chat_room_name", json_object_new_string(msg.chat_room_name));
}

void send_chat_message(int type, const char *room_name) {
    struct to_server_chat_msg_t msg = { -1, NULL, NULL };
    msg.type = type;
    dynamic_string_copy(&msg.chat_room_name, (char*)room_name);
    dynamic_string_copy(&msg.name, chat_userg.name);

    struct json_object *send_obj = json_object_new_object();
    struct_to_json_chat(send_obj, msg);

    send_dynamic_data_tcp(chat_server_sockg, (void*)json_object_get_string(send_obj));

    chat_thread_running = 1;

    free(msg.chat_room_name);
    free(msg.name);
    json_object_put(send_obj);
}

void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_OK) {
        GtkWidget *content_area = gtk_dialog_get_content_area(dialog);

        // Retrieve the entry widget from the content area
        GtkWidget *entry = gtk_container_get_children(GTK_CONTAINER(content_area))->data;
        
        if (GTK_IS_ENTRY(entry)) {
            const gchar *room_name = gtk_entry_get_text(GTK_ENTRY(entry));

            send_chat_message(CREATE, room_name);
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}


void on_chat_button_enter(GtkWidget *button, gpointer user_data) {
    GdkDisplay *display = gdk_display_get_default();
    if (!chat_thread_running) {
        GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_HAND1);
        gdk_window_set_cursor(gtk_widget_get_window(button), cursor);
    }
    else {
        GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_X_CURSOR);
        gdk_window_set_cursor(gtk_widget_get_window(button), cursor);
    }
}

void on_chat_button_leave(GtkWidget *button, gpointer user_data) {
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_LEFT_PTR);
    gdk_window_set_cursor(gtk_widget_get_window(button), cursor);
}

void create_chat_room_item(GtkWidget *button, gpointer user_data) {
    if (chat_thread_running) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Type new chat room name",
                                                GTK_WINDOW(windowg),
                                                GTK_DIALOG_MODAL,
                                                "_OK",
                                                GTK_RESPONSE_OK,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *name_entry = gtk_entry_new();

    gtk_container_add(GTK_CONTAINER(content_area), name_entry);

    gtk_widget_show_all(dialog);

    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), NULL);
}

GtkWidget *get_child_widget(GtkContainer *container) {
    GList *children = gtk_container_get_children(container);
    GtkWidget *childWidget = NULL;
    if (children != NULL) {
        childWidget = GTK_WIDGET(children->data);
        g_list_free(children);
    }
    return childWidget;
}

void clear_scrolled_window(GtkWidget *scrolledWindow) {
    GtkWidget *childWidget = get_child_widget(GTK_CONTAINER(scrolledWindow));
    gtk_container_remove(GTK_CONTAINER(scrolledWindow), childWidget);
    g_object_unref(childWidget);
}

void join_button_handler(GtkWidget *button, gpointer user_data) {
    if (chat_thread_running) return;

    GtkWidget *parentWidget = gtk_widget_get_parent(button);

    if (GTK_IS_CONTAINER(parentWidget)) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(parentWidget));
        if (children != NULL) {
            GtkLabel *firstChild = GTK_LABEL(children->data);
            const gchar *room_name = gtk_label_get_text(firstChild);
            send_chat_message(JOIN, room_name);
            g_list_free(children);
        }
    }
}

void print_chat_list(sock_fd_t *server_sockp) {
    char *msg_raw = NULL;
    recv_dynamic_data_tcp(*server_sockp, &msg_raw);

    struct json_object *chat_list_arr = json_tokener_parse(msg_raw);

    GtkWidget *scrolled_window = GTK_WIDGET(gtk_builder_get_object(builderg, "chat_list_scrolled_window"));
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    clear_scrolled_window(scrolled_window);

    GtkWidget *scrolled_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    int len = json_object_array_length(chat_list_arr);

    for (int i = 0 ; i < len ; i++) {
        struct json_object *elem = json_object_array_get_idx(chat_list_arr, i);
        const char *room_name = json_object_get_string(elem);
        GtkWidget *chat_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        GtkWidget *room_name_label = gtk_label_new(room_name);
        GtkWidget *join_button = gtk_button_new_with_label("Join");

        gtk_box_pack_start(GTK_BOX(chat_box), room_name_label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(chat_box), join_button, FALSE, FALSE, 0);

        g_signal_connect(join_button, "clicked", G_CALLBACK(join_button_handler), GINT_TO_POINTER(*server_sockp));

        gtk_box_pack_start(GTK_BOX(scrolled_box), chat_box, FALSE, TRUE, 0);
    }

    gtk_container_add(GTK_CONTAINER(scrolled_window), scrolled_box);

    gtk_widget_show_all(windowg);
    json_object_put(chat_list_arr);
    free(msg_raw);
}

void json_to_struct_chat(struct json_object *j_obj, struct to_client_chat_msg_t *msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "type") == 0) {
            msgp->type = json_object_get_int(val);
        }
        else if (strcmp(key, "chat_port") == 0) {
            msgp->chat_port = json_object_get_int(val);
        }
    }
}

void *async_chat_manager_pth(void* args) {
    int chat_close = 0;

    while (chat_close == 0) {
        if (!chat_thread_running) continue;

        char *buffer = NULL;
        if (recv_dynamic_data_tcp(chat_server_sockg, &buffer) != 0) {
            struct json_object *recv_obj = json_tokener_parse(buffer);

            if (recv_obj != NULL) {
                printf("buffer %s\n", buffer);
                struct to_client_chat_msg_t recv_msg = { -1, -1 };

                json_to_struct_chat(recv_obj, &recv_msg);

                if (recv_msg.type == SUCCESS) {
                    chat_close = 1;
                }
            }
            chat_thread_running = 0;

            free(buffer);
            json_object_put(recv_obj);
        }
    }

    pthread_kill(main_thread, SIGUSR2);

    pthread_exit(NULL);
}

void chat_mode() {

}

void chat_thread_done_callback(int signum) {
    gtk_stack_set_visible_child_name(GTK_STACK(stackg), "page3");
    

    free(chat_userg.name);
    free(chat_userg.password);
}

void chat_program(sock_fd_t server_sock, struct user_t user) {
    main_thread = pthread_self();
    chat_server_sockg = server_sock;
    dynamic_string_copy(&chat_userg.name, user.name);
    dynamic_string_copy(&chat_userg.password, user.password);

    gtk_stack_set_visible_child_name(GTK_STACK(stackg), "page2");

    GtkWidget *submit_button = GTK_WIDGET(gtk_builder_get_object(builderg, "create_chat_room_button"));

    g_signal_connect(submit_button, "clicked", G_CALLBACK(create_chat_room_item), GINT_TO_POINTER(server_sock));
    g_signal_connect(submit_button, "enter-notify-event", G_CALLBACK(on_chat_button_enter), NULL);
    g_signal_connect(submit_button, "leave-notify-event", G_CALLBACK(on_chat_button_leave), NULL);

    signal(SIGUSR2, chat_thread_done_callback);
    print_chat_list(&server_sock);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, async_chat_manager_pth, NULL) == -1) {
        perror("pthread_create in chat program, please reopen program");
    }
    else {
        pthread_detach(thread_id);
    }
}
