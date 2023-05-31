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
#include "./chat_child.h"

struct env_t ENV = {
    40,
    NULL,
    NULL,
    { NULL, 0 }
};

int main(int argc, char** argv) {
    void init(char*);
    void terminate();
    void create_chats_process();

    init(argv[0]);

    create_chats_process();
    
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
    signal(SIGPIPE, SIG_IGN);
}

void sigchld_handler() {
    wait(NULL);
}

void terminate() {
    if (ENV.child_pids != NULL)
        free(ENV.child_pids);
    
    if (ENV.child_ports != NULL)
        free(ENV.child_ports);

    if (ENV.child_names.len > 0)
        string_arr_free(&ENV.child_names);
    
    exit(0);
}

void create_chats_process() {
    DIR *dir_ptr;
    struct dirent *direntp;
    int chats_cnt = 0, len = 0;
    char *room_name;

    if ((dir_ptr = opendir("chats")) == NULL) {
        fprintf(stderr, "cannot open chats directory\n");
        exit(1);
    }

    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (strstr(direntp->d_name, ".json") != NULL) {
            chats_cnt++;
            len = strlen(direntp->d_name) - 5;
            room_name = (char*)malloc(sizeof(char) * len);

            strncpy(room_name, direntp->d_name, len);

            string_arr_append(&ENV.child_names, room_name);
        }
    }

    closedir(dir_ptr);

    ENV.child_pids = (int*)malloc(sizeof(int) * chats_cnt);

    for (int i = 0 ; i < chats_cnt ;) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        else if (pid == 0) {
            ENV.child_pids[i] = getpid();
            ENV.child_ports[i] = DEFAULT_PORT + (i + 1);

            child_server(DEFAULT_PORT + (i + 1));

            exit(0);
        }
        else i++;
    }
}