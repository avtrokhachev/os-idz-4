#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024  // размер буфера

char buffer[BUFFER_SIZE];


// сервер, хранит в себе ip и порт
struct Server {
    char* ip;
    int port;
};

// валидирует размер кол-во входных аргументов
void validate_input_arguments(int argc) {
    if (argc != 3) {
        printf("Expected 2 arguments\n");
        exit(1);
    }
}

// отправляет сообщение на клиент
void send_message_to_server(int sock, struct sockaddr_in* server_addr, const char* message) {
    if (sendto(sock, message, strlen(message), 0, (struct sockaddr*) server_addr, sizeof(*server_addr)) < 0) {
        printf("Error occured while trying to send message.\n");
        exit(1);
    }
}


// принимает сообщение
void receive_message(int sock, struct sockaddr_in* server_addr, char* buffer) {
    unsigned int len = sizeof(*server_addr);
    int n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*) server_addr, &len);
    if (n < 0) {
        printf("Error occured while trying to receive message.\n");
        exit(1);
    }
    buffer[n] = '\0';
}


int main(int argc, char *argv[]) {
    validate_input_arguments(argc);

    // создаю логгер и сервер, куда отправлять запросы
    Server server;
    server.ip = argv[1];
    server.port = atoi(argv[2]);

    printf("Logger start working!\n");

    // создаю адрес курильщика и сервера
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        printf("Error occured while creating socket\n");
        exit(1);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server.ip);
    server_addr.sin_port = htons(server.port);

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(0);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(client_socket, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
        printf("Error while on bind\n");
        exit(1);
    }

    // отправляю сообщение на сервер с id логгера
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "%d", 3);
    send_message_to_server(client_socket, &server_addr, buffer);
    printf("Logger sent first message to the server\n");

    while (true) {
        // читаю сообщение от серсера
        receive_message(client_socket, &server_addr, buffer);
        printf("Logger got message from server: %s\n", buffer);

        sleep(1);
    }

    close(client_socket);
    return 0;
}