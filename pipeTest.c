//
// Created by loveliness on 2021/11/2.
//

#include <stdio.h>
#include<unistd.h>
#include <sys/wait.h>

void main() {
    int i, r, p1, p2, fd[2];
    char buf[50], s[50];
    pipe(fd);
    p1 = fork();
    if (p1 == 0) {
        lockf(fd[1], 1, 0);
        sprintf(buf, "child process1 is sending \n");
        printf("child process1! \n");
        write(fd[1], buf, 50);
        sleep(5);
        lockf(fd[1], 0, 0);
        exit(0);
    } else {
        p2 = fork();
        if (p2 == 0) {
            lockf(fd[1], 1, 0);
            sprintf(buf, "child process2 is sending! \n");
            printf("child process2! \n");
            write(fd[1], buf, 50);
            sleep(5);
            lockf(fd[1], 0, 0);
            exit(0);
        } else {
            wait(0);
            r = read(fd[0], s, 50) == -1;
            if (r) printf("can't read pipe\n");
            else printf("%s\n", s);
            wait(0);
            if (r) printf("can't read pipe\n");
            else printf("%s\n", s);
            exit(0);
        }
    }
}/* main end */

