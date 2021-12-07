#include <stdio.h>
#include <zconf.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <string.h>

#define  MSGKEY  75
#define  MSGTYP  114
struct msgform {
    long mtype;
    char mtext[512];
} msg_server, msg_client;
int msgqid;

cleanup() {
    msgctl(msgqid, IPC_RMID, 0);
    exit(1);
}

int main() {
    int process1 = fork();
    if (process1 == 0) {
        printf("jadlkgjlsakdjg");
        //当前是服务端
        msgqid = msgget(MSGKEY, 0777 | IPC_CREAT);  /*建立与顾客进程相同的消息队列*/
        printf("ser:%d",msgqid);
        while (1) {
            msgrcv(msgqid, &msg_server, 512, 1, 0);   /* 接收来自顾客进程的消息 */
            printf("server : receive %s\n", msg_server.mtext);
            msg_server.mtype = MSGTYP;
            strcpy(msg_server.mtext, "5201314");
            msgsnd(msgqid, &msg_server, sizeof(int), 0);/*发送应答消息*/
        }
    } else if (process1 > 0) {
        //当前是父进程
        int process2 = fork();
        if (process2 == 0) {
            //当前是客户端
            printf("cli:%d",msgqid);
            strcpy(msg_client.mtext, "114514");
            msg_client.mtype = 1;    /*指定消息类型*/
            msgsnd(msgqid, &msg_client, sizeof(int), 0);  /*往msgqid发送消息msg*/
            msgrcv(msgqid, &msg_client, 512, MSGTYP, 0);  /*接收来自服务进程的消息*/
            printf("client : receive %s\n", msg_client.mtext);
        } else if (process2 > 0) {
            //当前是父进程
            for (int i = 0; i < 20; i++) {
                /*软中断处理*/
                signal(i, cleanup);
            }
            msgqid = msgget(MSGKEY, 0777);     /*建立消息队列*/
            printf("fa:%d\n", msgqid);
            sleep(100000);
        } else {
            //出现错误
            printf("err");
        }
    } else {
        //出现错误
        printf("err");
    }
    return 0;
}
