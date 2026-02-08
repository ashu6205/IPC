#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &server.sin_addr);

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    printf("Connected to server. Type messages:\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);

        write(sock, buffer, strlen(buffer));

        int n = read(sock, buffer, BUFFER_SIZE - 1);
        if (n <= 0) {
            printf("Server disconnected\n");
            break;
        }
        buffer[n] = '\0';
        printf("Echo: %s", buffer);
    }

    close(sock);
    return 0;
}
