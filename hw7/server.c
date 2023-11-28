#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define PIPE_NAME "server_pipe"
#define SUCCESS_MESSAGE "\bSuccess Sending Message"
#define MAX_LINE 256

// === 클라이언트 정보를 저장할 연결 리스트 ===
typedef struct Node {
    char client_name[50];
    int client_pd;
    struct Node* next;
} ClientNode;

ClientNode* clients = NULL;

// === LinkedList에 클라이언트 정보 추가 ===
void add_client(const char *client_name, int client_pd) {
    ClientNode* new_client = (ClientNode*)malloc(sizeof(ClientNode));
    if (new_client == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(new_client->client_name, client_name);
    new_client->client_pd = client_pd;
    new_client->next = clients;
    clients = new_client;

    printf("Client connected: %s\n", client_name);
}

// === LinkedList에서 클라이언트 정보 제거 ===
void remove_client(ClientNode* target) {
    ClientNode* current = clients;
    ClientNode* prev = NULL;

    while (current != NULL) {
        if (current == target) {
            if (prev == NULL) clients = current->next;
            else prev->next = current->next;

            // === 클라이언트 파이프를 닫고, 연결리스트 메모리 해제 ===
            close(current->client_pd);
            free(current);
            return;
        } else {
            prev = current;
            current = current->next;
        }
    }
}

// === 메세지를 보낸 클라이언트를 제외한 모든 클라이언트에게 메세지 전송 ===
void handle_client_message(const char *message) {
    ClientNode* current = clients;
    while (current != NULL) {
        if (strstr(message, current->client_name) == NULL) {
            ssize_t n = write(current->client_pd, message, strlen(message)+1);

            // === 메세지 전송에 실패한 경우 : 클라이언트의 연결 종료 ===
            if (n == -1) {
                printf("Client disconnected: %s\n", current->client_name);
                remove_client(current);
            }
        }
        else {
            // === 메세지를 보낸 클라이언트에게 Success 메세지 전송 ===
            ssize_t n = write(current->client_pd, SUCCESS_MESSAGE, strlen(SUCCESS_MESSAGE)+1);
            if (n == -1) {
                printf("Client disconnected: %s\n", current->client_name);
                remove_client(current);
            }
        }
        current = current->next;
    }
}

int main() {
    // === SIGPIPE 무시 ===
    signal(SIGPIPE, SIG_IGN);

    printf("==== Server Started ====\n");

    // === Server 파이프 생성 ===
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo: create server pipe");
        exit(EXIT_FAILURE);
    }

    // === Server 파이프 열기 ===
    int server_pd = open(PIPE_NAME, O_RDONLY);
    if (server_pd == -1) {
        perror("open: open server pipe");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char message[MAX_LINE];
        
        memset(message, 0, sizeof(message));
        int n = read(server_pd, message, MAX_LINE);

        // === 클라이언트가 연결 요청을 보낸 경우 ===
        if (strstr(message, "connect") != NULL) {
            char *token = strtok(message, ":");
            char *client_name = strtok(NULL, ":");

            // === 클라이언트 파이프 열기 ===
            int client_pd = open(client_name, O_WRONLY);
            if (client_pd == -1) {
                perror("open: open client pipe");
                exit(EXIT_FAILURE);
            }

            // === 클라이언트 정보 저장 ===
            add_client(client_name, client_pd);
        }
        // === 클라이언트가 메시지를 보낸 경우 ===
        else {
            // === 메시지 처리 ===
            handle_client_message(message);
        }
    }
}