#ifndef DAEMONIZE_H
#define DAEMONIZE_H

/**
 * 用于创建守护进程
 */
class Daemonize
{
public:
  /**
     * 将普通进程转换为守护进程,脱离控制终端
     * @param const char *lockFile:确保单实例守护进程时设置记录锁的文件,
     *    默认为“./daemon.pid”,创建于workDir指定的目录;unique为false时失效可设为NULL，
     * @param const char *workDir:重定向到指定目录，默认为“/”
     * @param bool unique:true创建单实例守护进程，false反之
     * @param bool noclose：false重定向stdin,stdout,stderr到/dev/null
     * @return true为成功,false时会记录错误日志
     */
  static bool Init(const char *lockFile = "daemon.pid", const char *workDir = "/", bool unique = true, bool noclose = false);
  /**
   * 返回守护进程的pid
   * return long:如果还未调用Init成为守护进程则返回0，否则返回守护pid
   */
  inline static long GetPid() { return m_lPid; }

private:
  const Daemonize &operator=(const Daemonize &);
  Daemonize(const Daemonize &);
  Daemonize(); //所有函数为static，不允许定义实例
  /**
   * 在fd指向的文件整体上加一把写锁
   * @param int fd：文件描述符
   * @return fcntl函数的返回值
   */
  static int LockFile(int fd);
  /**
   * 确保只运行一个守护进程的一个副本(即单实例守护进程);注意：关闭文件描述符会使锁释放
   * @param const char *lockFile:确保单实例守护进程时设置记录锁的文件
   * @return true为已有副本在运行，false表示当前为唯一副本
   */
  static bool IsRunning(const char *lockFile);

  static long m_lPid; //缓存daemon pid，不用每次都调用getpid()陷入内核
};

#endif // DAEMONIZE_H
