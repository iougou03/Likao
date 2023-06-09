#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <json-c/json.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <pthread.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"

#define MAX_CLIENTS 40
#define BUFFER_SIZE 1024

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

struct client_info_args {
    int clientSocket;
    int* clientCount;
};

int clientSockets[MAX_CLIENTS];

void *child_thread(void* args) {
    struct client_info_args *argsp = (struct client_info_args*)args;

    while (1) {
        char *buffer = NULL;

        if (recv_dynamic_data_tcp(argsp->clientSocket, &buffer) == -1) {
            break;
        }

        if (buffer > 0) {
            json_object *msg_obj = json_tokener_parse(buffer);

            if (msg_obj != NULL) {
                for (int i = 0; i < *argsp->clientCount; i++) {
                    int client = clientSockets[i];
                    send_dynamic_data_tcp(client, (char*)json_object_get_string(msg_obj));
                }

                json_object_put(msg_obj);
            }
        }

        if (send(argsp->clientSocket, " ", sizeof(char), 0) == -1) {
            break;
        }
    }

    // Close the client socket and exit the child process
    close(argsp->clientSocket);
    pthread_exit(NULL);
}

void child_server(int port) {
    int serverSocket, maxClients = MAX_CLIENTS;
    struct sockaddr_in serverAddress, clientAddress;

    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind the server socket to the specified address and port
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, maxClients) < 0) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    // Create shared memory for client count
    key_t shmKey = ftok(".", 's');
    int shmID = shmget(shmKey, sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("Failed to create shared memory segment");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    int *clientCount = (int *)shmat(shmID, NULL, 0);
    if (clientCount == (int *)-1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // Initialize the client count
    *clientCount = 0;

    while (1) {
        socklen_t clientAddressSize = sizeof(clientAddress);

        // Accept a new connection
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressSize);
        if (clientSocket < 0) {
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }

        printf("New client connected\n");

        // Add the new client socket to the array
        clientSockets[*clientCount] = clientSocket;
        (*clientCount)++;

        pthread_t thread_id;
        struct client_info_args a = { clientSocket, clientCount};

        if (pthread_create(&thread_id, NULL, child_thread, (void*)&a) == -1) {
        perror("pthread_create, please reopen program");
        }
        else {
            pthread_detach(thread_id);
        }
    }

    // Detach and remove shared memory
    shmdt(clientCount);
    shmctl(shmID, IPC_RMID, NULL);

    // Close the server socket
    close(serverSocket);

    return;
}
