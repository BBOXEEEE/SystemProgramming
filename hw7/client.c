#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <signal.h>

#define SERVER_PIPE_NAME "server_pipe"
#define MAX_LINE 256

int main() {
    // === SIGPIPE 무시 ===
    signal(SIGPIPE, SIG_IGN);

    // === 클라이언트 파이프 이름 생성 ===
    char client_pipe_name[50];
    sprintf(client_pipe_name, "client_pipe_%d", getpid());

    printf("==== Client Name : %s ====\n", client_pipe_name);

    // === 클라이언트 파이프 생성 ===ㄴ
    if (mkfifo(client_pipe_name, 0666) == -1) {
        perror("mkfifo: create client pipe");
        exit(EXIT_FAILURE);
    }

    // === 서버 파이프 열기 ===
    int server_fd = open(SERVER_PIPE_NAME, O_WRONLY);
    if (server_fd == -1) {
        perror("open: open server pipe");
        exit(EXIT_FAILURE);
    }

    // === 서버에 연결 요청 보내기 ===
    char connect_message[MAX_LINE];
    sprintf(connect_message, "connect:%s", client_pipe_name);
    write(server_fd, connect_message, strlen(connect_message)+1);

    // === 클라이언트 파이프 열기 ===
    int client_fd = open(client_pipe_name, O_RDONLY);
    if (client_fd == -1) {
        perror("open: open client pipe");
        exit(EXIT_FAILURE);
    }

    while (1) {
       // === fork()로 부모&자식 프로세스 생성 ===
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        // === 자식 프로세스 : 서버로부터 메세지를 읽고, 화면에 출력하는 역할 ===
        if (child_pid == 0) {
            char message[MAX_LINE + 50];
            int n = read(client_fd, message, MAX_LINE + 50);

            // === Server의 연결이 종료된 경우 ===
            if (n == 0) {
                printf("Server disconnected\n");
                exit(EXIT_SUCCESS);
            }

            // === 메세지를 보낸 클라이언트는 Success 메세지 출력, 받은 클라이언트는 메세지를 출력 ===
            if(strstr(message, "Success") != NULL) printf("%s\n", message);
            else printf("\n%s\n", message);
        }
        // === 부모 프로세스 : 사용자의 입력을 서버에 보내는 역할 ===
        else {
            char input[MAX_LINE];
            printf("Enter Message: ");
            fgets(input, MAX_LINE, stdin);

            // === 메세지 형식 만들기 -> client_name : message ===
            char message[MAX_LINE + strlen(client_pipe_name)+2];
            memset(message, 0, sizeof(message));
            sprintf(message, "%s: %s", client_pipe_name, input);

            // === 개행 문자 제거 ===
            size_t len = strlen(message);
            if (len > 0 && message[len - 1] == '\n') {
                message[len - 1] = '\0';
            }

            // === 서버로 메세지 전송 ===
            ssize_t n = write(server_fd, message, strlen(message));
            if(n == -1) {
                printf("Server disconnected\n");
                exit(EXIT_SUCCESS);
            }
            wait(NULL);
        }
    }
}