// s-talk.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "list.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define RED_TEXT "\x1b[31m"
#define GREEN_TEXT "\x1b[32m"
#define RESET_TEXT "\x1b[0m"

List* messageList;
pthread_mutex_t inputMutex;
char username[50];
int sockfd; // Socket file descriptor
int listen_sock; // Listening socket for incoming connections
bool isServer; // Indicates whether the program is acting as a server

// Function prototypes
void* keyboardInput(void* arg);
void* sendMessage(void* arg);
void* receiveMessage(void* arg);
void* displayMessages(void* arg);
void launchServer(int myPort);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s [my port number] [remote machine name] [remote port number]\n", argv[0]);
        return 1;
    }

    int myPort = atoi(argv[1]);
    char* remoteMachine = argv[2];
    int remotePort = atoi(argv[3]);

    struct sockaddr_in server_addr;
    struct hostent* server;

    messageList = List_create();
    pthread_mutex_init(&inputMutex, NULL);
    isServer = false;

    // Get the username from the user
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; // Remove the newline character

    // Try to connect to the remote server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }

    server = gethostbyname(remoteMachine);
    if (server == NULL) {
        fprintf(stderr, "Error: Host not found\n");
        launchServer(myPort); // Launch the server if host is not found
    } else {
        memset((char*)&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy((char*)&server_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
        server_addr.sin_port = htons(remotePort);

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) < 0)) {
            perror("Error connecting to remote machine");
            launchServer(myPort); // Launch the server if connection fails
        }
    }

    pthread_t inputThread, sendThread, receiveThread, displayThread;
    pthread_create(&inputThread, NULL, keyboardInput, NULL);
    pthread_create(&sendThread, NULL, sendMessage, NULL);
    pthread_create(&receiveThread, NULL, receiveMessage, NULL);
    pthread_create(&displayThread, NULL, displayMessages, NULL);

    pthread_join(inputThread, NULL);
    pthread_join(sendThread, NULL);
    pthread_join(receiveThread, NULL);
    pthread_join(displayThread, NULL);

    List_free(messageList, free);

    close(sockfd); // Close the socket
    if (isServer) {
        close(listen_sock); // Close the listening socket if it's the server
    }

    return 0;
}

void* keyboardInput(void* arg) {
    while (true) {
        char message[256];
        printf("Enter your message: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0'; // Remove the newline character

        pthread_mutex_lock(&inputMutex);
        List_append(messageList, strdup(message)); // Store the message in the list
        pthread_mutex_unlock(&inputMutex);

        // Check if the entered message is the termination condition ('!')
        if (strcmp(message, "!") == 0) {
            break;
        }
    }
    return NULL;
}

void* sendMessage(void* arg) {
    while (true) {
        pthread_mutex_lock(&inputMutex);
        int count = List_count(messageList);
        pthread_mutex_unlock(&inputMutex);

        if (count > 0) {
            char* message = NULL;

            pthread_mutex_lock(&inputMutex);
            message = (char*)List_trim(messageList); // Get the oldest message
            pthread_mutex_unlock(&inputMutex);

            // Send the message over the network
            if (send(sockfd, message, strlen(message), 0) < 0) {
                perror("Error sending message");
            }
            free(message); // Free the message after sending
        }
    }
    return NULL;
}

void* receiveMessage(void* arg) {
    while (true) {
        char receivedMessage[256];
        ssize_t bytesReceived = recv(sockfd, receivedMessage, sizeof(receivedMessage) - 1, 0);
        if (bytesReceived < 0) {
            perror("Error receiving message");
            return NULL;
        } else if (bytesReceived == 0) {
            // Connection closed by the other user
            printf("Connection closed by the remote user.\n");
            break;
        }
        receivedMessage[bytesReceived] = '\0'; // Null-terminate the received message

        printf(RED_TEXT "[%s]: %s\n" RESET_TEXT, "Remote User", receivedMessage); // Display received message
    }
    return NULL;
}

void* displayMessages(void* arg) {
    while (true) {
        pthread_mutex_lock(&inputMutex);
        int count = List_count(messageList);
        pthread_mutex_unlock(&inputMutex);

        if (count > 0) {
            char* message = NULL;

            pthread_mutex_lock(&inputMutex);
            message = (char*)List_trim(messageList); // Get the oldest message
            pthread_mutex_unlock(&inputMutex);

            printf(GREEN_TEXT "[You]: %s\n" RESET_TEXT, message); // Display your own message
            free(message); // Free the message after displaying
        }
    }
    return NULL;
}

void launchServer(int myPort) {
    struct sockaddr_in my_addr;

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("Error creating listening socket");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(myPort);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

    if (bind(listen_sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) < 0) {
        perror("Error binding to local port");
        exit(1);
    }

    if (listen(listen_sock, 5) < 0) {
        perror("Error listening on socket");
        exit(1);
    }

    isServer = true;

    printf("Server launched and waiting for incoming connections...\n");

    while (1) {
        int new_fd;
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(struct sockaddr_in);

        if ((new_fd = accept(listen_sock, (struct sockaddr*)&client_addr, &sin_size)) < 0) {
            perror("Error accepting connection");
            continue; // Continue waiting for a new connection
        }

        printf("Connected to a remote user.\n");
        sockfd = new_fd;
        break; // Exit the loop after a successful connection
    }
}
