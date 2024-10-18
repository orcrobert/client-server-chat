#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080

typedef int SOCKET;

void * receive_messages(void * sock) {
    int server_socket = *(int *) sock;
    char message[1024];
    int read_size;

    while ((read_size = recv(server_socket, message, 1024, 0)) > 0) {
        message[read_size - 1] = '\0';
        printf("%s", message);
    }

    return 0;
}


int main() {
    SOCKET server_socket;
    struct sockaddr_in server;
    char message[1024];
    char username[50];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        printf("Could not create socket!\n");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (connect(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Error connecting to server!\n");
        return 1;
    }

    printf("Enter username: ");
    fgets(username, 50, stdin);
    username[strcspn(username, "\n")] = 0;

    send(server_socket, username, strlen(username) + 1, 0);

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *) &server_socket) < 0) {
        perror("Could not create thread!\n");
        return 1;
    }

    while (1) {
        fgets(message, 1024, stdin);
        send(server_socket, message, strlen(message) + 1, 0);
    }
    
    close(server_socket);
}