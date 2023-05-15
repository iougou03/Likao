#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

#include "../lib/chat.h"
#include "util.h"

/**
 * -1: file open
 * -2: JSON file
 * -3: password error
*/
int sign_in(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]) {
    int flag = 0;
    char path[1024];
    sprintf(path, "usr/%s.json", name);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    
    // Read the JSON object from the file
    struct json_object *root = json_object_from_file(path);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON\n");
        fclose(fp);
        return -2;
    }
    // Close the file
    fclose(fp);
    
    // Use the JSON object
    // printf("The JSON object is:\n%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    
    //get password
    json_object *j_password;
    json_object_object_get_ex(root, "password", &j_password);


    //check password
    char n_password[PASSWORD_MAX_LEN+2];
    sprintf(n_password, "%s", password);
    // printf("password in .json : %s\n", json_object_get_string(j_password));
    // printf("password input : %s\n", n_password);
    if(strcmp(json_object_get_string(j_password), n_password) == 0){
        printf("sign in\n");
        return 0;
    }
    else{
        printf("Error : Wrong Password\n");
        return -3;
    }

}

/**
 * error code
 * -1: there already user signed
 * -2: error while finding usr/
 * -3: error while saving json file
*/
int sign_up(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]) {
    if (chdir("usr") == -1) {
        fprintf(stderr, "there error while entering usr/, check init()");
        return -2;
    }
    int flag = 0, fd;
    char *ext = ".json";
    char *filename =  (char*)malloc(sizeof(char) * (strlen(name) + strlen(ext)));

    strcpy(filename, name);
    strcat(filename, ext);
    

    if ((fd = open(filename, O_RDONLY)) == -1) {
        time_t now = time(0);
        struct json_object *usr_json = json_object_new_object();
        json_object_object_add(usr_json, "name", json_object_new_string(name));
        json_object_object_add(usr_json, "password", json_object_new_string(password));
        json_object_object_add(usr_json, "created_at", json_object_new_int64(now));

        fd = open(filename, O_WRONLY | O_CREAT);

        if (json_object_to_fd(fd, usr_json, JSON_C_TO_STRING_PRETTY | O_SYNC) == -1) {
            perror(json_util_get_last_err());
            flag = -3;
        }

        flag = 0;

        chmod(filename, 00744);
        json_object_put(usr_json);
    }
    else flag = -1;

    close(fd);
    chdir("..");
    free(filename);
    return flag;
}

void pth_auth(pthread_t tid, sock_fd client_sock_fd) {
    void* received_msg_raw = malloc(sizeof(struct msg_from_client_t));

    /**
     * TODO: if user termiate, server also terminate
     * 
     * RESOLVE: should not terminate
    */
    if (recv(client_sock_fd, received_msg_raw ,sizeof(struct msg_from_client_t), 0) == -1) {
        perror("error while receiving");
        pthread_cancel(tid);
    }

    struct json_object *j_obj = json_tokener_parse(received_msg_raw);
    struct msg_from_client_t msg;

    json_to_strcut(j_obj, &msg);

    printf("recv message: %d, %s %s\n",msg.type, msg.name, msg.password);

    struct msg_from_server_t msg_server;
    struct json_object *send_json_obj = json_object_new_object();

    // TODO: sign handler
    if (msg.type == SIGN_IN) {
        if(sign_in(msg.name, msg.password) == 0){
            msg_server.type = SUCCESS;
            strcpy(msg_server.msg, "you success to sign in!\n");            
        }
        else {
            msg_server.type = FAILED;
            strcpy(msg_server.msg, "error!");
        }

        struct_to_json(send_json_obj, msg_server);

        send(
            client_sock_fd,
            json_object_to_json_string(send_json_obj),
            sizeof(struct msg_from_server_t),
            0
        );
    }
    else if (msg.type == SIGN_UP) {
        int flag = sign_up(msg.name, msg.password);
        if (flag == -1) {
            msg_server.type = FAILED;
            strcpy(msg_server.msg, "there already signed user!");

            struct_to_json(send_json_obj, msg_server);

            send(
                client_sock_fd,
                json_object_to_json_string(send_json_obj),
                sizeof(struct msg_from_server_t),
                0
            );
        }
        else if (flag == 0) {
            msg_server.type = SUCCESS;
            strcpy(msg_server.msg, "you success to sign up!");

            struct_to_json(send_json_obj, msg_server);

            if (send(
                client_sock_fd,
                json_object_to_json_string(send_json_obj),
                sizeof(struct msg_from_server_t),
                0
            ) == -1) {
                perror("error at sending sign msg to client");
            }
        }
        else {
            fprintf(stderr, "error while sign up");
        }

    }
    
    json_object_put(j_obj);
    json_object_put(send_json_obj);
    free(received_msg_raw);
}