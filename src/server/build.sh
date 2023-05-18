#/bin/bash!

gcc main.c util.c auth.c chat_manager.c ../lib/chatutil.c ../lib/string_array.c -l json-c -l pthread