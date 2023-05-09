#include "../lib/chat.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <json-c/json.h>

int sign_in(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]) {
    int flag = 0;

    char path[1024];
    sprintf(path, "usr/%s.json", name);

    FILE *fp = fopen(path, "r");
    if (!fp) { 
        return -1;
    }
    
    // Read the JSON object from the file
    struct json_object *root = json_object_from_file(fp);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON\n");
        fclose(fp);
        return -1;
    }
    // Close the file
    fclose(fp);
    
    // Use the JSON object
    printf("The JSON object is:\n%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    
    //get password
    char *j_password;
    json_object_object_get_ex(root, "password", &j_password);

    // Free the JSON object
    json_object_put(root);

    //check password
    if(strcmp(j_password, password) == 0){
        printf("sign in ");
        return 0;
    }
    else{
        printf("Error : Wrong Password");
        return -1;
    }

}

int sign_up(char name[NAME_MAX_LEN], char password[PASSWORD_MAX_LEN]) {


}