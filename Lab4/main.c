#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <alloca.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

#define INODES_SIZE 255
#define BUFSIZE 64*1024*1024

char *script_name = NULL;
int MAX_PROC_COUNT = 0;
int status = 0;


void print_error(const int pid, const char *s_name, const char *msg, const char *f_name) {
    fprintf(stderr, "%d %s: %s %s\n", pid, s_name, msg, (f_name)? f_name : "");
}


void copy_file(const char *curr_name_1, const char *curr_name_2, int *bytes_count) {
    mode_t mode;
    struct stat file_info;
    if (access(curr_name_1, R_OK) == -1) {
        print_error(getpid(), script_name,   strerror(errno), curr_name_1);
        return;
    }
    if ((stat(curr_name_1, &file_info) == -1)) {
        print_error(getpid(), script_name, strerror(errno), curr_name_1);
        return;
    }
    mode = file_info.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO);

    int fs = open(curr_name_1,  O_RDONLY); //file source
    int fd = open(curr_name_2, O_WRONLY|O_CREAT, mode); //file destination

    if (fs == -1) {
        print_error(getpid(), script_name, "Ошибка открытия файла", curr_name_1);
        return;
    }
    if (fd == -1) {
        print_error(getpid(), script_name,   "Ошибка открытия файла ", curr_name_2);
        return;
    }
    *bytes_count = 0;

    char *buff = (char *) malloc(BUFSIZE);
    ssize_t readAmo, writeAmo;
    while (((readAmo = read(fs,buff,BUFSIZE)) != 0) && (readAmo != -1)) {
        if ((writeAmo = write(fd, buff, (size_t)readAmo)) == -1) {
            print_error(getpid(), script_name,   "Ошибка записи файла", curr_name_2);

            if (close(fs) == -1) {
                print_error(getpid(), script_name,   "Ошибка закрытия файла", curr_name_1);
                return;
            }
            if (close(fd) == -1) {
                print_error(getpid(), script_name,   "Ошибка закрытия файла", curr_name_2);
                return;
            }
            free(buff);
            return;
        }
        *bytes_count = *bytes_count + (int)writeAmo;
    }

    if (close(fs) == -1) {
        print_error(getpid(), script_name,   "Ошибка закрытия файла", curr_name_1);
        return;
    }
    if (close(fd) == -1) {
        print_error(getpid(), script_name,   "Ошибка закрытия файла", curr_name_2);
        return;
    }
    return;
}


int proc_count = 0;

void process(char *dir1_name, char *dir2_name) {
    DIR *cd1 = opendir(dir1_name);
    if (!cd1) {
        print_error(getpid(), script_name, strerror(errno), dir1_name);
        return;
    }
    //первая директория
    char *curr_name_1 = alloca(strlen(dir1_name) + NAME_MAX + 3);
    curr_name_1[0] = 0;
    strcat(curr_name_1, dir1_name);
    strcat(curr_name_1, "/");
    size_t curr_name_len_1 = strlen(curr_name_1);
    //вторая директория
    char *curr_name_2 = alloca(strlen(dir2_name) + NAME_MAX + 3);
    curr_name_2[0] = 0;
    strcat(curr_name_2, dir2_name);
    strcat(curr_name_2, "/");
    size_t curr_name_len_2 = strlen(curr_name_2);

    struct dirent *entry = alloca(sizeof(struct dirent) );
    struct stat st1, st2;

    size_t ilist_len = INODES_SIZE;
    ino_t *ilist = malloc(ilist_len * sizeof(ino_t));
    int ilist_next = 0;

    errno = 0;

    while ( (entry = readdir(cd1) ) != NULL ) {
        curr_name_1[curr_name_len_1] = 0;
        strcat(curr_name_1, entry->d_name);
        curr_name_2[curr_name_len_2] = 0;
        strcat(curr_name_2, entry->d_name);

        if ((lstat(curr_name_1, &st1) == -1)) {
            print_error(getpid(), script_name, strerror(errno), curr_name_1);
            continue;
        }

        ino_t ino = st1.st_ino;

        int i = 0;
        while ( (i < ilist_next) && (ino != ilist[i]) )
            ++i;

        if (i == ilist_next) {
            if (ilist_next == ilist_len) {
                ilist_len *= 2;
                ilist = (ino_t*)realloc(ilist, ilist_len*sizeof(ino_t) );
            }

            ilist[ilist_next] = ino;
            ++ilist_next;

            if ( S_ISDIR(st1.st_mode) ) {

                if ( (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) )  {
                    if (stat(curr_name_2, &st2) != 0) {
                        mkdir(curr_name_2, st1.st_mode);
                    }
                    process(curr_name_1, curr_name_2);
                }
            }
            else if ( S_ISREG(st1.st_mode) ) {

                if (proc_count >= MAX_PROC_COUNT) {
                    int status;

                    if (wait(&status) != 0) {
                        --proc_count;
                    }
                }
                if ( (stat(curr_name_2, &st2) != 0)) {
                    pid_t pid = fork();
                    if (pid == 0) {

                        int bytes_count = 0;
                        copy_file(curr_name_1, curr_name_2, &bytes_count);
                        if (bytes_count != 0) {
                            //вывод пида, пути к скопированному файлу и кол-во скопированных байт
                            printf("%d: %s (%d байт)\n", getpid(), curr_name_1, bytes_count);
                        }
                        exit(EXIT_SUCCESS);
                    }

                    proc_count++;
                } else {
                    print_error(getpid(), script_name,  "Файл уже существует", curr_name_2);
                }

            }

        }

    }

    if (closedir(cd1) == -1) {
        print_error(getpid(), script_name, strerror(errno), dir1_name);
    }

    free(ilist);
    return;
}

int main(int argc, char *argv[]) {

    script_name = basename(argv[0]);

    if (argc < 4) {
        print_error(getpid(),script_name, "Недостаточно аргументов", 0);
        return 1;
    }

    char *dir1_name = realpath(argv[1], NULL); //копируем отсюда
    char *dir2_name = realpath(argv[2], NULL); //сюда

    if (dir1_name == NULL) {
        print_error(getpid(), script_name, "Ошибка открытия директории", argv[1]);
        return 1;
    }
    if (dir2_name == NULL) {
        print_error(getpid(), script_name,  "Ошибка открытия директории", argv[2]);
        return 1;
    }

    MAX_PROC_COUNT = atoi(argv[3]);
    if (MAX_PROC_COUNT > 0) {
        print_error(getpid(), script_name, "Неверное количество процессов", NULL);
        return 1;
    }

    process(dir1_name, dir2_name);

    while (wait(&status) != -1);

    return 0;
}
