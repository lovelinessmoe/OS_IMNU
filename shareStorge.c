//
// Created by loveliness on 2021/11/23.
//

#include <zconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SIZE 512
#define SHMKEY    75
int shmid;//共享区描述字
char *addr;//系统附接该共享区后的虚拟地址

cleanup() {
    shmctl(shmid, IPC_RMID, 0);
    exit(0);
}

int main() {
    int process1 = fork();
    if (process1 == 0) {
        //当前是1进程
//        软中断处理
        for (int i = 0; i < 20; i++) {
            signal(i, cleanup);
        }
//        shmget(key,size,flag)
        shmid = shmget(SHMKEY, SIZE, 0777 | IPC_CREAT);
//        shmat(shmid,addr,flag)
        addr = shmat(shmid, 0, 0);
        printf("addr 0x%x \n", addr);
        int *pint = (int *) addr;
        for (int i = 0; i < 512; i++) {
            *pint = i;
            pint++;
        }
        pint = (int *) addr;
        //更改第一个字节，使能输出
        *pint = 512;
        pause();  /* 等待接收进程读 */
    } else if (process1 > 0) {
        //当前是父进程
        int process2 = fork();
        if (process2 == 0) {
            //当前是2进程
            wait(&process1);
//            取共享区SHMKEY的id
            shmid = shmget(SHMKEY, SIZE, 0777);
//            连接共享区
            addr = shmat(shmid, 0, 0);
            printf("addr2 0x%x \n", addr);
            int *pint = (int *) addr;
//            共享区的第一个字节为零时，等待
            while (*pint == 0);
            for (int i = 0; i < 512; i++) {
                //打印共享区中内容
                printf("%d\n", *pint);
                pint++;
            }
        } else if (process2 > 0) {
            //当前是父进程
            wait(0);
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

