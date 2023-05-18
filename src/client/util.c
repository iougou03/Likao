#include <stdio.h>
#include <string.h>

struct string_arr {
    char** data;
    int size;
};

void string_arr_append(struct string_arr* arr, char* str) {
    int str_size = strlen(str) * sizeof(char);
    
    arr->size += str_size;

    // arr->data.realloc()
}