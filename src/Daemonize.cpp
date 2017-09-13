#include "Daemonize.h"
#include <unistd.h> //fork,getpid,getdtablesize
#include <signal.h> //sigaction
#include <sys/stat.h> //umask
#include <fcntl.h> //open

#include <iostream>
using namespace std;

bool Daemonize::Init(bool unique, const char *workDir, bool noclose)
{
    pid_t pid = fork(); //创建子进程，与父进程共享相同的文件表项
    if (pid < 0)        //创建失败
    {
        cout << "first fork() in Daemonize::Init() failed" << endl;
        return false;
    }
    else if (pid > 0) //父进程退出，使子进程在后台运行
    {
        cout << "Parent PID: " << getpid() << " First Child PID: " << pid << endl;
        _exit(0); //系统_exit调用,直接终止程序不做关闭文件描述符等处理
    }

    /* 第一子进程继续,以上fork()已确保该子进程不是进程组的组长,这是setsid()的必要条件
       且子进程现在运行于后台进程组，相当于在终端运行shell & */
    if (setsid() < 0) //该子进程 1.成为会话组长(首进程),2.成为新进程组的组长进程,3.脱离控制终端
    {
        cout << "setsid() in Daemonize::init() failed" << endl;
        return false;
    }

    /* 会话首进程(第一子进程)终止时会发送SIGHUP信号给进程组(默认操作终止进程),忽略它 */
    struct sigaction act;
    act.sa_handler = SIG_IGN; //忽略信号
    //信号集使用前必须做empty或full的初始化，调用sa_handler之前，sa_mask会加入进程信号屏蔽字
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGHUP, &act, NULL) < 0)
    {
        cout << "sigaction() in Daemonize::init() failed " << endl;
        return false;
    }

    /* 终止第一子进程(会话首进程),第二子进程(孤儿进程，同时也是守护进程)继续,
       确保即使守护进程打开一个控制终端，也不会自动获得控制终端，因为只有会话首进程能获得 */
    pid = fork();
    if (pid < 0)
    {
        cout << "second fork() in Daemonize::Init() failed" << endl;
        return false;
    }
    else if (pid > 0)
    {
        cout << "Daemon(Second Child) PID: " << pid << endl;
        _exit(0);
    }

    //重设文件模式创建屏蔽字,0表示清空，即允许创建任意rwx权限的文件，没有出错返回
    umask(0);
    // 改变工作目录
    if (chdir(workDir) < 0)
    {
        cout << "Change work directory failed" << endl;
        return false;
    }
    /* 关闭daemon从父进程继承来的所有打开的文件描述符,在此之前的标准输出都会打印到控制终端
       getdtablesize()返回当前进程最大可能存在文件描述符数量*/
    for (int fd = 0; fd < getdtablesize(); fd++)
    {
        cout << "can here " << fd << endl;
        close(fd);
    }

    if (!noclose)
    {
        /* 重定向标准stdin,stdout,stderr到/dev/null
           避免某些库函数假设了从它们中读写数据而它们却没打开
           或者它们作为套接字打开而非预期的发送和接收数据 */
        open("/dev/null", O_RDONLY);
        open("/dev/null", O_RDWR);
        open("/dev/null", O_RDWR);
    }
    //到此处时日志文件也需重新打开
    return true;
}
