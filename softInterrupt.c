//
// Created by loveliness on 2021/11/9.
//
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

//要发送的信号编号
#define SIGNUM 16

//收到信号之后执行的操作
void printMSG() {
    printf("received p1 signal.");
}

int main() {
    int id1 = getpid();
    printf("main的id%d\n",id1);

    int process1 = fork();
    if (process1 != 0) {//父进程
        //建立信号联系
        signal(SIGNUM, printMSG);
        int id = getpid();
        printf("父进程id%d\n",id);
        wait(&process1);
    } else {//子进程
        int pid = getppid();
        printf("要发给id%d\n", pid);
        //发送信号
        int resCode = kill(pid, SIGNUM);
        if (resCode == -1) {
            printf("error in killProcess");
        }
    }
    return 0;
}
