#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <stdlib.h>

int main() {
    int process1 = fork();
    if (process1 == 0) {
        //当前是1进程
        sleep(1);
        printf("borther1\n");
        /*char str[] = "borther1";
        for (int i = 0; i < strlen(str); i++) {
            sleep((int)random()%5);
            sleep(1);
            printf("%c", str[i]);
        }
        printf("\n");*/
    } else if (process1 > 0) {
        //当前是父进程
        int process2 = fork();
        if (process2 == 0) {
            //当前是2进程
            sleep(2);
            printf("borther2\n");
            /*char str[] = "borther2";
            for (int i = 0; i < strlen(str); i++) {
                sleep((int) random() % 5);
                printf("%c", str[i]);
            }
            printf("\n");*/
        } else if (process2 > 0) {
            //当前是父进程
            sleep(3);
            printf("father\n");
            printf("process1:%d\n", process1);
            printf("process2:%d\n", process2);
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
