​
[虚拟机镜像安装的版本](https://old-releases.ubuntu.com/releases/16.04.1/) 这个页面中的
```
ubuntu-16.04-desktop-i386.iso
```


刚学linux没几天，在学习实验楼中的os操作系统的时候，想着配置一下本地环境，出现了下面的报错:

linux0.11 make : gcc-3.4:Command not found


[githup上的教程](https://github.com/Wangzhike/HIT-Linux-0.11/blob/master/0-prepEnv/%E5%87%86%E5%A4%87%E5%AE%89%E8%A3%85%E7%8E%AF%E5%A2%83.md)

在上面的教程中走到:
```
./setup.sh
```

这一步之后，按照以下步骤一步一步执行
```
cd /tmp
sudo  cp -r gcc-3.4   /usr/bin
cd /usr/bin/gcc-3.4
./inst.h  i386
```


**如果安装的ubuntu镜像文件是amd64的将最后一步指令中的i386换成amd64的试试**

我是这样搞成了。
