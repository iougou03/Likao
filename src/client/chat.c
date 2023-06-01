#include <stdio.h>
#include <unistd.h>
#include <json-c/json.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./utils.h"
#include "./appwindow.h"
#include "./utils.h"

int chat_thread_running = 0;
struct user_t chat_userg;
sock_fd_t chat_server_sockg;
pthread_t main_thread;

int room_portg;

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

void on_create_chat_room_button_clicked(GtkWidget *button, gpointer user_data) {
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

    if (childWidget != NULL) {
        gtk_container_remove(GTK_CONTAINER(scrolledWindow), childWidget);
        g_object_unref(childWidget);
    }
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
    int bytes_recv;
    char *msg_raw = NULL;
    if ((bytes_recv = recv_dynamic_data_tcp(*server_sockp, &msg_raw)) == -1) return;

    if (bytes_recv == 0) return;

    struct json_object *chat_list_arr = json_tokener_parse(msg_raw);

    if (chat_list_arr == NULL) return;

    if (json_object_is_type(chat_list_arr, json_type_array)) {
        GtkWidget *scrolled_window = GTK_WIDGET(gtk_builder_get_object(builderg, "chat_list_scrolled_window"));
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        // clear_scrolled_window(scrolled_window);

        GtkWidget *scrolled_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        int len = json_object_array_length(chat_list_arr);

        GtkWidget *viewport = gtk_viewport_new(NULL, NULL);

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

        gtk_container_add(GTK_CONTAINER(viewport), scrolled_box);
        gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);

        gtk_widget_show_all(windowg);
    }

    free(msg_raw);
    json_object_put(chat_list_arr);
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
    int chat_close = 0, bytes;

    tcp_non_block(chat_server_sockg);

    while (!chat_close) {
        char *buffer = NULL;
        if ((bytes = recv_dynamic_data_tcp(chat_server_sockg, &buffer)) == -1) {
            sleep(1);
            continue;
        }

        if (bytes > 0) {
            struct json_object *recv_obj = json_tokener_parse(buffer);
            if (recv_obj != NULL && json_object_is_type(recv_obj, json_type_object)) {
                struct to_client_chat_msg_t recv_msg = { -1, -1 };

                json_to_struct_chat(recv_obj, &recv_msg);
                if (recv_msg.type == SUCCESS && recv_msg.chat_port > 0) {
                    // chat_close = 1;
                    room_portg = recv_msg.chat_port;
                    pthread_kill(main_thread, SIGUSR2);
                }

                chat_thread_running = 0;

                free(buffer);
                json_object_put(recv_obj);
            }
        }

    }
    pthread_exit(NULL);
}


sock_fd_t child_chat_sockg;
struct sockaddr_in child_chat_addrg;

void enter_log(GtkWidget *button, gpointer user_data) {

    GtkEntry *chat_log_entry = GTK_ENTRY(gtk_builder_get_object(builderg, "chat_log_entry"));

    const gchar *log = gtk_entry_get_text(chat_log_entry);

    struct json_object *j_obj = json_object_new_object();
    json_object_object_add(j_obj, "name", json_object_new_string(chat_userg.name));
    json_object_object_add(j_obj, "message", json_object_new_string(log));

    send_dynamic_data_tcp(child_chat_sockg, (char*)json_object_get_string(j_obj));
    
    gtk_entry_set_text(GTK_ENTRY(chat_log_entry), "");
}

void log_json_to_struct(struct json_object* j_obj, struct to_server_log_msg_t *msgp) {
    json_object_object_foreach(j_obj, key, val) {
        if (strcmp(key, "name") == 0) {
            char *name = (char*)json_object_get_string(val);
            dynamic_string_copy(&(msgp->name), name);
        }
        else if (strcmp(key, "message") == 0) {
            char *message = (char*)json_object_get_string(val);
            dynamic_string_copy(&(msgp->message), message);
        }
    }
}

void *async_chat_mode_pth(void* args) {
    struct json_object *welcome_obj = json_object_new_object();
    json_object_object_add(welcome_obj, "name", json_object_new_string(chat_userg.name));
    json_object_object_add(welcome_obj, "message", json_object_new_string("welcome"));
    char *msg = (char*)json_object_get_string(welcome_obj);

    send_dynamic_data_tcp(child_chat_sockg, msg);

    json_object_put(welcome_obj);

    while (1) {
        int bytes;
        char *buffer = NULL;
        if ((bytes = recv_dynamic_data_tcp(child_chat_sockg, &buffer)) == -1) 
            break;

        if (bytes > 0) {
            struct json_object *msg_obj = json_tokener_parse(buffer);

            if (msg_obj != NULL && json_object_is_type(msg_obj, json_type_object)) {
                GtkWidget *log_box = GTK_WIDGET(gtk_builder_get_object(builderg,"chat_log_box"));
                
                struct to_server_log_msg_t recv_msg = { NULL, NULL };
                log_json_to_struct(msg_obj, &recv_msg);

                char *label_text = NULL;
                if (strcmp(recv_msg.message, "welcome") == 0) {
                    char *wel_msg = " has entered!";
                    label_text = (char*)malloc(sizeof(char) * (strlen(recv_msg.name) + strlen(wel_msg) + 1));
                    sprintf(label_text, "%s%s", recv_msg.name, wel_msg);
                }
                else {
                    label_text = (char*)malloc(sizeof(char) * (strlen(recv_msg.name) + 5 + strlen(recv_msg.message) + 1));
                    sprintf(label_text, "[%s] : %s", recv_msg.name, recv_msg.message);
                }

                GtkWidget *label = gtk_label_new(label_text);

                gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                gtk_container_set_border_width(GTK_CONTAINER(log_box), 16);
                gtk_box_pack_start(GTK_BOX(log_box), label, FALSE, TRUE, 0);
                gtk_widget_show_all(windowg);

                json_object_put(msg_obj);
                free(label_text);
            }
            free(buffer);
        }
    }

    pthread_exit(NULL);
}

void chat_mode() {
    gtk_stack_set_visible_child_name(GTK_STACK(stackg), "page3");

    GtkWidget *scrolled_window = GTK_WIDGET(gtk_builder_get_object(builderg, "chat_log_scrolled_window"));
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    // Create a UDP socket
    if ((child_chat_sockg = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket");
        return;
    }

    // Set up the server address structure
    memset(&child_chat_addrg, 0, sizeof(child_chat_addrg));
    child_chat_addrg.sin_family = AF_INET;
    child_chat_addrg.sin_port = htons(room_portg);
    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &(child_chat_addrg.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        return;
    }

    if (connect(child_chat_sockg, (struct sockaddr *)&child_chat_addrg, sizeof(child_chat_addrg)) < 0) {
        perror("Failed to connect to the server");
        exit(EXIT_FAILURE);
    }

    GtkWidget *submit_button = GTK_WIDGET(gtk_builder_get_object(builderg, "submit_chat_log_button"));

    g_signal_connect(submit_button, "clicked", G_CALLBACK(enter_log), NULL);

    // close(child_chat_sock);
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, async_chat_mode_pth, NULL) == -1) {
        perror("pthread_create in chat program, please reopen program");
    }
    else {
        pthread_detach(thread_id);
    }
}

void chat_thread_done_callback(int signum) {
    chat_mode();

    // free(chat_userg.name);
    // free(chat_userg.password);
}

void chat_program(sock_fd_t server_sock, struct user_t *user) {
    main_thread = pthread_self();
    chat_server_sockg = server_sock;
    chat_userg.name = NULL;
    chat_userg.password = NULL;

    dynamic_string_copy(&chat_userg.name, user->name);
    dynamic_string_copy(&chat_userg.password, user->password);

    GtkWidget *submit_button = GTK_WIDGET(gtk_builder_get_object(builderg, "create_chat_room_button"));

    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_create_chat_room_button_clicked), GINT_TO_POINTER(server_sock));
    g_signal_connect(submit_button, "enter-notify-event", G_CALLBACK(on_chat_button_enter), NULL);
    g_signal_connect(submit_button, "leave-notify-event", G_CALLBACK(on_chat_button_leave), NULL);

    signal(SIGUSR2, chat_thread_done_callback);

    gtk_stack_set_visible_child_name(GTK_STACK(stackg), "page2");

    print_chat_list(&server_sock);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, async_chat_manager_pth, NULL) == -1) {
        perror("pthread_create in chat program, please reopen program");
    }
    else {
        pthread_detach(thread_id);
    }

}
