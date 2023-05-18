#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_array.h"

void string_arr_append(struct string_arr* arr, char* str) {
    int str_size = strlen(str) * sizeof(char);
    char* copy = (char*)malloc(str_size);
    strcpy(copy, str);

    arr->data = realloc(arr->data, sizeof(char**) * (arr->len + 1));

    arr->data[arr->len++] = copy;
    arr->size += str_size;
}

void string_arr_remove(struct string_arr* arr, char* str) {
    int remove_size = 0, find = 0;

    for (int i = 0 ; i < arr->len ; i++) {
        if (!find && strcmp(arr->data[i], str) == 0) {
            remove_size = strlen(str) * sizeof(char);
            
            free(arr->data[i]);

            find = i;
        }
        else if (find) {
            arr->data[i - 1] = arr->data[i];
        }
    }
    arr->len--;
    arr->size -= remove_size;
}

void string_arr_free(struct string_arr* arr) {
    for (int i = 0 ; i < arr->len ; i++) {
        free(arr->data[i]);
    }

    free(arr->data);
}
