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
     * @param bool unique:true创建单实例守护进程，false反之
     * @param const char *workDir:重定向到指定目录，默认为“/”
     * @param bool noclose：false重定向stdin,stdout,stderr到/dev/null
     * @return true为成功,false时会记录错误日志
     */
  static bool Init(bool unique = true, const char *workDir = "/", bool noclose = false);

private:
  const Daemonize &operator=(const Daemonize &);
  Daemonize(const Daemonize &);
  Daemonize();
};

#endif // DAEMONIZE_H
