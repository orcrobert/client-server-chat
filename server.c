#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 20

typedef int SOCKET;

SOCKET client_sockets[MAX_CLIENTS];
char * client_usernames[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char * message, int sender_socket) {
    pthread_mutex_lock(&lock);

    for (int i = 0; i < client_count; ++i) {
        if (client_sockets[i] != sender_socket) {
            send(client_sockets[i], message, strlen(message) + 1, 0);
        }
    }

    pthread_mutex_unlock(&lock);
}

void * handle_client(void * client_socket) {
    int sock = *(int *)client_socket;
    char buffer[1024];
    int read_size;

    read_size = recv(sock, buffer, 1024, 0);
    if (read_size > 0) {
        buffer[read_size] = '\0';
        pthread_mutex_lock(&lock);
        client_usernames[client_count - 1] = strdup(buffer);
        pthread_mutex_unlock(&lock);
        printf("%s connected\n", buffer);
    }

    while ((read_size = recv(sock, buffer, 1024, 0)) > 0) {
        buffer[read_size] = '\0';

        char message[1024];
        snprintf(message, sizeof(message), "%s: %s", client_usernames[client_count - 1], buffer);

        printf("%s", message);
        broadcast_message(message, sock);
    }

    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; ++i) {
        if (client_sockets[i] == sock) {
            free(client_usernames[i]);
            client_sockets[i] = client_sockets[client_count - 1];
            client_usernames[i] = client_usernames[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    close(sock);
    printf("Client disconnected!\n");
    return 0;
}

int main() {
    SOCKET server_socket, client_socket, c;
    struct sockaddr_in server, client;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Socket could not be created!\n");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Bind failed!\n");
        return 1;
    }

    listen(server_socket, 3);
    printf("Waiting for connections...\n");

    c = sizeof(struct sockaddr_in);

    while ((client_socket = accept(server_socket, (struct sockaddr *) &client, (socklen_t *) &c))) {
        pthread_mutex_lock(&lock);
        client_sockets[client_count++] = client_socket;
        pthread_mutex_unlock(&lock);

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *) &client_socket) < 0) {
            perror("Could not create thread!\n");
            return 1;
        }
    }

    if (client_socket < 0) {
        perror("Accept failed!\n");
        return 1;
    }

    close(server_socket);
    return 0;
}