#ifndef HEADER_FILE_STRING_ARRAY
#define HEADER_FILE_STRING_ARRAY

struct string_arr {
    char** data;
    int len;
    int size;
};

void string_arr_append(struct string_arr* arr, char* str);

void string_arr_remove(struct string_arr* arr, char* str) ;

void string_arr_free(struct string_arr* arr);

#endif