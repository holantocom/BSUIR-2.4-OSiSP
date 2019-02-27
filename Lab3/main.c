#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define SIGCOUNT 2


static int i = 1;
static int endflag = 0, x = 0, y = 0;
int arrpid[] = {0,0,0,0,0,0,0,0,0,0};

void my_handler(int signum)
{
    usleep(clock());
    if(signum == SIGUSR1) {
        printf("%d процесс, pid: %d, ppid: %d, получил сигнал SIGUSR1, time %ld\n", i, getpid(), getppid(), clock());
        x++;
    }
    if(signum == SIGUSR2) {
        printf("%d процесс, pid: %d, ppid: %d, получил сигнал SIGUSR2, time %ld\n", i, getpid(), getppid(), clock());
        y++;
    }
    if(signum == SIGTERM)
        endflag = 1;

}

int main ()
{
	pid_t pid;
    FILE *filepid;

    pid = fork();

	if (pid == 0) {

		arrpid[1] = getpid();

		for (int j = 2; j <= 6; j++){
		    pid = fork();
		    if(pid == 0){
		        i = j;
		        break;
		    } else if (pid > 0){
		        arrpid[j] = pid;
		    }
        }

        if (i == 6){

            arrpid[6] = getpid();

            for (int j = 7; j <= 8; j++){
                pid = fork();
                if(pid == 0){
                    i = j;
                    break;
                } else if (pid > 0){
                    arrpid[j] = pid;
                }
            }

        }

        printf("pid of %d is %d, parent %d\n", i, getpid(), getppid());

        if(i == 8) {
            arrpid[i] = getpid();
            filepid = fopen("allpids.txt", "wb+");
            fwrite(arrpid, sizeof(pid_t), 9, filepid);
            fclose(filepid);
        } else {
            usleep(10000);
        }

        filepid = fopen ("allpids.txt","rb+");
        fread(&arrpid, sizeof(pid_t), 9, filepid);
        fclose(filepid);


        signal(SIGUSR1, my_handler);
        signal(SIGUSR2, my_handler);
        signal(SIGTERM, my_handler);

        usleep(10000);

        pid_t group;
        int h;

        switch (i) {
            case 1: // 1 -> (2,3,4,5,6)

                setpgid(arrpid[2], arrpid[2]);
                setpgid(arrpid[3], arrpid[2]);
                setpgid(arrpid[4], arrpid[2]);
                setpgid(arrpid[5], arrpid[2]);
                setpgid(arrpid[6], arrpid[2]);
                group = getpgid(arrpid[2]);

                for(h = 1; h <= SIGCOUNT; h++){
                    killpg(group, SIGUSR2);
                    printf(">> %d процесс, pid: %d, ppid: %d, отправил сигнал SIGUSR2 группе %d, time %ld\n", i, getpid(), getppid(), group, clock());
                    usleep(100000);
                }
                killpg(group, SIGTERM);

                break;
            case 6: // 6 -> (7,8)

                setpgid(arrpid[7], arrpid[7]);
                setpgid(arrpid[8], arrpid[7]);
                group = getpgid(arrpid[7]);
                for(h = 1; h <= SIGCOUNT; h++){
                    killpg(group, SIGUSR1);
                    printf(">> %d процесс, pid: %d, ppid: %d, отправил сигнал SIGUSR1 группе %d, time %ld\n", i, getpid(), getppid(), group, clock());
                    usleep(100000);
                }
                killpg(group, SIGTERM);

                break;
            case 8: // 8 -> 1

                for(h = 1; h <= SIGCOUNT; h++) {
                    kill(arrpid[1], SIGUSR2);
                    printf(">> %d процесс, pid: %d, ppid: %d, отправил сигнал SIGUSR2, time %ld\n", i, getpid(), getppid(), clock());
                    usleep(100000);
                }
                kill(arrpid[1], SIGTERM);

                break;

        }

        while(1) if(endflag == 1) break;
        while (wait(0) > 0);

        printf("=== %d процесс, pid: %d, ppid: %d, завершил работу после %d сигнала SIGUSR1 и %d сигнала SIGUSR2\n", i, getpid(), getppid(), x, y);

    } else {

		printf("0 process: %d\n", getpid());
		wait(NULL);

	}

	return (0);
}
