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
#include <stdbool.h>
#include <ctype.h>

#define MAX_CMD_SIZE (128)

// === 상대경로를 절대경로로 변환해주는 함수 ===
static char *convertToAbsolutePath(char *relative_path) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    char *absolute_path = (char *)malloc(PATH_MAX);
    if (!absolute_path) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (!realpath(relative_path, absolute_path)) {
        perror("realpath");
        free(absolute_path);
    }

    return absolute_path;
}

// === 가능한 경로인지 확인하는 함수, 가상의 root(/tmp/test)보다 상위로 이동하는 것을 제한 ===
static int isPossiblePath(const char *path, const char *ROOT_DIR) {
    return strlen(path) < strlen(ROOT_DIR);
}

static const char *get_type_str(char type) {
    switch (type) {
        case DT_BLK:
            return "BLK";
        case DT_CHR:
            return "CHR";
        case DT_DIR:
            return "DIR";
        case DT_FIFO:
            return "FIFO";
        case DT_LNK:
            return "LNK";
        case DT_REG:
            return "REG";
        case DT_SOCK:
            return "SOCK";
        default: // include DT_UNKNOWN
            return "UNKN";
    }
}

// === 숫자로 나열된 파일의 접근 권한을 문자로 바꿔주는 함수 ===
static const char *mode_to_str(mode_t mode) {
    char *str = (char *)malloc(12);
    if (!str) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char type =
        (S_ISDIR(mode))  ? 'd' :
        (S_ISFIFO(mode)) ? 'p' :
        (S_ISLNK(mode))  ? 'l' : 
        (S_ISSOCK(mode)) ? 's' : 
        (S_ISCHR(mode))  ? 'c' : 
        (S_ISBLK(mode))  ? 'b' : '-';

    snprintf(str, 12, "%c%c%c%c%c%c%c%c%c%c",
             type,
             (mode & S_IRUSR) ? 'r' : '-',
             (mode & S_IWUSR) ? 'w' : '-',
             (mode & S_IXUSR) ? 'x' : '-',
             (mode & S_IRGRP) ? 'r' : '-',
             (mode & S_IWGRP) ? 'w' : '-',
             (mode & S_IXGRP) ? 'x' : '-',
             (mode & S_IROTH) ? 'r' : '-',
             (mode & S_IWOTH) ? 'w' : '-',
             (mode & S_IXOTH) ? 'x' : '-');

    return str;
}

// === 심볼링 링크의 유무룰 확인하여 파일명을 변환하는 함수 ===
static const char *convert_filename(mode_t mode, const char *filename) {
    if (S_ISDIR(mode)) {
        return filename;
    }
    else if (S_ISLNK(mode)) {
        char *buf = (char *)malloc(PATH_MAX);
        if (!buf) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        ssize_t len = readlink(filename, buf, PATH_MAX);
        if (len == -1) {
            perror("readlink");
            exit(EXIT_FAILURE);
        }
        buf[len] = '\0';

        // === 마지막 / 기준으로 분리
        char *last_slash = strrchr(buf, '/');
        // === 경로와 폴더명을 분리 ===
        char *origin_filename;
        if (!last_slash) {
            *last_slash = '\0';
            origin_filename = last_slash + 1;
        }

        char *ret = (char *)malloc(strlen(origin_filename) + strlen(filename) + 5);
        if (!ret) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        strcpy(ret, filename);
        strcat(ret, " -> ");
        strcat(ret, origin_filename);
        return ret;
    }
    else {
        return filename;
    }
}

// === 문자열로 된 접근 권한을 mode_t 타입으로 변환하는 함수 ===
static mode_t parse_permission(const char* mode_token, mode_t curr_mode) {
    mode_t mode = curr_mode;
    printf("%o\n", mode);

    #define true 1
    #define false 0

    char *target;
    char *permission;
    int add_permission;

    // === +- 를 기준으로 대상과 권한을 분리 ===
    char *mode_token_copy = strdup(mode_token);
    target = strtok(mode_token_copy, "+-");
    // === token을 활용해 권한 추가/삭제를 구분 ===
    if(target != NULL) {
        char separator = mode_token[strlen(target)];
        if(separator == '+') {
            add_permission = true;
        } else if(separator == '-') {
            add_permission = false;
        }
    }
    permission = strtok(NULL, "+-");

    // === 대상을 User, Group, Other, All로 구분 ===
    int is_user = false;
    int is_group = false;
    int is_other = false;
    while(*target) {
        switch(*target) {
            case 'u':
                is_user = true;
                break;
            case 'g':
                is_group = true;
                break;
            case 'o':
                is_other = true;
                break;
            case 'a':
                is_user = true;
                is_group = true;
                is_other = true;
                break;
        }
        target++;
    }

    // === 권한의 추가/삭제 그리고 대상에 따라 접근 권한 설정 ===
    while(*permission) {
        if(add_permission) {
            if(*permission == 'r') {
                if(is_user) mode |= S_IRUSR;
                if(is_group) mode |= S_IRGRP;
                if(is_other) mode |= S_IROTH;
            }
            else if(*permission == 'w') {
                if(is_user) mode |= S_IWUSR;
                if(is_group) mode |= S_IWGRP;
                if(is_other) mode |= S_IWOTH;
            }
            else if(*permission == 'x') {
                if(is_user) mode |= S_IXUSR;
                if(is_group) mode |= S_IXGRP;
                if(is_other) mode |= S_IXOTH;
            }
        }
        else {
            if(*permission == 'r') {
                if(is_user) mode &= ~S_IRUSR;
                if(is_group) mode &= ~S_IRGRP;
                if(is_other) mode &= ~S_IROTH;
            }
            else if(*permission == 'w') {
                if(is_user) mode &= ~S_IWUSR;
                if(is_group) mode &= ~S_IWGRP;
                if(is_other) mode &= ~S_IWOTH;
            }
            else if(*permission == 'x') {
                if(is_user) mode &= ~S_IXUSR;
                if(is_group) mode &= ~S_IXGRP;
                if(is_other) mode &= ~S_IXOTH;
            }
        }
        permission++;
    }

    return mode;
}

// === main 함수 ===
int main(int argc, char **argv){
    char *command, *tok_str;
    char *current_dir = "/";

    // === Root Directory 설정 - 사용자마다 username이 다르기 때문에 이를 위한 일반화 ===
    char *ROOT_DIR = NULL;
    char *user_home = getcwd(NULL, BUFSIZ);
    ROOT_DIR = user_home;
    strcat(ROOT_DIR, "/tmp/test");

    // === /tmp/test 유무 확인 후 생성 ===
    if (!(opendir(ROOT_DIR))){
        char *cwd = getcwd(NULL, BUFSIZ);
        strcat(cwd, "/tmp");
        mkdir(cwd, 0755);
        strcat(cwd, "/test");
        mkdir(cwd, 0755);
        printf("success!\n");
    }
    // === 가상의 root : ~/tmp/test ===
    chdir(ROOT_DIR);

    command = (char *)malloc(MAX_CMD_SIZE);
    if (!command){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    printf("⭐️ Hi~!⭐️\n");
    do {
        printf("%s $ ", current_dir);
        if (fgets(command, MAX_CMD_SIZE - 1, stdin) == NULL)
            break;

        tok_str = strtok(command, " \n");
        if (tok_str == NULL)
            continue;

        if (strcmp(tok_str, "quit") == 0) {
            printf("✋Exit the programm. Bye✋\n");
            break;
        }
        // === Command : help ===
        else if (strcmp(tok_str, "help") == 0) {
            printf("\t\t\t\t======= Help =======\n");
            printf("\t\t\t\t quit : Quit program\n");
            printf("\t\t\t\t help : Show a list of commands.\n");
            printf("\t\t\t\t cd : Change the directory -> cd <directory>\n");
            printf("\t\t\t\t mkdir : Creates a directory -> mkdir <directory>\n");
            printf("\t\t\t\t rmdir : Remove the directory -> rmdir <directory>\n");
            printf("\t\t\t\t rename : Rename the source directory -> rename <source> <target>\n");
            printf("\t\t\t\t ls : Shows the contents of the current directory.\n");
            printf("\t\t\t\t ln : Create a link file -> ln [-s] <source> <target>\n");
            printf("\t\t\t\t rm : Remove the file -> rm <file>\n");
            printf("\t\t\t\t chmod : Change the file permission -> chmod <permission> <file>\n");
            printf("\t\t\t\t cat : Show the contents of the file -> cat <file>\n");
            printf("\t\t\t\t cp : Copy the file -> cp <source> <target>\n");
        }
        // === Command : cd ===
        else if (strcmp(tok_str, "cd") == 0) {
            // === 이동하고자 하는 경로 ===
            char *relative_path = strtok(NULL, " \n");

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);

            // === 절대 경로가 '/'(실제 root)일 경우 '~/tmp/test'(가상의 root)로 변경
            if (strcmp(absolute_path, "/") == 0)
            {
                absolute_path = realloc(absolute_path, strlen(ROOT_DIR) + 1);
                absolute_path = ROOT_DIR;
            }

            // === 이동 가능한 경로인지 확인 ===
            if (isPossiblePath(absolute_path, ROOT_DIR) == 1) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                if (chdir(absolute_path) != 0) {
                    perror("chdir");
                }
                else {
                    // === 사용자에게 보여지는 경로(current_dir) 업데이트 ===
                    char *new_current_dir = strdup(absolute_path + strlen(ROOT_DIR));
                    if (new_current_dir == NULL) {
                        perror("strdup");
                        exit(EXIT_FAILURE);
                    }
                    if (strlen(new_current_dir) == 0) {
                        new_current_dir = realloc(new_current_dir, 2);
                        if (new_current_dir == NULL) {
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

            if (!last_slash) {
                *last_slash = '\0';
                relative_path = path;
                folder_name = last_slash + 1;
            }
            else {
                relative_path = "./";
                folder_name = token;
            }

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);
            if (isPossiblePath(absolute_path, ROOT_DIR)) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                // === 절대경로에 폴더명을 추가해 mkdir 명령어 생성 ===
                char *absolute_path_with_foldername = (char *)malloc(strlen(absolute_path) + strlen(folder_name) + 2);
                if (!absolute_path_with_foldername) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(absolute_path_with_foldername, absolute_path);
                strcat(absolute_path_with_foldername, "/");
                strcat(absolute_path_with_foldername, folder_name);
                free(absolute_path);

                if (mkdir(absolute_path_with_foldername, 0755) == 0) {
                    free(absolute_path_with_foldername);
                    printf("Success make directory. %s\n", folder_name);
                }
                else {
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

            if (!last_slash) {
                *last_slash = '\0';
                relative_path = path;
                folder_name = last_slash + 1;
            }
            else {
                relative_path = "./";
                folder_name = token;
            }

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);
            if (isPossiblePath(absolute_path, ROOT_DIR)) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                // === 절대경로에 폴더명을 추가해 rmdir 명령어 생성 ===
                char *absolute_path_with_foldername = (char *)malloc(strlen(absolute_path) + strlen(folder_name) + 2);
                if (!absolute_path_with_foldername) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(absolute_path_with_foldername, absolute_path);
                strcat(absolute_path_with_foldername, "/");
                strcat(absolute_path_with_foldername, folder_name);
                free(absolute_path);

                if (rmdir(absolute_path_with_foldername) == 0) {
                    free(absolute_path_with_foldername);
                    printf("Success remove directory. %s\n", folder_name);
                }
                else {
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
            }
            else {
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
            if (!dir) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            printf("TYPE      MODE     LINK     UID      GID        ATIME       MTIME         CTIME      SIZE      NAME\n");
            printf("---------------------------------------------------------------------------------------------------------\n");
            // === entry 순회하며 파일명과 종류 출력 ===
            while ((entry = readdir(dir)) != NULL) {
                char *name = entry->d_name;

                if (lstat(name, &file_info) == -1){
                    perror("lsstat");
                    exit(EXIT_FAILURE);
                }

                printf("%-7s %-12s %-6d %-8d %-8d %-12d %-12d %-12d %-8d %s\n",
                       get_type_str(entry->d_type),
                       mode_to_str(file_info.st_mode),
                       (int)file_info.st_nlink,
                       (int)file_info.st_uid,
                       (int)file_info.st_gid,
                       (int)file_info.st_atime,
                       (int)file_info.st_mtime,
                       (int)file_info.st_ctime,
                       (int)file_info.st_size,
                       convert_filename(file_info.st_mode, name));
            }
            closedir(dir);
        }
        // === Command : ln===
        else if (strcmp(tok_str, "ln") == 0) {
            char *token = strtok(NULL, " \n");
            char *old_path;
            char *new_path;

            // 심볼링 링크 생성
            if (strcmp(token, "-s") == 0) {
                old_path = strtok(NULL, " \n");
                new_path = strtok(NULL, " \n");
            }
            // 하드 링크 생성
            else {
                old_path = token;
                new_path = strtok(NULL, " \n");
            }

            char path[PATH_MAX];
            strcpy(path, new_path);
            // === 마지막 / 를 기준으로 분리 ===
            char *last_slash = strrchr(path, '/');

            // === 경로와 폴더명을 분리 ===
            char *relative_path = path;
            char *name;

            if (!last_slash) {
                *last_slash = '\0';
                relative_path = path;
                name = last_slash + 1;
            }
            else {
                relative_path = "./";
                name = new_path;
            }

            char *old_absolute_path = convertToAbsolutePath(old_path);
            char *new_absolute_path = convertToAbsolutePath(relative_path);

            if (isPossiblePath(old_absolute_path, ROOT_DIR) || isPossiblePath(new_absolute_path, ROOT_DIR)) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                char *new_absolute_path_with_name = (char *)malloc(strlen(new_absolute_path) + strlen(name) + 2);
                if (!new_absolute_path_with_name) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(new_absolute_path_with_name, new_absolute_path);
                strcat(new_absolute_path_with_name, "/");
                strcat(new_absolute_path_with_name, name);

                // 심볼링 링크 생성
                if (strcmp(token, "-s") == 0) {
                    if (symlink(old_absolute_path, new_absolute_path_with_name) == 0) {
                        printf("Success Create Symbolic Link ... %s -> %s\n", name, old_path);
                    }
                    else {
                        perror("ln");
                    }
                }
                // 하드 링크 생성
                else {
                    if (link(old_absolute_path, new_absolute_path_with_name) == 0) {
                        printf("Suceess Create Hard Link\n");
                    }
                    else {
                        perror("symlink");
                    }
                }
            }
        }
        // === Command : rm===
        else if (strcmp(tok_str, "rm") == 0) {
            // === 삭제하려는 폴더 경로 ===
            char *token = strtok(NULL, " \n");

            char path[MAX_CMD_SIZE];
            strcpy(path, token);
            // === 마지막 / 기준으로 분리
            char *last_slash = strrchr(path, '/');

            // === 경로와 폴더명을 분리 ===
            char *relative_path = path;
            char *file_name;

            if (!last_slash) {
                *last_slash = '\0';
                relative_path = path;
                file_name = last_slash + 1;
            }
            else {
                relative_path = "./";
                file_name = token;
            }

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(relative_path);
            if (isPossiblePath(absolute_path, ROOT_DIR)) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                // === 절대경로에 폴더명을 추가해 unlink 명령어 생성 ===
                char *absolute_path_with_filename = (char *)malloc(strlen(absolute_path) + strlen(file_name) + 2);
                if (!absolute_path_with_filename) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(absolute_path_with_filename, absolute_path);
                strcat(absolute_path_with_filename, "/");
                strcat(absolute_path_with_filename, file_name);
                free(absolute_path);

                if (unlink(absolute_path_with_filename) == 0) {
                    free(absolute_path_with_filename);
                    printf("Success remove file. %s\n", file_name);
                }
                else {
                    perror("unlink");
                }
            }
        }
        // === Command : chmod===
        else if (strcmp(tok_str, "chmod") == 0) {
            char *mode_token = strtok(NULL, " \n");
            char *file_token = strtok(NULL, " \n");

            // === 상대경로를 절대경로로 변환 ===
            char *absolute_path = convertToAbsolutePath(file_token);
            if (isPossiblePath(absolute_path, ROOT_DIR))
            {
                printf("Permission denied: Inaccessible Path\n");
            }
            else
            {
                // 접근 권한 타입 변환
                mode_t mode;
                
                // === 접근 권한이 정수로 입력된 경우 ===
                if(isdigit(mode_token[0])) {
                     mode = strtol(mode_token, NULL, 8);
                }
                // === 문자열인 경우 ===
                else {
                    struct stat statbuf;
                    stat(file_token, &statbuf);
                    mode_t curr_mode = statbuf.st_mode & 07777;

                    mode = parse_permission(mode_token, curr_mode);
                }

                if (chmod(absolute_path, mode) == 0) {
                    printf("Success chmod file. %s\n", file_token);
                }
                else {
                    perror("chmod");
                }
            }
        }
        // === Command : cat===
        else if (strcmp(tok_str, "cat") == 0) {
            char *filename = strtok(NULL, " \n");
            char *absolute_path = convertToAbsolutePath(filename);

            if (isPossiblePath(absolute_path, ROOT_DIR)) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                // === 파일 열기===
                FILE *fp = fopen(absolute_path, "r");
                if (!fp) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }
                
                // === 한줄씩 버퍼로 읽어 화면에 출력 ===
                char buf[BUFSIZ];
                while (fgets(buf, BUFSIZ, fp) != NULL) {
                    printf("%s", buf);
                }

                printf("Success cat file. %s\n", filename);
                fclose(fp);
            }
        }
        // === Command : cp===
        else if (strcmp(tok_str, "cp") == 0) {
            char *origin = strtok(NULL, " \n");
            char *target = strtok(NULL, " \n");

            // target의 경로와 폴더를 분리
            char path[MAX_CMD_SIZE];
            strcpy(path, target);
            // === 마지막 / 기준으로 분리
            char *last_slash = strrchr(path, '/');

            char *target_path = path;
            char *filename;

            if (!last_slash) {
                *last_slash = '\0';
                target_path = path;
                filename = last_slash + 1;
            }
            else {
                target_path = "./";
                filename = target;
            }

            char *origin_absolute_path = convertToAbsolutePath(origin);
            char *target_absolute_path = convertToAbsolutePath(target_path);

            if (isPossiblePath(origin_absolute_path, ROOT_DIR) || isPossiblePath(target_absolute_path, ROOT_DIR)) {
                printf("Permission denied: Inaccessible Path\n");
            }
            else {
                // === 원본 파일 열기 ===
                FILE *origin_file = fopen(origin_absolute_path, "r");
                if (!origin_file) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }
                char *target_absolute_path_with_filename = (char *)malloc(strlen(target_absolute_path) + strlen(filename) + 2);
                if(!target_absolute_path_with_filename) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(target_absolute_path_with_filename, target_absolute_path);
                strcat(target_absolute_path_with_filename, "/");
                strcat(target_absolute_path_with_filename, filename);

                // === 복사할 파일 생성 ===
                FILE *target_file = fopen(target_absolute_path_with_filename, "w");
                if (!target_file) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }

                // === 한줄씩 버퍼에 읽어 복사할 파일에 쓰기 ===
                char buf[BUFSIZ];
                while (fgets(buf, BUFSIZ, origin_file) != NULL) {
                    fputs(buf, target_file);
                }
                printf("Success cp file. %s -> %s\n", origin, target);
                fclose(origin_file);
                fclose(target_file);
            }
        }
        // === Not Support Command ===
        else
        {
            printf("Command not supported!\n");
            printf("Use 'help' to see a list of commands.\n");
        }
    } while (1);

    free(command);

    return 0;
}