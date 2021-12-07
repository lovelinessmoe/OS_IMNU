//
// Created by loveliness on 2021/12/6.
// 1
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef OPERATOR_SYSTEM_EXP4_SIMPLEFS_H
#define OPERATOR_SYSTEM_EXP4_SIMPLEFS_H
#define BLOCK_SIZE      1024
#define BLOCK_NUM       1024
#define DISK_SIZE       1048576
#define SYS_PATH        "./fsfile"
#define END             0xffff  /**< 块结束，FAT 中的一个标志. */
#define FREE            0x0000  /**< 未使用的块，FAT 中的标志. */
#define ROOT            "/"     /**< 根目录名称.*/
#define ROOT_BLOCK_NUM  2       /**< 初始根目录块. */
#define MAX_OPENFILE    10      /**< 同时打开的最大文件数. */
#define NAMELENGTH      32
#define PATHLENGTH      128
#define DELIM           "/"
#define FOLDER_COLOR    "\e[1;32m"
#define DEFAULT_COLOR   "\e[0m"

/**
 * @brief 存储虚拟磁盘信息。
 * 包含块大小、块计数等信息以及有关磁盘的其他一些信息。
 */
typedef struct BLOCK0 {
    char information[200];
    unsigned short root;        /**< 根目录块号. */
    unsigned char *start_block; /**< 第一个数据块的位置. */
} block0;

/**
 * @brief 文件控制块。
 * 存储文件信息描述和当前状态。
 */
typedef struct FCB {
    char filename[8];
    char exname[3];
    unsigned char attribute;    /**< 0：目录或 1：文件。 */
    unsigned char reserve[10];
    unsigned short time;        /**< 文件创建时间. */
    unsigned short date;        /**< 文件创建日期。 */
    unsigned short first;       /**< 文件的第一个块号。 */
    unsigned long length;       /**< 文件的块数。 */
    char free;
} fcb;

/**
 * @brief 档案分配区
 * 记录下一个文件块号。
 * 当值为 0xffff 时，该块是文件的最后一个块。
 */
typedef struct FAT {
    unsigned short id;
} fat;

/**
 * @brief 用户打开文件。
 * 包含文件控制块和当前状态。
 */
typedef struct USEROPEN {
    /** FCB. */
    fcb open_fcb;
    /** Current state. */
    char dir[80];
    int count;
    char fcb_state;
    char free;
} useropen;

/** 全局变量 */
unsigned char *fs_head;         /**< 虚拟磁盘的初始地址。 */
useropen openfile_list[MAX_OPENFILE];   /**< 用户打开的文件数组。 */
int curdir;                     /**< 当前目录的文件描述符。 */
char current_dir[80];           /**< 当前目录名称。 */
unsigned char *start;           /**< 第一个数据块的位置。*/

/** 函数声明 */
int start_sys(void);

int my_format(char **args);

int do_format(void);

int my_cd(char **args);

void do_chdir(int fd);

int my_pwd(char **args);

int my_mkdir(char **args);

int do_mkdir(const char *parpath, const char *dirname);

int my_rmdir(char **args);

void do_rmdir(fcb *dir);

int my_ls(char **args);

void do_ls(int first, char mode);

int my_create(char **args);

int do_create(const char *parpath, const char *filename);

int my_rm(char **args);

void do_rm(fcb *file);

int do_open(char *path);

void do_close(int fd);

int my_exit_sys();

int get_free(int count);

int set_free(unsigned short first, unsigned short length, int mode);

int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attr, unsigned short first,
            unsigned long length,
            char ffree);

unsigned short get_time(struct tm *timeinfo);

unsigned short get_date(struct tm *timeinfo);

fcb *fcb_cpy(fcb *dest, fcb *src);

char *get_abspath(char *abspath, const char *relpath);

int get_useropen();

fcb *find_fcb(const char *path);

fcb *find_fcb_r(char *token, int root);

void init_folder(int first, int second);

void get_fullname(char *fullname, fcb *fcb1);

char *trans_date(char *sdate, unsigned short date);

char *trans_time(char *stime, unsigned short time);

#endif //OPERATOR_SYSTEM_EXP4_SIMPLEFS_H

/**
 * 启动文件系统和初始变量。
 */
int start_sys(void) {
    fs_head = (unsigned char *) malloc(DISK_SIZE);
    memset(fs_head, 0, DISK_SIZE);
    FILE *fp;
    int i;

    if ((fp = fopen(SYS_PATH, "r")) != NULL) {
        fread(fs_head, DISK_SIZE, 1, fp);
        fclose(fp);
    } else {
        printf("系统未初始化，现在安装它并创建系统文件。\n");
        printf("请不要离开程序。\n");
        printf("初始化成功！\n");
        do_format();
    }

    /**< 初始化第一个 openfile 条目。 */
    fcb_cpy(&openfile_list[0].open_fcb, ((fcb *) (fs_head + 5 * BLOCK_SIZE)));
    strcpy(openfile_list[0].dir, ROOT);
    openfile_list[0].count = 0;
    openfile_list[0].fcb_state = 0;
    openfile_list[0].free = 1;
    curdir = 0;

    /**< 初始化另一个 openfile 条目。 */
    fcb *empty = (fcb *) malloc(sizeof(fcb));
    set_fcb(empty, "\0", "\0", 0, 0, 0, 0);
    for (i = 1; i < MAX_OPENFILE; i++) {
        fcb_cpy(&openfile_list[i].open_fcb, empty);
        strcpy(openfile_list[i].dir, "\0");
        openfile_list[i].free = 0;
        openfile_list[i].count = 0;
        openfile_list[i].fcb_state = 0;
    }

    /**< 初始化全局变量。 */
    strcpy(current_dir, openfile_list[curdir].dir);
    start = ((block0 *) fs_head)->start_block;
    free(empty);
    return 0;
}

/**
 * 命令“格式”的条目。
 */
int my_format(char **args) {
    unsigned char *ptr;
    FILE *fp;
    int i;

    /**< 检查参数计数。 */
    for (i = 0; args[i] != NULL; i++);
    if (i > 2) {
        fprintf(stderr, "format: expected argument to \"format\"\n");
        return 1;
    }

    /**< 检查参数值。 */
    if (args[1] != NULL) {
        /**< 用 0 填充。 */
        if (!strcmp(args[1], "-x")) {
            ptr = (unsigned char *) malloc(DISK_SIZE);
            memset(ptr, 0, DISK_SIZE);
            fp = fopen(SYS_PATH, "w");
            fwrite(ptr, DISK_SIZE, 1, fp);
            free(ptr);
            fclose(fp);
        } else {
            fprintf(stderr, "format: expected argument to \"format\"\n");
            return 1;
        }
    }
    do_format();

    return 1;
}

/**
 * 快速格式化文件系统
 * 创建启动块, 文件表和根目录
 */
int do_format(void) {
    unsigned char *ptr = fs_head;
    int i;
    int first, second;
    FILE *fp;

    /**< 初始化启动块. */
    block0 *init_block = (block0 *) ptr;
    strcpy(init_block->information,
           "Disk Size = 1MB, Block Size = 1KB, Block0 in 0, FAT0/1 in 1/3, Root Directory in 5");
    init_block->root = 5;
    init_block->start_block = (unsigned char *) (init_block + BLOCK_SIZE * 7);
    ptr += BLOCK_SIZE;

    /**< 初始化 FAT01。 */
    set_free(0, 0, 2);

    /**< 将 5 个块分配给一个 block0(1) 和两个 fat(2)。 */
    set_free(get_free(1), 1, 0);
    set_free(get_free(2), 2, 0);
    set_free(get_free(2), 2, 0);

    ptr += BLOCK_SIZE * 4;

    /**< 2 块到根目录。 */
    fcb *root = (fcb *) ptr;
    first = get_free(ROOT_BLOCK_NUM);
    set_free(first, ROOT_BLOCK_NUM, 0);
    set_fcb(root, ".", "di", 0, first, BLOCK_SIZE * 2, 1);
    root++;
    set_fcb(root, "..", "di", 0, first, BLOCK_SIZE * 2, 1);
    root++;

    for (i = 2; i < BLOCK_SIZE * 2 / sizeof(fcb); i++, root++) {
        root->free = 0;
    }

    memset(fs_head + BLOCK_SIZE * 7, 'a', 15);
    /**< 回信。 */
    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    fclose(fp);
}

/**
 * 更改当前目录。
 */
int my_cd(char **args) {
    int i;
    int fd;
    char abspath[PATHLENGTH];
    fcb *dir;

    /**< 检查参数计数。 */
    for (i = 0; args[i] != NULL; i++);
    if (i != 2) {
        fprintf(stderr, "cd: expected argument to \"format\"\n");
        return 1;
    }

    /**< 检查参数值。 */
    memset(abspath, '\0', PATHLENGTH);
    get_abspath(abspath, args[1]);
    dir = find_fcb(abspath);
    if (dir == NULL || dir->attribute == 1) {
        fprintf(stderr, "cd: No such folder\n");
        return 1;
    }

    /**< 检查文件夹 fcb 是否存在于 openfile_list 中。 */
    for (i = 0; i < MAX_OPENFILE; i++) {
        if (openfile_list[i].free == 0) {
            continue;
        }

        if (!strcmp(dir->filename, openfile_list[i].open_fcb.filename) &&
            dir->first == openfile_list[i].open_fcb.first) {
            /**< 文件夹已打开。 */
            do_chdir(i);
            return 1;
        }
    }

    /**< 文件夹关闭，打开它并更改当前目录。 */
    if ((fd = do_open(abspath)) > 0) {
        do_chdir(fd);
    }

    return 1;
}

/**
 * 只需执行更改目录操作即可。
 * @param fd 目录的文件描述符。
 */
void do_chdir(int fd) {
    curdir = fd;
    memset(current_dir, '\0', sizeof(current_dir));
    strcpy(current_dir, openfile_list[curdir].dir);
}

/**
 * 显示当前目录。
 */
int my_pwd(char **args) {
    /**< 检查参数计数。 */
    if (args[1] != NULL) {
        fprintf(stderr, "pwd: too many arguments\n");
        return 1;
    }

    printf("%s\n", current_dir);
    return 1;
}

/**
 * 一次创建一个或多个目录。提供一次创建两个或多个目录的功能。如果 par 文件夹不存在，打印错误，其他人将继续。
 */
int my_mkdir(char **args) {
    int i;
    char path[PATHLENGTH];
    char parpath[PATHLENGTH], dirname[NAMELENGTH];
    char *end;

    /**< 检查参数计数。*/
    if (args[1] == NULL) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 1;
    }

    /**< 创建目录 */
    for (i = 1; args[i] != NULL; i++) {
        /**< 将路径拆分为父文件夹和当前子文件夹。*/
        get_abspath(path, args[i]);
        end = strrchr(path, '/');
        if (end == path) {
            strcpy(parpath, "/");
            strcpy(dirname, path + 1);
        } else {
            strncpy(parpath, path, end - path);
            strcpy(dirname, end + 1);
        }

        if (find_fcb(parpath) == NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Parent folder not exists\n", parpath);
            continue;
        }
        if (find_fcb(path) != NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Folder or file exists\n", args[i]);
            continue;
        }

        do_mkdir(parpath, dirname);
    }

    return 1;
}

/**
 * 只需创建目录。
 * @param parpath 要创建的文件夹的 Par 文件夹。
 * @param dirname 要创建的文件夹名称。
 * @return -1 错误，否则返回 0。
 */
int do_mkdir(const char *parpath, const char *dirname) {
    int second = get_free(1);
    int i, flag = 0, first = find_fcb(parpath)->first;
    fcb *dir = (fcb *) (fs_head + BLOCK_SIZE * first);

    /**< 检查空闲FCB. */
    for (i = 0; i < BLOCK_SIZE / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            flag = 1;
            break;
        }
    }
    if (!flag) {
        fprintf(stderr, "mkdir: Cannot create more file in %s\n", parpath);
        return -1;
    }

    /**< 检查空闲空间 */
    if (second == -1) {
        fprintf(stderr, "mkdir: No more space\n");
        return -1;
    }
    set_free(second, 1, 0);

    /**< 设置FCB并且初始化文件夹 */
    set_fcb(dir, dirname, "di", 0, second, BLOCK_SIZE, 1);
    init_folder(first, second);
    return 0;
}

/**
 * 删除一个或多个文件夹一次。
 * @param 要删除的文件夹名称。
 */
int my_rmdir(char **args) {
    int i, j;
    fcb *dir;

    /**< 检查参数计数。 */
    if (args[1] == NULL) {
        fprintf(stderr, "rmdir: missing operand\n");
        return 1;
    }

    /**< Do remove. */
    for (i = 1; args[i] != NULL; i++) {
        if (!strcmp(args[i], ".") || !strcmp(args[i], "..")) {
            fprintf(stderr, "rmdir: cannot remove %s: '.' or '..' is read only \n", args[i]);
            return 1;
        }

        if (!strcmp(args[i], "/")) {
            fprintf(stderr, "rmdir:  Permission denied\n");
            return 1;
        }

        dir = find_fcb(args[i]);
        if (dir == NULL) {
            fprintf(stderr, "rmdir: cannot remove %s: No such folder\n", args[i]);
            return 1;
        }

        if (dir->attribute == 1) {
            fprintf(stderr, "rmdir: cannot remove %s: Is a directory\n", args[i]);
            return 1;
        }

        /**< 检查文件夹 fcb 是否存在于 openfile_list. */
        for (j = 0; j < MAX_OPENFILE; j++) {
            if (openfile_list[j].free == 0) {
                continue;
            }

            if (!strcmp(dir->filename, openfile_list[j].open_fcb.filename) &&
                dir->first == openfile_list[j].open_fcb.first) {
                /**< 文件夹已打开. */
                fprintf(stderr, "rmdir: cannot remove %s: File is open\n", args[i]);
                return 1;
            }
        }

        do_rmdir(dir);
    }
    return 1;
}

/**
 * 只需删除目录。
 */
void do_rmdir(fcb *dir) {
    int first = dir->first;

    dir->free = 0;
    dir = (fcb *) (fs_head + BLOCK_SIZE * first);
    dir->free = 0;
    dir++;
    dir->free = 0;

    set_free(first, 1, 1);
}

/**
 * 展示文件夹的所有内容
 * @param args 空以显示当前文件夹。 '-l' 以长格式显示。 'path' 来显示一个特定的文件夹
 */
int my_ls(char **args) {
    int first = openfile_list[curdir].open_fcb.first;
    int i, mode = 'n';
    int flag[3];
    fcb *dir;

    /**< Check argument count. */
    for (i = 0; args[i] != NULL; i++) {
        flag[i] = 0;
    }
    if (i > 3) {
        fprintf(stderr, "ls: expected argument\n");
        return 1;
    }

    flag[0] = 1;
    for (i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            flag[i] = 1;
            if (!strcmp(args[i], "-l")) {
                mode = 'l';
                break;
            } else {
                fprintf(stderr, "ls: wrong operand\n");
                return 1;
            }
        }
    }

    for (i = 1; args[i] != NULL; i++) {
        if (flag[i] == 0) {
            dir = find_fcb(args[i]);
            if (dir != NULL && dir->attribute == 0) {
                first = dir->first;
            } else {
                fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", args[i]);
                return 1;
            }
            break;
        }
    }

    do_ls(first, mode);

    return 1;
}

/**
 * 只是做ls。
 * @param first 您要显示的第一个文件夹块。
 * @param mode 'n' 为普通格式，'l' 为长格式。
 */
void do_ls(int first, char mode) {
    int i, count, length = BLOCK_SIZE;
    char fullname[NAMELENGTH], date[16], time[16];
    fcb *root = (fcb *) (fs_head + BLOCK_SIZE * first);
    block0 *init_block = (block0 *) fs_head;
    if (first == init_block->root) {
        length = ROOT_BLOCK_NUM * BLOCK_SIZE;
    }

    if (mode == 'n') {
        for (i = 0, count = 1; i < length / sizeof(fcb); i++, root++) {


            if (root->free == 0) {
                continue;
            }

            if (root->attribute == 0) {
                printf("%s", FOLDER_COLOR);
                printf("%s\t", root->filename);
                printf("%s", DEFAULT_COLOR);
            } else {
                get_fullname(fullname, root);
                printf("%s\t", fullname);
            }
            if (count % 5 == 0) {
                printf("\n");
            }
            count++;
        }
    } else if (mode == 'l') {
        for (i = 0, count = 1; i < length / sizeof(fcb); i++, root++) {
            /**< 检查是否使用了 fcb. */
            if (root->free == 0) {
                continue;
            }

            trans_date(date, root->date);
            trans_time(time, root->time);
            get_fullname(fullname, root);
            printf("%d\t%6d\t%6ld\t%s\t%s\t", root->attribute, root->first, root->length, date, time);
            if (root->attribute == 0) {
                printf("%s", FOLDER_COLOR);
                printf("%s\n", fullname);
                printf("%s", DEFAULT_COLOR);
            } else {
                printf("%s\n", fullname);
            }
            count++;
        }
    }
    printf("\n");
}

/**
 * 一次创建一个或多个文件。
 * @param args 要创建的文件名。
 * @return 总是 1。
 */
int my_create(char **args) {
    int i;
    char path[PATHLENGTH];
    char parpath[PATHLENGTH], filename[NAMELENGTH];
    char *end;

    /**< 检查参数计数。 */
    if (args[1] == NULL) {
        fprintf(stderr, "create: missing operand\n");
        return 1;
    }

    memset(parpath, '\0', PATHLENGTH);
    memset(filename, '\0', NAMELENGTH);

    for (i = 1; args[i] != NULL; i++) {
        /**< 拆分父文件夹和文件名. */
        get_abspath(path, args[i]);
        end = strrchr(path, '/');
        if (end == path) {
            strcpy(parpath, "/");
            strcpy(filename, path + 1);
        } else {
            strncpy(parpath, path, end - path);
            strcpy(filename, end + 1);
        }

        if (find_fcb(parpath) == NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Parent folder not exists\n", parpath);
            continue;
        }
        if (find_fcb(path) != NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Folder or file exists\n", args[i]);
            continue;
        }

        do_create(parpath, filename);
    }

    return 1;
}

/**
 * 只需创建文件。
 * @param parpath 文件 par 文件夹。
 * @param 文件名 文件名。
 * @return 错误为 -1，否则返回 0。
 */
int do_create(const char *parpath, const char *filename) {
    char fullname[NAMELENGTH], fname[16], exname[8];
    char *token;
    int first = get_free(1);
    int i, flag = 0;
    fcb *dir = (fcb *) (fs_head + BLOCK_SIZE * find_fcb(parpath)->first);

    for (i = 0; i < BLOCK_SIZE / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            flag = 1;
            break;
        }
    }
    if (!flag) {
        fprintf(stderr, "create: Cannot create more file in %s\n", parpath);
        return -1;
    }

    /**< 检查可用空间. */
    if (first == -1) {
        fprintf(stderr, "create: No more space\n");
        return -1;
    }
    set_free(first, 1, 0);


    memset(fullname, '\0', NAMELENGTH);
    memset(fname, '\0', 8);
    memset(exname, '\0', 3);
    strcpy(fullname, filename);
    token = strtok(fullname, ".");
    strncpy(fname, token, 8);
    token = strtok(NULL, ".");
    if (token != NULL) {
        strncpy(exname, token, 3);
    } else {
        strncpy(exname, "d", 2);
    }


    set_fcb(dir, fname, exname, 1, first, 0, 1);

    return 0;
}

/**
 * 删除文件。
 * @param args 要删除的文件名。
 * @return 总是返回 1。
 */
int my_rm(char **args) {
    int i, j;
    fcb *file;

    if (args[1] == NULL) {
        fprintf(stderr, "rm: missing operand\n");
        return 1;
    }

    for (i = 1; args[i] != NULL; i++) {
        file = find_fcb(args[i]);
        if (file == NULL) {
            fprintf(stderr, "rm: cannot remove %s: No such file\n", args[i]);
            return 1;
        }

        if (file->attribute == 0) {
            fprintf(stderr, "rm: cannot remove %s: Is a directory\n", args[i]);
            return 1;
        }

        for (j = 0; j < MAX_OPENFILE; j++) {
            if (openfile_list[j].free == 0) {
                continue;
            }

            if (!strcmp(file->filename, openfile_list[j].open_fcb.filename) &&
                file->first == openfile_list[j].open_fcb.first) {
                fprintf(stderr, "rm: cannot remove %s: File is open\n", args[i]);
                return 1;
            }
        }

        do_rm(file);
    }

    return 1;
}

/**
 * 只需删除文件。
 * @param file FCB 指针要删除的文件。
 */
void do_rm(fcb *file) {
    int first = file->first;

    file->free = 0;
    set_free(first, 0, 1);
}

/**
 * 只需打开文件。
 * @param path 要打开的文件的绝对路径..
 * @return 错误为 -1，否则返回 fd;
 */
int do_open(char *path) {
    int fd = get_useropen();
    fcb *file = find_fcb(path);

    if (fd == -1) {
        fprintf(stderr, "open: cannot open file, no more useropen entry\n");
        return -1;
    }
    fcb_cpy(&openfile_list[fd].open_fcb, file);
    openfile_list[fd].free = 1;
    openfile_list[fd].count = 0;
    memset(openfile_list[fd].dir, '\0', 80);
    strcpy(openfile_list[fd].dir, path);

    return fd;
}


/**
 * 只需关闭文件。
 * @param fd 文件描述符。
 */
void do_close(int fd) {
    if (openfile_list[fd].fcb_state == 1) {
        fcb_cpy(find_fcb(openfile_list[fd].dir), &openfile_list[fd].open_fcb);
    }
    openfile_list[fd].free = 0;
}

/**
 * 退出系统，保存更改
 */
int my_exit_sys(void) {
    int i;
    FILE *fp;

    for (i = 0; i < MAX_OPENFILE; i++) {
        do_close(i);
    }

    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    free(fs_head);
    fclose(fp);
    return 0;
}

/**
 * 检测 FAT 中的空闲块。
 * @param count 所需块的数量。
 * @return 0 没有足够的空间，否则返回第一个块号。
 */
int get_free(int count) {
    unsigned char *ptr = fs_head;
    fat *fat0 = (fat *) (ptr + BLOCK_SIZE);
    int i, j, flag = 0;
    int fat[BLOCK_NUM];

    /** 复制FAT。 */
    for (i = 0; i < BLOCK_NUM; i++, fat0++) {
        fat[i] = fat0->id;
    }

    /** 找一个连续的空间。*/
    for (i = 0; i < BLOCK_NUM - count; i++) {
        for (j = i; j < i + count; j++) {
            if (fat[j] > 0) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            flag = 0;
            i = j;
        } else {
            return i;
        }
    }

    return -1;
}

/**
 * 改变 FAT 的值。
 * @param first 起始块号。
 * @param length 块数。
 * @param mode 0 分配，1 回收，2 格式化。
 */
int set_free(unsigned short first, unsigned short length, int mode) {
    fat *flag = (fat *) (fs_head + BLOCK_SIZE);
    fat *fat0 = (fat *) (fs_head + BLOCK_SIZE);
    fat *fat1 = (fat *) (fs_head + BLOCK_SIZE * 3);
    int i;
    int offset;

    for (i = 0; i < first; i++, fat0++, fat1++);

    if (mode == 1) {
        /**< 回收空间。 */
        while (fat0->id != END) {
            offset = fat0->id - (fat0 - flag) / sizeof(fat);
            fat0->id = FREE;
            fat1->id = FREE;
            fat0 += offset;
            fat1 += offset;
        }
        fat0->id = FREE;
        fat1->id = FREE;
    } else if (mode == 2) {
        /**< 格式化FAT */
        for (i = 0; i < BLOCK_NUM; i++, fat0++, fat1++) {
            fat0->id = FREE;
            fat1->id = FREE;
        }
    } else {
        /**< 分配连续空间. */
        for (; i < first + length - 1; i++, fat0++, fat1++) {
            fat0->id = first + 1;
            fat1->id = first + 1;
        }
        fat0->id = END;
        fat1->id = END;
    }

    return 0;
}

/**
 * 设置 fcb 属性。
 * @param f fcb 的指针。
 * @param 文件名 FCB 文件名。
 * @param exname FCB 文件扩展名。
 * @param attr FCB 文件属性。
 * @param 第一个 FCB 起始块号。
 * @param length FCB 文件长度。
 * @param ffree 文件被占用时为 1，否则为 0。
 */
int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attr, unsigned short first,
            unsigned long length, char ffree) {
    time_t *now = (time_t *) malloc(sizeof(time_t));
    struct tm *timeinfo;
    time(now);
    timeinfo = localtime(now);

    memset(f->filename, 0, 8);
    memset(f->exname, 0, 3);
    strncpy(f->filename, filename, 7);
    strncpy(f->exname, exname, 2);
    f->attribute = attr;
    f->time = get_time(timeinfo);
    f->date = get_date(timeinfo);
    f->first = first;
    f->length = length;
    f->free = ffree;

    free(now);
    return 0;
}

/**
 * 将 ISO 时间转换为短时间。
 * @param timeinfo 当前时间结构。
 * @return 翻译后的时间编号。
 */
unsigned short get_time(struct tm *timeinfo) {
    int hour, min, sec;
    unsigned short result;

    hour = timeinfo->tm_hour;
    min = timeinfo->tm_min;
    sec = timeinfo->tm_sec;
    result = (hour << 11) + (min << 5) + (sec >> 1);

    return result;
}

/**
 * 将 ISO 日期转换为短日期。
 * @param timeinfo local
 * @return 翻译后的日期编号。
 */
unsigned short get_date(struct tm *timeinfo) {
    int year, mon, day;
    unsigned short result;

    year = timeinfo->tm_year;
    mon = timeinfo->tm_mon;
    day = timeinfo->tm_mday;
    result = (year << 9) + (mon << 5) + day;

    return result;
}

/**
 * 复制一个 fcb。
 * @param dest 目的地 fcb。
 * @param src 源 fcb。
 * @return 目标 fcb 指针。
 */
fcb *fcb_cpy(fcb *dest, fcb *src) {
    memset(dest->filename, '\0', 8);
    memset(dest->exname, '\0', 3);

    strcpy(dest->filename, src->filename);
    strcpy(dest->exname, src->exname);
    dest->attribute = src->attribute;
    dest->time = src->time;
    dest->date = src->date;
    dest->first = src->first;
    dest->length = src->length;
    dest->free = src->free;

    return dest;
}

/**
 * 将相对路径转换为绝对路径
 * @param abspath 绝对路径。
 * @param relpath 相对路径。
 * @return 绝对路径。
 */
char *get_abspath(char *abspath, const char *relpath) {
    /**< 如果 relpath 是绝对路径。 */
    if (!strcmp(relpath, DELIM) || relpath[0] == '/') {
        strcpy(abspath, relpath);
        return 0;
    }

    char str[PATHLENGTH];
    char *token, *end;

    memset(abspath, '\0', PATHLENGTH);
    abspath[0] = '/';
    strcpy(abspath, current_dir);

    strcpy(str, relpath);
    token = strtok(str, DELIM);

    do {
        if (!strcmp(token, ".")) {
            continue;
        }
        if (!strcmp(token, "..")) {
            if (!strcmp(abspath, ROOT)) {
                continue;
            } else {
                end = strrchr(abspath, '/');
                if (end == abspath) {
                    strcpy(abspath, ROOT);
                    continue;
                }
                memset(end, '\0', 1);
                continue;
            }
        }
        if (strcmp(abspath, "/")) {
            strcat(abspath, DELIM);
        }
        strcat(abspath, token);
    } while ((token = strtok(NULL, DELIM)) != NULL);

    return abspath;
}

/**
 * 通过 abspath 查找 fcb。
 * @param path 文件路径。
 * @return 文件 fcb 指针。
 */
fcb *find_fcb(const char *path) {
    char abspath[PATHLENGTH];
    get_abspath(abspath, path);
    char *token = strtok(abspath, DELIM);
    if (token == NULL) {
        return (fcb *) (fs_head + BLOCK_SIZE * 5);
    }
    return find_fcb_r(token, 5);
}

/**
 * 递归查找 fcb 的过程。
 * @param token (ptr) 中的文件名。
 * @param first Par fcb 指针。
 * @return 令牌的 FCB 指针。
 */
fcb *find_fcb_r(char *token, int first) {
    int i, length = BLOCK_SIZE;
    char fullname[NAMELENGTH] = "\0";
    fcb *root = (fcb *) (BLOCK_SIZE * first + fs_head);
    fcb *dir;
    block0 *init_block = (block0 *) fs_head;
    if (first == init_block->root) {
        length = ROOT_BLOCK_NUM * BLOCK_SIZE;
    }

    for (i = 0, dir = root; i < length / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            continue;
        }
        get_fullname(fullname, dir);
        if (!strcmp(token, fullname)) {
            token = strtok(NULL, DELIM);
            if (token == NULL) {
                return dir;
            }
            return find_fcb_r(token, dir->first);
        }
    }
    return NULL;
}

/**
 * 获取一个空的 useropen 条目。
 * @return 如果空 useropen 存在返回条目索引，否则返回 -1；
 */
int get_useropen() {
    int i;

    for (i = 0; i < MAX_OPENFILE; i++) {
        if (openfile_list[i].free == 0) {
            return i;
        }
    }

    return -1;
}

/**
 * 初始化一个文件夹。
 * @param first 父文件夹块编号。
 * @param second 当前文件夹块编号。
 */
void init_folder(int first, int second) {
    int i;
    fcb *par = (fcb *) (fs_head + BLOCK_SIZE * first);
    fcb *cur = (fcb *) (fs_head + BLOCK_SIZE * second);

    set_fcb(cur, ".", "di", 0, second, BLOCK_SIZE, 1);
    cur++;
    set_fcb(cur, "..", "di", 0, first, par->length, 1);
    cur++;
    for (i = 2; i < BLOCK_SIZE / sizeof(fcb); i++, cur++) {
        cur->free = 0;
    }
}

/**
 * 获取文件全名。
 * @param fullname 一个字符数组[NAMELENGTH]。
 * @param fcb1 源 fcb 指针。
 */
void get_fullname(char *fullname, fcb *fcb1) {
    memset(fullname, '\0', NAMELENGTH);

    strcat(fullname, fcb1->filename);
    if (fcb1->attribute == 1) {
        strncat(fullname, ".", 2);
        strncat(fullname, fcb1->exname, 3);
    }
}

/**
 * 将无符号短数字转换为日期字符串。
 * @param sdate 日期到字符串。
 * @param date 表示日期的数字。
 * @return 日期。
 */
char *trans_date(char *sdate, unsigned short date) {
    int year, month, day;
    memset(sdate, '\0', 16);

    year = date & 0xfe00;
    month = date & 0x01e0;
    day = date & 0x001f;
    sprintf(sdate, "%04d-%02d-%02d", (year >> 9) + 1900, (month >> 5) + 1, day);
    return sdate;
}

/**
 * 将无符号短数转换为时间字符串。
 * @param stime 字符串时间。
 * @param time 代表时间的数字。
 * @return 时间。
 */
char *trans_time(char *stime, unsigned short time) {
    int hour, min, sec;
    memset(stime, '\0', 16);

    hour = time & 0xfc1f;
    min = time & 0x03e0;
    sec = time & 0x001f;
    sprintf(stime, "%02d:%02d:%02d", hour >> 11, min >> 5, sec << 1);
    return stime;
}


/** 内置命令列表，后面是它们对应的功能。 */
char *builtin_str[] = {
        "format",
        "cd",
        "mkdir",
        "rmdir",
        "ls",
        "create",
        "rm",
        "exit",
        "pwd"
};

int (*builtin_func[])(char **) = {
        &my_format,
        &my_cd,
        &my_mkdir,
        &my_rmdir,
        &my_ls,
        &my_create,
        &my_rm,
        &my_exit_sys,
        &my_pwd
};

int csh_num_builtins(void) {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
 * @brief 启动一个程序并等待它终止
 * @param args Null 终止的参数列表。
 * @return 始终返回 1，以继续执行。
 */
int csh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // 子进程
        if (execvp(args[0], args) == -1) {
            perror("csh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("csh");
    } else {
        // 父进程
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/*
 * @brief 执行 shell 内置或启动程序。
 * @param args Null 终止的参数列表。
 * @return 1 如果 shell 应该继续运行，0 如果它应该终止。
 */
int csh_execute(char **args) {
    int i;
    if (args[0] == NULL) {
        // 输入了一个空命令
        return 1;
    }

    for (i = 0; i < csh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return csh_launch(args);
}

/*
 @brief 从标准输入读取一行输入。
 @return 来自标准输入的行。
 */
char *csh_read_line(void) {
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

#define CSH_TOK_BUFSIZE 64
#define CSH_TOK_DELIM " \t\r\n\a"

char **csh_split_line(char *line) {
    int bufsize = CSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens) {
        fprintf(stderr, "csh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, CSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += CSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "csh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, CSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/*
 * @brief 循环获取输入并执行它。
 */
void csh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("\n\e[1mloveliness\e[0m@loveliness-PC \e[1m%s\e[0m\n", current_dir);
        printf("> \e[032m$\e[0m ");
        line = csh_read_line();
        args = csh_split_line(line);
        status = csh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {
    start_sys();
    csh_loop();

    return EXIT_SUCCESS;
}
