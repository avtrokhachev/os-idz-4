#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>


#define BUFFER_SIZE 1024
#define NUMBER_OF_SMOKERS 3
#define NUMBER_OF_LOGGERS 1


char buffer[BUFFER_SIZE];
char output_buffer[BUFFER_SIZE];


// курильщик
struct Smoker {
    int id;
};

// логгировщик системы
struct Logger {
    int id;
};


// валидирует размер кол-во входных аргументов
void validate_input_arguments(int argc) {
    if (argc != 3) {
        printf("Expected 2 argument\n");
        exit(1);
    }
}


// отправляет сообщение на клиент
void send_message(int sock, struct sockaddr_in* client_addr, const char* message) {
    if (sendto(sock, message, strlen(message), 0, (struct sockaddr*) client_addr, sizeof(*client_addr)) < 0) {
        printf("Error occured while trying to send message.\n");
        exit(1);
    }
}


// принимает сообщение
void receive_message(int sock, struct sockaddr_in* client_addr, char* buffer) {
    unsigned int len = sizeof(*client_addr);
    int n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*) client_addr, &len);
    if (n < 0) {
        printf("Error occured while trying to receive message.\n");
        exit(1);
    }
    buffer[n] = '\0';
}


int main(int argc, char *argv[]) {
    validate_input_arguments(argc);

    // создаю сокет для сервера
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        printf("Error occured while creating server socket\n");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        printf("Error occured while trying to bind socket\n");
        exit(1);
    }

    printf("Server successfully started working on port %d\n", atoi(argv[2]));

    // ждем, пока подключатся все 4 клиента подлючатся
    int cnt_smokers = 0;
    int cnt_loggers = 0;
    Smoker smokers[NUMBER_OF_SMOKERS];
    Logger loggers[NUMBER_OF_LOGGERS];
    struct sockaddr_in client_addr_tmp[NUMBER_OF_SMOKERS + NUMBER_OF_LOGGERS]; // аддреса клиентов в порядке получения
    struct sockaddr_in client_addr[NUMBER_OF_SMOKERS + NUMBER_OF_LOGGERS]; // аддреса курильщиков и логгеров
    while (cnt_smokers + cnt_loggers < NUMBER_OF_SMOKERS + NUMBER_OF_LOGGERS) {
        receive_message(server_socket, &client_addr_tmp[cnt_smokers + cnt_loggers], buffer);
        printf("Successfully read initial message from client\n");

        int smoker_item_id;
        if (sscanf(buffer, "%d", &smoker_item_id) != 1) {
            printf("Error occured while trying to parse client message.\n");
            continue;
        }
        if (smoker_item_id <= 2) {
            smokers[smoker_item_id].id = smoker_item_id;
            client_addr[smoker_item_id] = client_addr_tmp[cnt_smokers + cnt_loggers];
            ++cnt_smokers;
            printf("Successfully added smoker id=%d\n", smoker_item_id);
        } else {
            loggers[0].id = 0;
            client_addr[NUMBER_OF_SMOKERS] = client_addr_tmp[cnt_smokers + cnt_loggers];
            ++cnt_loggers;
            printf("Successfully added logger id=%d\n", loggers[0].id);
        }
        sleep(1);
    }
    printf("All smokers and logger connected to the server\n");
    send_message(server_socket, &client_addr[NUMBER_OF_SMOKERS], "All smokers and logger connected to the server\n");

    // основной цикл работы сервера
    while (true) {
        // сгенерировать новый предмет на столе и отправиль сообщения всем курильщикам об этом событии
        int smoke_item_id = rand() % 3;
        printf("Server generated smoke_item id=%d\n", smoke_item_id);
        sprintf(output_buffer, "Server generated smoke_item id=%d\n", smoke_item_id);
        send_message(server_socket, &client_addr[NUMBER_OF_SMOKERS], output_buffer);
        memset(buffer, 0, BUFFER_SIZE);
        sprintf(buffer, "%d", smoke_item_id);
        for (int i = 0; i < NUMBER_OF_SMOKERS; ++i) {
            send_message(server_socket, &client_addr[i], buffer);
        }
        printf("Successfully sent messages to all smokers about smoke_item_id=%d\n", smoke_item_id);
        sprintf(output_buffer, "Successfully sent messages to all smokers about smoke_item_id=%d\n", smoke_item_id);
        send_message(server_socket, &client_addr[NUMBER_OF_SMOKERS], output_buffer);

        // ждать ответа от курильщика, когда он закурит
        receive_message(server_socket, &client_addr_tmp[0], buffer);
        printf("Smoker id=%s got component and start smoking\n", buffer);
        sprintf(output_buffer, "Smoker id=%s got component and start smoking\n", buffer);
        send_message(server_socket, &client_addr[NUMBER_OF_SMOKERS], output_buffer);

        int time_to_sleep = 2 + (rand() % 4);
        sleep(time_to_sleep);
    }

    close(server_socket);
    return 0;
}