// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <sys/shm.h>
// #include <sys/sem.h>
// #include <sys/ipc.h>

// #define MAX_CLIENTS 40

// void child_server(int port) {
//     sock_fd_t child_socket_fd, client_socks[MAX_CLIENTS];
//     int client_cnt = 0;
//     struct sockaddr_in server_addr, client_addr;

//     child_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

//     if (child_socket_fd == -1) {
//         perror("socket");
//         exit(1);
//     }

//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//     server_addr.sin_port = htons(port);

//     if (bind(child_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))== -1) {
//         perror("bind");
//         exit(1);
//     }

//     if (listen(child_socket_fd, MAX_CLIENTS) < 0) {
//         perror("listen");
//         exit(1);
//     }

//     printf("child UDP server open at port %d\n",port);

//     while(1) {
//          socklen_t client_addr_size = sizeof(client_addr);

//         // Accept a new connection
//         int clientSocket = accept(child_socket_fd, (struct sockaddr *)&client_addr, &client_addr_size);
//         if (clientSocket < 0) {
//             perror("Failed to accept connection");
//             exit(EXIT_FAILURE);
//         }

//         printf("New client connected\n");

//         // Add the new client socket to the array
//         client_socks[client_cnt++] = clientSocket;

//         pid_t pid = fork();
//         if (pid < 0) {
//             perror("Failed to fork");
//             exit(EXIT_FAILURE);
//         } else if (pid == 0) {
//             // Child process handles the client

//             char buffer = NULL;
//             ssize_t bytesRead;

//             while (1) {
//                 // Receive data from the client
//                 bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
//                 if (bytesRead < 0) {
//                     perror("Failed to receive data from client");
//                     exit(EXIT_FAILURE);
//                 } else if (bytesRead == 0) {
//                     // Client has closed the connection
//                     break;
//                 }

//                 // Process the received data (e.g., echo back to the client)
//                 // ...

//                 // Send a response to the client
//                 ssize_t bytesSent = send(clientSocket, buffer, bytesRead, 0);
//                 if (bytesSent < 0) {
//                     perror("Failed to send data to client");
//                     exit(EXIT_FAILURE);
//                 }
//             }

//             // Close the client socket and exit the child process
//             close(clientSocket);
//             exit(0);
//         } else {
//             close(clientSocket);
//         }
//     }

//     close(child_socket_fd);
// }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#include "../lib/likao_chat.h"
#include "../lib/likao_utils.h"

#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

void child_server(int port) {
    int serverSocket, clientSockets[MAX_CLIENTS], maxClients = MAX_CLIENTS;
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

        // Fork a new process to handle the client
        pid_t pid = fork();
        if (pid < 0) {
            perror("Failed to fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process handles the client


            while (1) {
                char *buffer = NULL;
                recv_dynamic_data_tcp(clientSocket, &buffer);

                if (buffer < 0) {
                    continue;
                }

                for (int i = 0; i < *clientCount; i++) {
                    int client = clientSockets[i];
                    if (client != clientSocket) {
                        send_dynamic_data_tcp(clientSocket, "hello");
                    }
                }
            }

            // Close the client socket and exit the child process
            close(clientSocket);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process continues to accept new connections
            // Close the client socket (parent does not need it)
            close(clientSocket);
        }
    }

    // Detach and remove shared memory
    shmdt(clientCount);
    shmctl(shmID, IPC_RMID, NULL);

    // Close the server socket
    close(serverSocket);

    return;
}
