#/bin/bash!

gcc main.c util.c auth.c chat_manager.c ../lib/chatutil.c -l json-c -l pthread