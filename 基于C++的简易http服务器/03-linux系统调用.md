### 系统调用函数`pipe()`

在Linux系统编程中，`pipe` 函数用于创建一个无名管道（pipe）。无名管道是一种进程间通信的机制，==它允许一个进程将数据写入管道，而另一个进程则可以从管道中读取这些数据==。通常，无名管道用于在父进程和子进程之间进行通信。

`pipe` 函数的原型如下：

```c++
#include <unistd.h>

int pipe(int pipefd[2]);
```

- **参数：** `pipefd` 是一个包含==两个文件描述符==的数组，==`pipefd[0]` 用于从管道读取数据(对于进程是输入)，`pipefd[1]` 用于写入数据到管道(对于进程是输出)==。
- **返回值：** 若成功，返回0；若失败，返回-1，并设置 `errno` 来指示错误的类型。

下面是一个简单的例子，演示了如何使用 `pipe` 函数创建一个无名管道，并通过管道在父子进程之间传递数据：

```c++
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[30];

    // 创建管道
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // 子进程
        close(pipefd[1]); // 关闭写入端，子进程只读

        // 从管道中读取数据
        read(pipefd[0], buffer, sizeof(buffer));
        printf("Child process received: %s\n", buffer);

        close(pipefd[0]); // 关闭读取端
    } else { // 父进程
        close(pipefd[0]); // 关闭读取端，父进程只写

        // 向管道中写入数据
        write(pipefd[1], "Hello from parent", 17);

        close(pipefd[1]); // 关闭写入端
    }

    return 0;
}
```

在这个例子中，父进程和子进程通过管道进行通信。父进程向管道写入字符串，而子进程从管道中读取这个字符串。需要注意的是，==写入端和读取端的关闭是为了确保进程间通信的正确性。==



## 系统调用函数`fork()` 函数

`fork()` 是一个在Unix/Linux系统中非常重要的系统调用函数之一，它用于创建一个==新的进程==，这个新进程称为子进程，而调用 `fork()` 的进程称为父进程。新的子进程是父进程的复制，==它从父进程继承了大部分的状态，包括代码段、数据段、堆、栈以及文件描述符等。==

`fork()` 的原型如下：

```c++
#include <unistd.h>

pid_t fork(void);
```

- **返回值：** ==在父进程中，`fork()` 返回新创建的子进程的进程ID；在子进程中，返回0；如果发生错误，父进程返回-1，子进程不会被创建==

下面是一个简单的例子，演示了 `fork()` 的基本用法：

```c++
#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid;

    // 调用 fork() 创建子进程
    pid = fork();

    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // 子进程
        printf("This is the child process. PID: %d\n", getpid());
    } else {
        // 父进程
        printf("This is the parent process. Child PID: %d\n", pid);
    }
    // 这里的代码会被父进程和子进程都执行
    
   return 0;
}
```

在这个例子中，`fork()` 被调用后，它返回两次：一次在父进程中，返回子进程的PID；一次在子进程中，返回0。因此，==通过检查返回值可以确定当前代码是在父进程还是子进程中执行。==

需要注意的是，==`fork()` 之后，父进程和子进程是相对独立的，它们各自有独立的内存空间，互不影响==在实际使用中，`fork()` 的返回值常用于区分父子进程，从而执行不同的代码逻辑。





### `putenv()` 函数

在Linux中，`putenv()` 函数用于设置环境变量。==环境变量是一种在操作系统中存储配置信息的机制，它可以由进程读取，用于控制程序的行为。==`putenv()` 函数允许你通过指定字符串的方式设置环境变量。该函数的原型如下：

```c++
#include <stdlib.h>

int putenv(char *string);
```

- **参数：** `string` 是一个以 "name=value" 格式表示的字符串，其中 `name` 是环境变量的名称，`value` 是它的值。
- **返回值：** 如果成功，返回0；如果失败，返回非零值。

下面是一个简单的例子，演示了如何使用 `putenv()` 设置和获取环境变量：

```c++
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 设置环境变量
    if (putenv("MY_VARIABLE=HelloWorld") == 0) {
        printf("Environment variable set successfully.\n");
    } else {
        perror("putenv");
        return -1;
    }

    // 获取环境变量
    char *value = getenv("MY_VARIABLE");

    if (value != NULL) {
        printf("Value of MY_VARIABLE: %s\n", value);
    } else {
        printf("MY_VARIABLE is not set.\n");
    }

    return 0;
}
```

在这个例子中，`putenv()` 被用来设置一个名为 "MY_VARIABLE" 的环境变量，并通过 `getenv()` 获取它的值。需要注意的是，`putenv()` 设置的环境变量的内存由它自己管理，因此在调用 `putenv()` 之后，不要修改传递给它的字符串。如果需要修改环境变量，建议使用 `setenv()` 函数。



## `getenv()` 函数

在Linux中，`getenv()` 函数**用于获取指定环境变量的值。环境变量是一种在操作系统中存储配置信息的机制，允许进程读取这些变量以获取有关系统环境的信息**。`getenv()` 函数接受一个环境变量的名称作为参数，并返回该变量的值。

该函数的原型如下：

```c++
#include <stdlib.h>

char *getenv(const char *name);
```

- **参数：** `name` 是要获取的环境变量的名称。
- **返回值：** 如果指定名称的环境变量存在，则返回该环境变量的值（以字符串形式）；如果指定名称的环境变量不存在，则返回 `NULL`。

下面是一个简单的例子，演示了如何使用 `getenv()` 获取环境变量的值：

```c++
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 获取环境变量的值
    char *value = getenv("HOME");

    if (value != NULL) {
        printf("Value of HOME: %s\n", value);
    } else {
        printf("HOME is not set.\n");
    }

    return 0;
}
```

在这个例子中，`getenv()` 被用来获取名为 "HOME" 的环境变量的值。如果该环境变量存在，则打印其值；否则，打印一条消息说明该环境变量未设置。

需要注意的是，`getenv()` 返回的**字符串指针指向一个内部的静态缓冲区，因此不应该尝试修改返回的字符串**。如果需要修改环境变量的值，应该使用 `setenv()` 函数。





## 系统调用dup2

在Linux系统编程中，==`dup2` 函数用于复制文件描述符，并使新的文件描述符引用已存在的文件描述符==。这允许一个文件描述符（如标准输入、标准输出）被重新定向到另一个文件： 

`dup2` 函数的原型如下：

```c++
#include <unistd.h>

int dup2(int oldfd, int newfd);
```

- **参数：**
  - `oldfd` 是待复制的文件描述符。
  - `newfd` 是新的文件描述符。如果 `newfd` 已经打开，它会被关闭；如果 `newfd` 等于 `oldfd`，则不进行复制，直接返回 `newfd`。
- **返回值：** 如果成功，返回新的文件描述符 `newfd`；如果失败，返回-1，并设置 `errno` 来指示错误的类型。

下面是一个简单的例子，演示了如何使用 `dup2` 函数将标准输出（文件描述符1）重定向到一个文件：

```c++
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    // 打开或创建一个文件（例如output.txt）
    int file_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file_fd == -1) {
        perror("open");
        return -1;
    }

    // 复制文件描述符，将标准输出重定向到文件
    if (dup2(file_fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(file_fd);
        return -1;
    }

    // 关闭不再需要的文件描述符
    close(file_fd);

    // 现在标准输出已经被重定向到文件，下面的内容会写入到文件而不是终端
    printf("This will be written to the file.\n");

    return 0;
}
```

在这个例子中，`dup2` 被用来将文件描述符 `file_fd` 复制到标准输出（文件描述符1），这样所有写入标准输出(实际上就是写入到索引为2的槽中的对应的文件)的内容都会被重定向到打开的文件中。





## `execl()` 函数

`execl` 是 Linux 中的一个系统调用函数，用于执行一个新的程序。

其原型如下：

```c++
#include <unistd.h>

int execl(const char *path, const char *arg0, const char *arg1, ..., const char *argn, (char *) NULL);
```

- `path` 参数是要执行的可执行文件的路径。
- `arg0`、`arg1`、...、`argn` 是要传递给新程序的参数列表。最后一个参数必须是 `nullptr`，表示参数列表的结束。(`arg0`一般传递的是可执行文件的名称或路径)
- `execl` 函数会加载并执行指定路径的新程序，并用该程序替换当前进程的镜像。这意味着==原始程序的代码、数据和堆栈==（不包括文件描述符表）都会被新程序所取代。
- 如果 `execl` 函数执行成功，它不会返回。如果返回了，通常是因为出现了错误，可以通过检查返回值来确定错误原因。

示例：

```c++
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Executing ls command...\n");

    // 调用 execl 执行 ls 命令
    execl("/bin/ls", "ls", "-l", nullptr ) ; 

    // 如果 execl 执行成功，此处的代码不会被执行
    perror("execl"); // 如果 execl 返回，可能是因为出错了
    return 1; // 返回错误码
}
```

在这个例子中，`execl` 被用来执行 `ls -l` 命令。如果 `execl` 执行成功，当前进程的镜像会被 `ls` 命令替换，并输出当前目录的文件列表和详细信息。如果 `execl` 函数执行失败，将会输出相应的错误信息。

==需要注意的是，`execl` 函数并不创建新的进程，而是在当前进程中加载并执行新程序。==





## `read()` 函数

`read` 是 Linux 中的一个系统调用函数，用于从文件描述符（通常是文件、管道、套接字等）读取数据。

其原型如下：

```c++
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
```

- `fd` 是文件描述符，指向要读取的文件、管道、套接字等。
- `buf` 是一个指向存储读取数据的缓冲区的指针。
- `count` 是要读取的字节数。

==`read` 返回值是实际读取的字节数。如果返回 -1，表示发生错误。如果返回 0，表示已经到达文件尾或者在读取套接字时对方已经关闭连接。==

示例：

```c++
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd;
    ssize_t bytesRead;
    char buffer[100];

    // 打开文件，获取文件描述符
    fd = open("example.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 使用 read 从文件中读取数据
    bytesRead = read(fd, buffer, sizeof(buffer));

    if (bytesRead == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    // 输出读取的数据
    printf("Read %zd bytes: %s\n", bytesRead, buffer);

    // 关闭文件描述符
    close(fd);

    return 0;
}
```

在这个例子中，程序打开一个名为 "example.txt" 的文件，然后使用 `read` 从文件中读取数据到缓冲区 `buffer` 中。最后，程序输出读取的数据，并关闭文件描述符。需要注意的是，这只是一个简单的示例，实际应用中需要更多的错误处理和边界检查。







##`waitpid` 函数

大家知道，当用fork启动一个新的子进程的时候，子进程就有了新的生命周期，并将在其自己的地址空间内独立运行。但有的时候，我们希望知道某一个自己创建的子进程何时结束，从而方便父进程做一些处理动作。同样的，在用ptrace去attach一个进程滞后，那个被attach的进程某种意义上说可以算作那个attach它进程的子进程，这种情况下，有时候就想知道被调试的进程何时停止运行。

以上两种情况下，都可以使用Linux中的waitpid()函数做到。先来看看waitpid函数的定义：

```c++
#include <sys/types.h> 
#include <sys/wait.h>
pid_t waitpid(pid_t pid,int *status,int options);
```



如果在调用waitpid()函数时，当指定等待的子进程已经停止运行或结束了，则waitpid()会立即返回；但是如果子进程还没有停止运行或结束，则调用waitpid()函数的父进程则会被阻塞，暂停运行。
下面来解释以下调用参数的含义：

1）pid_t pid

参数pid为欲等待的子进程识别码，其具体含义如下：
![image-20231117102641071](assets/image-20231117102641071.png)

2）**int *status**

**这个参数将保存子进程的状态信息，有了这个信息父进程就可以了解子进程为什么会退出，是正常推出还是出了什么错误**。如果`status`不是空指针，则状态信息将被写入
器指向的位置。当然，如果不关心子进程为什么推出的话，也可以传入空指针。
Linux提供了一些非常有用的宏来帮助解析这个状态信息，这些宏都定义在`sys/wait.h`头文件中。主要有以下几个：
![image-20231117102703825](assets/image-20231117102703825.png)

**3）int options**

参数options提供了一些另外的选项来控制waitpid()函数的行为。如果不想使用这些选项，则可以把这个参数设为0。

主要使用的有以下两个选项：

![image-20231117102726561](assets/image-20231117102726561.png)

这些参数可以用“|”运算符连接起来使用。
==如果waitpid()函数执行成功，则返回子进程的进程号；如果有错误发生，则返回-1，并且将失败的原因存放在errno变量中。==
失败的原因主要有：没有子进程（errno设置为ECHILD），调用被某个信号中断（errno设置为EINTR）或选项参数无效（errno设置为EINVAL）
如果像这样调用waitpid函数：waitpid(-1, status, 0)，这此时waitpid()函数就完全退化成了wait()函数。

## 系统调用sendfile

`sendfile` 是 `Linux` 中的系统调用函数，==用于在两个文件描述符之间直接传输数据，而无需在用户空间缓冲区中进行中间复制==。主要用于优化文件传输性能，特别是在网络编程中。

其原型如下：

```c++
#include <sys/sendfile.h>

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
```

- `out_fd` 是目标文件描述符，通常是一个套接字
- `in_fd` 是源文件描述符，通常是一个普通文件
- `offset` 是指向 `in_fd` 文件中的偏移量的指针，用于指定从源文件的哪个位置开始读取数据。如果设置为 `NULL`，则从当前文件偏移量开始。
- `count` 是要传输的字节数。

`sendfile` 函数返回实际传输的字节数。如果返回 -1，则表示出现错误。

示例：

```c++
#include <stdio.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    int source_fd, dest_fd;
    struct stat stat_source;

    // 打开源文件
    source_fd = open("source.txt", O_RDONLY);
    if (source_fd == -1) {
        perror("open source");
        return 1;
    }

    // 获取源文件的状态信息
    if (fstat(source_fd, &stat_source) == -1) {
        perror("fstat");
        close(source_fd);
        return 1;
    }

    // 打开目标文件
    dest_fd = open("destination.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("open destination");
        close(source_fd);
        return 1;
    }

    // 使用 sendfile 将数据从源文件传输到目标文件
    off_t offset = 0;
    ssize_t sent_bytes = sendfile(dest_fd, source_fd, &offset, stat_source.st_size);

    if (sent_bytes == -1) {
        perror("sendfile");
        close(source_fd);
        close(dest_fd);
        return 1;
    }

    printf("Sent %zd bytes from source to destination.\n", sent_bytes);

    // 关闭文件描述符
    close(source_fd);
    close(dest_fd);

    return 0;
}
```

在这个例子中，程序使用 `sendfile` 将数据从一个文件（source.txt）传输到另一个文件（destination.txt），而不需要在用户空间中创建中间缓冲区。这可以提高文件传输的效率。





