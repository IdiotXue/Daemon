# Daemon Procedure
- 初始化程序为守护进程,可选是否唯一实例
- main.cpp中增加信号捕获函数，并设置捕获SIGHUP和SIGTERM
- make之前请确保目录结构完整，运行build.sh能够自动检测目录结构
