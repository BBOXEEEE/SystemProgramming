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
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_CMD_SIZE (128) 

// === 환경 변수 검색을 위한 전역변수 선언
extern char **environ;

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
        if (last_slash != NULL) {
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

// === uid를 이용해 username을 반환하는 함수 ===
static const char *get_user_name(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (!pw) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    return pw->pw_name;
}

// === gid를 이용해 group name을 반환하는 함수 ===
static const char *get_group_name(gid_t gid) {
    struct group *gr = getgrgid(gid);
    if (!gr) {
        perror("getgrgid");
        exit(EXIT_FAILURE);
    }
    return gr->gr_name;
}

// === tm 구조체를 이용한 시간 정보 변환하는 함수 ===
// int 형 시간 정보를 2023-11-08 과 같은 형태로 변환
static const char *get_time(time_t time) {
    struct tm *tm = localtime(&time);
    if (!tm) {
        perror("localtime");
        exit(EXIT_FAILURE);
    }

    char *str = (char *)malloc(20);
    if (!str) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    snprintf(str, 20, "%d-%02d-%02d",
             tm->tm_year + 1900,
             tm->tm_mon + 1,
             tm->tm_mday);

    return str;
}

// === 문자열로 된 접근 권한을 mode_t 타입으로 변환하는 함수 ===
static mode_t parse_permission(const char* mode_token, mode_t curr_mode) {
    mode_t mode = curr_mode;

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

// === 시그널 핸들러 ===
static void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nCtrl+C is ignored\n");
    }
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

    // === 시그널 설정 ===
    signal(SIGINT, sig_handler);

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
            printf("\t\t\t quit : Quit program\n");
            printf("\t\t\t help : Show a list of commands.\n");
            printf("\t\t\t cd : Change the directory -> cd <directory>\n");
            printf("\t\t\t mkdir : Creates a directory -> mkdir <directory>\n");
            printf("\t\t\t rmdir : Remove the directory -> rmdir <directory>\n");
            printf("\t\t\t rename : Rename the source directory -> rename <source> <target>\n");
            printf("\t\t\t ls : Shows the contents of the current directory.\n");
            printf("\t\t\t ln : Create a link file -> ln [-s] <source> <target>\n");
            printf("\t\t\t rm : Remove the file -> rm <file>\n");
            printf("\t\t\t chmod : Change the file permission -> chmod <permission> <file>\n");
            printf("\t\t\t cat : Show the contents of the file -> cat <file>\n");
            printf("\t\t\t cp : Copy the file -> cp <source> <target>\n");
            printf("\t\t\t ps : Show the process list -> ps\n");
            printf("\t\t\t env : Show the environment variable list -> env\n");
            printf("\t\t\t echo : Show the string or environment variable -> echo <string> or echo $<variable>\n");
            printf("\t\t\t <variable>=<value> : Export the environment variable -> export <variable>=<value>\n");
            printf("\t\t\t unset : Unset the environment variable -> unset <variable>\n");
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

            if (last_slash) {
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

            if (last_slash) {
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

                printf("%-7s %-12s %-4d %-8s %-10s %-12s %-12s %-12s %-8d %s\n",
                       get_type_str(entry->d_type),
                       mode_to_str(file_info.st_mode),
                       (int)file_info.st_nlink,
                       get_user_name(file_info.st_uid),
                       get_group_name(file_info.st_gid),
                       get_time(file_info.st_atime),
                       get_time(file_info.st_mtime),
                       get_time(file_info.st_ctime),
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

            if (last_slash) {
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

            if (last_slash) {
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

            if (last_slash) {
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
               // === 메모리 맵핑과 memcpy()를 통한 메모리 복사 방식의 cp
                int origin_fd = open(origin_absolute_path, O_RDONLY);
                if (origin_fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                // === 원본 파일의 크기를 구하기 위해 stat 사용 ===
                struct stat statbuf;
                if (stat(origin_absolute_path, &statbuf) == -1) {
                    perror("stat");
                    exit(EXIT_FAILURE);
                }

                // === 원본 파일의 크기만큼 메모리 맵핑 ===
                char *origin_addr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, origin_fd, 0);
                if (origin_addr == MAP_FAILED) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
                }

                // === 복사할 파일 생성 ===
                char *target_absolute_path_with_filename = (char *)malloc(strlen(target_absolute_path) + strlen(filename) + 2);
                if(!target_absolute_path_with_filename) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(target_absolute_path_with_filename, target_absolute_path);
                strcat(target_absolute_path_with_filename, "/");
                strcat(target_absolute_path_with_filename, filename);

                int target_fd = open(target_absolute_path_with_filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
                if (target_fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                // === ftruncate()를 통해 복사할 파일의 크기를 원본 파일의 크기로 설정 ===
                if (ftruncate(target_fd, statbuf.st_size) == -1) {
                    perror("ftruncate");
                    exit(EXIT_FAILURE);
                }

                // === 복사할 파일의 크기만큼 메모리 맵핑 ===
                char *target_addr = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, target_fd, 0);
                if (target_addr == MAP_FAILED) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
                }

                // === 메모리 복사 ===
                memcpy(target_addr, origin_addr, statbuf.st_size);

                // === 메모리 맵핑 해제 ===
                if (munmap(origin_addr, statbuf.st_size) == -1) {
                    perror("munmap");
                    exit(EXIT_FAILURE);
                }
                if (munmap(target_addr, statbuf.st_size) == -1) {
                    perror("munmap");
                    exit(EXIT_FAILURE);
                }

                printf("Success cp file. %s -> %s\n", origin, target);
                close(origin_fd);
                close(target_fd);
            }
        }
        // /proc 디렉토리 내용을 읽어서 프로세스 목록을 출력하는 ps 기능 구현
        else if (strcmp(tok_str, "ps") == 0) {
            DIR *dir;
            struct dirent *entry;

            // === /proc 디렉토리 열기 ===
            dir = opendir("/proc");
            if (dir == NULL) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            printf("%-8s %-30s %s\n", "PID", "P_NAME", "CMDLINE");
            printf("===============================================================\n");

            while ((entry = readdir(dir)) != NULL) {
                // === 디렉토리가 아니라면 다음 entry로 ===
                if (entry->d_type != DT_DIR) {
                    continue;
                }
                size_t len = strlen(entry->d_name);

                int is_digit = 1;
                for (size_t i=0; i<len; ++i) {
                    // === 디렉토리 이름이 숫자가 아니라면 다음 entry로 ===
                    if (!isdigit(entry->d_name[i])) {
                        is_digit = 0;
                        break;
                    }
                }

                if (is_digit) {
                    // === 출력해야하는 정보 -> PID, 프로세스 이름, cmdline ===
                    int pid;
                    char comm[100];
                    char cmdline[100];

                    // === 파일 경로 설정 ===
                    char cmdline_path[PATH_MAX];
                    char stat_path[PATH_MAX];
                    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", entry->d_name);
                    snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", entry->d_name);

                    // === stat 파일 열기 ===
                    FILE *stat_fp = fopen(stat_path, "r");
                    if (stat_fp != NULL) {
                        fscanf(stat_fp, "%d (%99[^)])", &pid, comm);
                        fclose(stat_fp);

                        // === cmdline 파일 열기 ===
                        FILE *cmdline_fp = fopen(cmdline_path, "r");
                        if (cmdline_fp != NULL) {
                            fscanf(cmdline_fp, "%s", cmdline);
                            fclose(cmdline_fp);
                        }

                        // === 출력 ===
                        printf("%-8d %-30s %s\n", pid, comm, cmdline);
                    }
                }
            }
            closedir(dir);
        }
        // === Command : env ===
        else if (strcmp(tok_str, "env") == 0) {
            char **env = environ;
            while (*env) {
                printf("%s\n", *env);
                env++;
            }
        }
        // === Command : echo ===
        else if (strcmp(tok_str, "echo") == 0) {
            char *token = strtok(NULL, " \n");
            
            if (token[0] == '$') {
                char *env_name = token + 1;
                char *env_value = getenv(env_name);
                if (env_value) {
                    printf("%s\n", env_value);
                }
                else {
                    printf("No such environment variable\n");
                }
            } else {
                printf("%s\n", token);
            }
        }
        // === Command : unser ===
        else if (strcmp(tok_str, "unset") == 0) {
            char *env_name = strtok(NULL, " \n");
            if (unsetenv(env_name) == 0) {
                printf("Success unset environment variable. %s\n", env_name);
            }
            else {
                perror("unsetenv");
                exit(EXIT_FAILURE);
            }
        }
        // === 환경변수 등록 ===
        else if (strstr(tok_str, "=") != NULL) {
            char *env_name = strtok(tok_str, "=");
            char *env_value = strtok(NULL, "=");

            if (setenv(env_name, env_value, 1) == 0) {
                printf("Success set environment variable. %s=%s\n", env_name, env_value);
            }
            else {
                perror("setenv");
            }
        }
        // Command : export
        else if (strcmp(tok_str, "export") == 0) {
            char *env = strtok(NULL, " \n");
            char *env_name = strtok(env, "=");
            char *env_value = strtok(NULL, "=");

            if (setenv(env_name, env_value, 1) == 0) {
                printf("Success export environment variable. %s=%s\n", env_name, env_value);
            }
            else {
                perror("setenv");
            }
        }
        // Command : kill
        else if (strcmp(tok_str, "kill") == 0) {
            char *sig_tok = strtok(NULL, " \n");
            int sig = atoi(sig_tok);
            char *pid_tok = strtok(NULL, " \n");
            int pid = atoi(pid_tok);

            if (kill(pid, sig) == 0) {
                printf("Success!\n");
            }
            else {
                perror("kill");
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