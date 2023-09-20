/** 시스템 프로그래밍 과제 3
 * 디렉토리 관리 툴 만들기
 * @author : 2019136056 박세현
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define MAX_CMD_SIZE     (128)

// === 상대경로를 절대경로로 변환해주는 함수 ===
char *convertToAbsolutePath(const char *relative_path){
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    char *absolute_path = (char*)malloc(PATH_MAX);
    if (absolute_path == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (realpath(relative_path, absolute_path) == NULL){
        perror("realpath");
        free(absolute_path);
    }

    return absolute_path;
}

// === 가능한 경로인지 확인하는 함수, 가상의 root(/tmp/test)보다 상위로 이동하는 것을 제한 ===
int isPossiblePath(const char *path, const char *ROOT_DIR){
    return strlen(path) < strlen(ROOT_DIR);
}

int main(int argc, char **argv)
{
    char *command, *tok_str;
    char *current_dir = "/";
    
    // === Root Directory 설정 - 사용자마다 username이 다르기 때문에 이를 위한 일반화 ===
    char *ROOT_DIR = NULL;
    char *user_home = getcwd(NULL, BUFSIZ);
    ROOT_DIR = user_home;
    strcat(ROOT_DIR, "/tmp/test");

    // === /tmp/test 유무 확인 후 생성 ===
    if((opendir(ROOT_DIR)) == NULL) {
        char *cwd = getcwd(NULL, BUFSIZ);
        strcat(cwd, "/tmp"); 
        mkdir(cwd, 0755);
        strcat(cwd, "/test");
        mkdir(cwd, 0755);
        printf("success!\n");
    }
    // === 가상의 root : ~/tmp/test ===
    chdir(ROOT_DIR);
    
    command = (char*) malloc(MAX_CMD_SIZE);
    if (command == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    printf("⭐️ Hi~!⭐️\n");
    do {
        printf("%s $ ", current_dir);
        if (fgets(command, MAX_CMD_SIZE-1, stdin) == NULL) break;

        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        if(strcmp(tok_str, "quit") == 0) {
            printf("✋Exit the programm. Bye✋\n");
            break;
        }
        // === Command : help ===
        else if (strcmp(tok_str, "help") == 0) {
            printf("======= Command List =======\n");
            printf("quit : Quit program\n");
            printf("help : Show a list of commands.\n");
            printf("cd <path> : Change the directory to the path entered in path.\n");
            printf("mkdir <path> : Creates a directory corresponding to path.\n");
            printf("rmdir <path> : Remove the directory corresponding to path.\n");
            printf("rename <source> <target> : Rename the source directory to target directory.\n");
            printf("ls : Shows the contents of the current directory.\n");
        }
        // === Command : cd ===
        else if (strcmp(tok_str, "cd") == 0) {
            // === 이동하고자 하는 경로 ===
            char *relative_path = strtok(NULL, " \n");

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);

            // === 절대 경로가 '/'(실제 root)일 경우 '~/tmp/test'(가상의 root)로 변경
            if(strcmp(absolute_path, "/") == 0) {
                absolute_path = realloc(absolute_path, strlen(ROOT_DIR) + 1);
                absolute_path = ROOT_DIR;
            }
        
            // === 이동 가능한 경로인지 확인 ===
            if (isPossiblePath(absolute_path, ROOT_DIR) == 1){
                printf("Permission denied: Inaccessible Path\n");
            } else {
                if (chdir(absolute_path) != 0){
                    perror("chdir");
                } else {
                    // === 사용자에게 보여지는 경로(current_dir) 업데이트 ===
                    char *new_current_dir = strdup(absolute_path + strlen(ROOT_DIR));
                    if (new_current_dir == NULL){
                        perror("strdup");
                        exit(EXIT_FAILURE);
                    }
                    if (strlen(new_current_dir) == 0){
                        new_current_dir = realloc(new_current_dir, 2);
                        if (new_current_dir == NULL){
                            perror("realloc");
                            exit(EXIT_FAILURE);
                        }
                        new_current_dir[0] = '/';
                        new_current_dir[1] = '\0';
                    }
                    current_dir = new_current_dir;

                    printf("Success to Change Directory. %s\n", current_dir);
                }
            }
        }
        // === Command : mkdir ===
        else if (strcmp(tok_str, "mkdir") == 0) {
            // === 폴더 생성 경로 ===
            char *token = strtok(NULL, " \n");
            
            char path[PATH_MAX];
            strcpy(path, token);
            // === 마지막 / 를 기준으로 분리 ===
            char *last_slash = strrchr(path, '/');

            // === 경로와 폴더명을 분리 ===
            char *relative_path = path;
            char *folder_name;

            if(last_slash != NULL) {
                *last_slash = '\0';
                relative_path = path;
                folder_name = last_slash + 1;
            } else {
                relative_path = "./";
                folder_name = token;
            }

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);
            if(isPossiblePath(absolute_path, ROOT_DIR)){
                printf("Permission denied: Inaccessible Path\n");
            } else {
                // === 절대경로에 폴더명을 추가해 mkdir 명령어 생성 ===
                char *absolute_path_with_foldername = (char *)malloc(strlen(absolute_path) + strlen(folder_name) + 2);
                if (absolute_path_with_foldername == NULL){
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(absolute_path_with_foldername, absolute_path);
                strcat(absolute_path_with_foldername, "/");
                strcat(absolute_path_with_foldername, folder_name);
                free(absolute_path);

                if (mkdir(absolute_path_with_foldername, 0755) == 0){
                    free(absolute_path_with_foldername);
                    printf("Success make directory. %s\n", folder_name);
                } else {
                    perror("mkdir");
                }
            }
        } 
        // === Command : rmdir ===
        else if (strcmp(tok_str, "rmdir") == 0) {
            // === 삭제하려는 폴더 경로 ===
            char *token = strtok(NULL, " \n");

            char path[MAX_CMD_SIZE];
            strcpy(path, token);
            // === 마지막 / 기준으로 분리
            char *last_slash = strrchr(path, '/');

            // === 경로와 폴더명을 분리 ===
            char *relative_path = path;
            char *folder_name;

            if(last_slash != NULL) {
                *last_slash = '\0';
                relative_path = path;
                folder_name = last_slash + 1;
            } else {
                relative_path = "./";
                folder_name = token;
            }

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);
            if(isPossiblePath(absolute_path, ROOT_DIR)){
                printf("Permission denied: Inaccessible Path\n");
            } else {
                // === 절대경로에 폴더명을 추가해 rmdir 명령어 생성 ===
                char *absolute_path_with_foldername = (char *)malloc(strlen(absolute_path) + strlen(folder_name) + 2);
                if (absolute_path_with_foldername == NULL){
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(absolute_path_with_foldername, absolute_path);
                strcat(absolute_path_with_foldername, "/");
                strcat(absolute_path_with_foldername, folder_name);
                free(absolute_path);

                if (rmdir(absolute_path_with_foldername) == 0){
                    free(absolute_path_with_foldername);
                    printf("Success remove directory. %s\n", folder_name);
                } else {
                    perror("rmdir");
                }
            }
        } 
        // === Command : rename ===
        else if (strcmp(tok_str, "rename") == 0) {
            char *old_name = strtok(NULL, " \n");
            char *new_name = strtok(NULL, " \n");

            if (rename(old_name, new_name) == 0) {
                printf("Success Rename %s -> %s\n", old_name, new_name);
            } else {
                perror("rename");
            }
        }
        // === Command : ls ===
        else if (strcmp(tok_str, "ls") == 0) {
            DIR *dir;
            struct dirent *entry;
            struct stat file_info;

            // === 현재 경로 저장 ===
            char *path = getcwd(NULL, BUFSIZ);

            dir = opendir(path);
            if (dir == NULL) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            printf("File/Directory Name   Type\n");
            printf("--------------------   ----\n");

            // === entry 순회하며 파일명과 종류 출력 ===
            while((entry = readdir(dir)) != NULL) {
                char *name = entry->d_name;

                if(stat(name, &file_info) == -1){
                    perror("stat");
                    exit(EXIT_FAILURE);
                }

                if(S_ISDIR(file_info.st_mode)){
                    printf("%-20s   Directory\n", name);
                } else if(S_ISREG(file_info.st_mode)){
                    printf("%-20s   File\n", name);
                } else{
                    printf("%-20s   Unknown\n", name);
                }
            }
            closedir(dir);
        }
        // === Not Support Command ===
        else {
            printf("Command not supported!\n");
            printf("Use 'help' to see a list of commands.\n");
        }
    } while(1);

    free(command);

    return 0;
}