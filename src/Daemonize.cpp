#include "Daemonize.h"
#include <signal.h>   //sigaction
#include <sys/stat.h> //umask
#include <fcntl.h>    //open
#include <string.h>   //strerror
#include <unistd.h>   //fork,getpid,getdtablesize,getpid,pid_t
#include <iostream>   //TODO:cout和cerr应改为记录入日志文件
using namespace std;

long Daemonize::m_lPid = 0; //守护进程id不可能是0，因此用来表示还未成为守护进程

bool Daemonize::Init(const char *lockFile, const char *workDir, bool unique, bool noclose)
{
    pid_t pid = fork(); //创建子进程，与父进程共享相同的文件表项
    if (pid < 0)        //创建失败
    {
        cerr << "first fork() in Daemonize::Init() failed" << endl;
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
        cerr << "setsid() in Daemonize::init() failed" << endl;
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
        cerr << "sigaction() in Daemonize::init() failed " << endl;
        return false;
    }

    /* 终止第一子进程(会话首进程),第二子进程(孤儿进程，同时也是守护进程)继续,
       确保即使守护进程打开一个控制终端，也不会自动获得控制终端，因为只有会话首进程能获得 */
    pid = fork();
    if (pid < 0)
    {
        cerr << "second fork() in Daemonize::Init() failed" << endl;
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
        cerr << "Change work directory failed" << endl;
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

    m_lPid = (long)getpid(); //获取守护进程的pid
    //判断是否初始化为单实例的守护进程
    if (unique)
    {
        if (IsRunning(lockFile))
        {
            cerr << "Daemon already running" << endl;
            exit(1);
        }
    }
    return true;
}

int Daemonize::LockFile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;            //唯一写锁
    fl.l_whence = SEEK_SET;         //偏移量设置为距文件开始处l_start字节
    fl.l_start = 0;                 //偏移字节量
    fl.l_len = 0;                   //锁范围为文件最大可能偏移量，即整个文件(无论追加多少数据)
    return fcntl(fd, F_SETLK, &fl); //无阻塞设置锁
}

bool Daemonize::IsRunning(const char *lockFile)
{
    //注意此处为读写打开文件，但并没有O_TRUNC:截断文件长度为0，不能用标准I/O的fopen替代
    int fd = open(lockFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        cerr << "can‘t open " << lockFile << " : " << strerror(errno) << endl;
        exit(1);
    }
    if (LockFile(fd) < 0)
    {
        //如果文件已被加写锁，再次加写锁会返回-1，errno设置为EACCES或EAGAIN
        if ((errno == EACCES) || (errno == EAGAIN))
        {
            close(fd);
            return true;
        }
        //无法加锁，且不是因为已被加锁
        cerr << "can't lock " << lockFile << " : " << strerror(errno) << endl;
        exit(1);
    }
    //文件长度截断为0，防止新的pid比文件中原有pid短，导致原数字没能全部覆盖
    ftruncate(fd, 0);
    char buf[32];
    sprintf(buf, "%ld", m_lPid);
    write(fd, buf, strlen(buf));
    //关闭文件描述符会使记录锁释放，所以程序结束之前都不应关闭fd
    return false;
}