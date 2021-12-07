//
// Created by loveliness on 2021/11/16.
//
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>

#define  MSGKEY  75
struct msgform {
    long mtype;
    char mtext[256];
} msg_server;
int msgqid;

int main() {
    printf("sfa");
    int i, pid, *pint;
    extern cleanup();
    for (i = 0; i < 20; i++)    /*软中断处理*/
        signal(i, cleanup);
    msgqid = msgget(MSGKEY, 0777 | IPC_CREAT);  /*建立与顾客进程相同的消息队列*/
    while (1) {
        msgrcv(msgqid, &msg_server, 256, 1, 0);   /* 接收来自顾客进程的消息 */
        pint = (int *) msg_server.mtext;
        pid = *pint;
        printf("server : receive from pid %d\n", pid);
        msg_server.mtype = pid;
        *pint = getpid();
        msgsnd(msgqid, &msg_server, sizeof(int), 0);/*发送应答消息*/
    }
}

cleanup() {
    msgctl(msgqid, IPC_RMID, 0);
    exit(1);
}
