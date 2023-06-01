#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"
#include "./auth.h"
#include "./utils.h"
#include "./chat.h"
#include "./appwindow.h"

sock_fd_t server_sockg = -1;
struct user_t userg;

sock_fd_t connect_to_server(char *ip_address) {
    sock_fd_t fd;
    struct sockaddr_in server_addr;

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);

    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(fd);
        perror("connect");
        return -1;
    }

    return fd;
}

void terminate(int signum) {
    shutdown(server_sockg, SHUT_RDWR);

    if (userg.name != NULL)
        free(userg.name);
    if (userg.password != NULL)
        free(userg.password);

    exit(0);
}

gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    terminate(0);

    return TRUE;
}

gboolean check_connection(gpointer data) {
    server_sockg = connect_to_server(SERVER_IP_ADDRESS);
    
    if (server_sockg == -1) 
        return TRUE;

    auth(&server_sockg, &userg);

    return FALSE;
}

int main (int argc, char *argv[]) {
    dynamic_string_copy(&SERVER_IP_ADDRESS, "");

    gtk_init(&argc, &argv);
    GError *gerror = NULL;
    builderg = gtk_builder_new();
    if (gtk_builder_add_from_file(builderg, "public/default.glade", &gerror) == 0) {
        g_printerr("Error loading file: %s\n", gerror->message);
        g_clear_error(&gerror);
        return 1;
    }

    stackg = GTK_STACK(gtk_builder_get_object(builderg, "main_stack"));
    windowg = GTK_WIDGET(gtk_builder_get_object(builderg, "main_window"));
    g_signal_connect(windowg, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(windowg, "delete-event", G_CALLBACK(terminate), NULL);

    gtk_widget_show_all(windowg);

    g_timeout_add(1000, check_connection, &windowg);

    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
    signal(SIGQUIT, terminate);

    GtkCssProvider *cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(cssProvider, "public/styles.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_main();

    g_object_unref(builderg);

    return 0;
}
