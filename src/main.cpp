#include "Daemonize.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define RWRWRW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int main(int argc, char const *argv[])
{
    if (!Daemonize::Init(false, "/home/zero/www"))
    {
        //此处需打开日志文件记录
        exit(1);
    }
    char buf[4] = "abc";
    int fd = open("./test.log", O_RDWR | O_CREAT | O_APPEND, RWRWRW);
    write(fd, buf, 2);
    close(fd);
    return 0;
}