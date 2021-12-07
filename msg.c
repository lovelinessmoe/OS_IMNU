//
// Created by loveliness on 2021/11/16.
//

#include <stdio.h>
#include <zconf.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <string.h>

//可以理解为一个消息队列的id
#define  MSGKEY  75
//msgrec的type值，这里写死
#define  MSGTYP  114
struct msgform {
    long mtype;
    char mtext[512];
} msg_server, msg_client;
int msgqid;

/**
 * 软中断的方法
 * 清除其他的消息队列
 * @return
 */
cleanup() {
    msgctl(msgqid, IPC_RMID, 0);
    exit(1);
}

int main() {
    int process1 = fork();
    if (process1 == 0) {
        //当前是服务端
        for (int i = 0; i < 20; i++) {
            //软中断处理
            signal(i, cleanup);
        }
        //建立与顾客进程相同的消息队列
        msgqid = msgget(MSGKEY, 0777 | IPC_CREAT);
        while (1) {
            //接收来自顾客进程的消息
            //对应type是1
            msgrcv(msgqid, &msg_server, 512, 1, 0);
            printf("server : receive %s\n", msg_server.mtext);
            //更改type值，发送给客户端type是114
            msg_server.mtype = MSGTYP;
            //更改消息
            strcpy(msg_server.mtext, "4321");
            //发送应答消息
            msgsnd(msgqid, &msg_server, sizeof(int), 0);
        }
    } else {
        //父进程
        int process2 = fork();
        if (process2 == 0) {
            //当前是客户端
            //建立消息队列
            msgqid = msgget(MSGKEY, 0777);
            //更改要发送的消息
            strcpy(msg_client.mtext, "1234");
            //给服务端发送type是1的消息
            msg_client.mtype = 1;
            msgsnd(msgqid, &msg_client, sizeof(int), 0);
            //接收来自服务进程的消息，type是114
            msgrcv(msgqid, &msg_client, 512, MSGTYP, 0);
            printf("client : receive %s\n", msg_client.mtext);
        } else {
            wait(&process1);
            wait(&process2);
        }
    }
}

