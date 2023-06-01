#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "./utils.h"
#include "./globals.h"
#include "./server.h"

struct env_t ENV;

int main(int argc, char** argv) {
    void init(char*);
    void terminate();

    init(argv[0]);

    server_on();

    terminate();
}

void init(char* filename) {
    void sigchld_handler();
    void terminate();

    char *user_dir = "usr",
         *chats_dir = "chats",
         *logs_dir = "logs";

    mode_t default_mode = 01755;

    if (opendir(user_dir) == NULL) {
        mkdir(user_dir, default_mode);
    }

    if (opendir(chats_dir) == NULL) {
        mkdir(chats_dir, default_mode);
    }

    if (opendir(logs_dir) == NULL) {
        mkdir(logs_dir, default_mode);
    }

    chmod(filename, 04700);

    signal(SIGINT, terminate);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, terminate);
    signal(SIGPIPE, SIG_IGN);

    ENV.max_password_cnt = 20;
    ENV.child_pids = NULL;
    ENV.child_ports = NULL;
    struct string_arr_t str_arr = { NULL, 0 };
    ENV.child_names = str_arr;
    struct client_arr_t clt_arr = { NULL, 0, NULL};
    ENV.clients_pipe = clt_arr;
    if (pthread_mutex_init(&ENV.mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        exit(0);
    }
}

void sigchld_handler() {
    wait(NULL);
}

void terminate() {
    if (ENV.child_pids != NULL) {
        for (int i = 0 ; i < ENV.child_names.len ; i++)
            kill(ENV.child_pids[i], SIGKILL);
        free(ENV.child_pids);
    }
    
    if (ENV.child_ports != NULL)
        free(ENV.child_ports);

    if (ENV.child_names.len > 0)
        string_arr_free(&ENV.child_names);

    if (ENV.clients_pipe.len > 0) {
        for (int i = 0 ; i < ENV.clients_pipe.len ; i++) {
            free(ENV.clients_pipe.pipe_arr[i]);
        }   

        free(ENV.clients_pipe.pipe_arr);
    }
    
    pthread_mutex_destroy(&ENV.mutex);

    exit(0);
}
