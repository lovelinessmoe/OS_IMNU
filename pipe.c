#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    //定义管道
    int filedes[2];
    //创建管道
    int resCode = pipe(filedes);
    if (resCode == -1) {
        printf("创建管道失败");
        return resCode;
    }
    int process = fork();
    if (process == 0) {        //子进程1
        //关闭读通道
        close(filedes[0]);
        char *p = "p1 process is sending data to father.\n";
//        对管道上锁
        lockf(filedes[1], 1, strlen(p));
        //将p写入管道
        write(filedes[1], p, strlen(p));
//        对管道解锁
        lockf(filedes[1], 0, strlen(p));
        close(filedes[1]);
    } else if (process > 0) {
        int process2 = fork();
        if (process2 == 0) {      //子进程2
//        关闭读通道
            close(filedes[0]);
            char *p = "p2 process is sending data to father.\n";
//        对管道上锁
            lockf(filedes[1], 1, strlen(p));
            //将p写入管道
            write(filedes[1], p, strlen(p));
//        对管道解锁
            lockf(filedes[1], 0, strlen(p));
            close(filedes[1]);
        } else if (process2 > 0) {         //父进程
            wait(&process);
            wait(&process2);
            //关闭写通道
            close(filedes[1]);
            //读的缓冲区
            char buf[1024];
            //将管道的字符写入缓冲区
            int len = read(filedes[0], buf, sizeof(buf));
            close(filedes[0]);
            //打印缓冲区内容
            for (int i = 0; i < len; i++) {
                printf("%c", buf[i]);
            }
        } else {
            printf("进程2创建错误");
            return -1;
        }
    } else {
        printf("进程1创建错误");
        return -1;
    }
    return 0;
}

