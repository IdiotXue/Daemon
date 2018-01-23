#include "Daemonize.h"
#include <unistd.h> //close
#include <stdlib.h> //exit
#include <fcntl.h>  //open
#include <string.h> //strlen
#include <signal.h>
#include <stdio.h> //fputs

#define RWRWRW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
typedef void SigHandle(int);
int fd;
FILE *fp;

void writeLog(const char *buf)
{
    fputs(buf, fp); //带缓冲I/O
    fflush(fp);    
    // write(fd, buf, strlen(buf)); //无缓存I/O
}

void sighup(int signo) //SIGHUP信号处理函数
{
    writeLog("sighup\n");
}

void sigterm(int signo) //SIGTERM信号处理函数
{
    writeLog("sigterm,exit\n");
    exit(0);
}
/**
 * 捕获指定信号，并做指定处理操作
 * @param int signo:指定信号编号
 * @param SigHandle *func:函数指针，指向信号处理函数void (*func)(int)
 * @return SigHandle *func:函数指针，指向之前的信号处理函数
 */
SigHandle *Signal(int signo, SigHandle *func)
{
    struct sigaction act, oact;
    act.sa_handler = func; //信号处理程序的地址，也可以是SIG_IGN或SIG_DFL
    //信号集使用前必须做empty或full的初始化，调用sa_handler之前，sa_mask会加入进程信号屏蔽字
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    //为了对I/O操作可以限时，所以不希望重启由SIGALRM中断的系统调用
    if (signo == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else //其他信号则重启系统调用
        act.sa_flags |= SA_RESTART;
    if (sigaction(signo, &act, &oact) < 0)
        return SIG_ERR;
    return oact.sa_handler;
}

int main(int argc, char const *argv[])
{
    if (!Daemonize::Init("prod.pid", "/home/zero/www"))
    {
        writeLog("Create daemon failed\n"); //此处需打开日志文件记录
        exit(1);
    }
    // fd = open("test.log", O_RDWR | O_CREAT | O_APPEND, RWRWRW);
    fp = fopen("test.log", "a+");
    writeLog("create daemon success\n");
    if (Signal(SIGTERM, sigterm) == SIG_ERR)
        writeLog("Signal() SIGTERM fail\n");
    if (Signal(SIGHUP, sighup) == SIG_ERR)
        writeLog("Signal() SIGTERM fail\n");
    while (1)
        ;
    // {
    //     writeLog("abcd");
    //     sleep(1);
    // }
    // close(fd);
    fclose(fp);
    return 0;
}