```c++
//设置端口复用
int opt = 1;
setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
LOG(INFO, "create socket ... success");
```

这段代码使用了 `setsockopt` 函数，用于设置套接字选项。让我解释一下：

- `setsockopt` 是一个系统调用，用于设置与套接字相关的选项。
- `_listen_sock` 是一个套接字描述符（Socket Descriptor），它表示一个被监听的套接字。
- `SOL_SOCKET` 是套接字级别的选项，表示我们要设置的是套接字本身的选项。
- `SO_REUSEADDR` 是一个选项名称，表示允许在 `bind()` 调用中使用已在使用中的地址。
- `&opt` 是一个指向选项值的指针，通常这里是一个布尔值或整数，用来控制选项的行为。
- `sizeof(opt)` 是指定选项值的大小，通常是 `int` 类型的大小。

在这种情况下，`SO_REUSEADDR` 选项的使用允许一个端口在释放之后立即被再次使用。这在一些网络编程场景下很有用，特别是在服务器关闭后重新启动时，允许服务器快速重用之前的端口而不必等待一段时间（等待 TIME_WAIT 时间结束）。

但是，这个选项也需要小心使用，因为它可能导致端口复用时的一些问题，比如可能会收到之前连接遗留的数据包。所以在使用这个选项时需要根据具体情况来考虑潜在的影响和安全性。





在 C/C++ 中，`strerror` 函数用于返回指定错误码的描述字符串。其函数原型通常如下：
```c++
#include <string.h>
char *strerror(int errnum);
```


1.errnum 是要获取描述字符串的错误码。
2.返回类型是 char *，指向错误描述字符串的指针。

**这个函数通常用于将系统错误码转换为对应的人类可读的描述信息**。例如，如果你有一个系统调用返回的错误码，你可以使用 `strerror` 来获取该错误码的描述字符串，以便在日志或用户界面中显示有意义的错误消息。
请注意，`strerror` 函数不是线程安全的，因为它使用了静态分配的缓冲区来存储错误描述字符串，因此在多线程环境中使用时需要小心。有一些线程安全版本的这个函数，例如 `strerror_r`，在某些系统中可能会提供。





`exit` 是一个在C和C++中用于终止程序执行的函数。它的作用有以下几个方面：

1. **终止程序的执行：** `exit` 函数会导致程序立即终止执行。它会执行一系列清理操作，包括调用所有注册的终止处理程序（通过 `atexit` 注册的函数），然后返回到操作系统。

2. **返回状态码：** `exit` 函数可以带有一个整数参数，该参数被用作程序的退出状态码。这个状态码会传递给操作系统，可以在调用程序的脚本或其他方式中检查，以了解程序的执行结果。通常，0 表示成功，而其他非零值表示出现错误。

   ```c++
   c#include <stdlib.h>
   
   int main() {
       // 程序执行一些操作...
   
       // 正常退出，返回状态码 0
       exit(0);
   }
   ```

3. **调用终止处理程序：** `exit` 函数会调用通过 `atexit` 注册的终止处理程序。这些处理程序可以用于释放资源、关闭文件等清理操作。例如：

   ```c++
   #include <stdlib.h>
   
   void cleanup() {
       // 清理操作...
   }
   
   int main() {
       // 注册终止处理程序
       atexit(cleanup);
   
       // 程序执行一些操作...
   
       // 正常退出，会调用 cleanup 函数
       exit(0);
   }
   ```

总的来说，`exit` 函数是一个程序正常终止并返回状态码的方式，它也触发了一些清理操作，包括调用注册的终止处理程序。





`\r`（回车符）和`\n`（换行符）是两个在计算机文本处理中常见的控制字符，它们在不同的操作系统和文本编辑环境中有不同的使用方式。

1. **回车符 `\r`：**
   - ASCII码为13。
   - 表示将光标移到当前行的开头，而不换行。
   - 在一些早期的系统和标准中，回车符通常与换行符配对使用，表示新的一行。在Windows中，文本文件的行结束符通常是`\r\n`。
2. **换行符 `\n`：**
   - ASCII码为10。
   - 表示在文本中开始新的一行，光标移到下一行的开头。
   - 在Unix/Linux系统中，通常只使用`\n`作为行结束符。在标准C和C++中，`'\n'` 表示新行，并且在多数现代操作系统中都得到了广泛支持。

在不同的操作系统中，对这两个字符的处理方式可能不同：

- **Windows：** 行结束通常是由 `\r\n` 组成的。
- **Unix/Linux：** 行结束通常是由 `\n` 单独表示的。
- **macOS：** 行结束通常是由 `\r` 单独表示的。

在文本处理和编程中，理解不同系统对行结束符的处理方式很重要，因为在跨平台开发和文本文件处理时可能会涉及到这些差异。例如，在读取文本文件时，程序可能需要根据系统的不同来正确解释行结束符。





## `linux`系统调用函数`stat()` 获取文件的属性



`stat()` 是一个用于获取文件信息的系统调用函数，用于检索与文件相关的元数据，如==文件大小==、==访问权限==、==创建时间==等。在Linux系统中，`stat()` 函数的原型定义在 `<sys/stat.h>` 头文件中，其基本形式如下：

```c++
int stat(const char *path, struct stat *buf);
```

其中：

- `path` 参数是指向文件或目录路径的字符串。
- `buf` 参数是指向 `struct stat` 结构的指针，该结构包含了文件的详细信息。

`struct stat` 的定义如下：

```c++
struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* Inode number */
    mode_t    st_mode;        /* File type and mode */
    nlink_t   st_nlink;       /* Number of hard links */
    uid_t     st_uid;         /* User ID of owner */
    gid_t     st_gid;         /* Group ID of owner */
    dev_t     st_rdev;        /* Device ID (if special file) */
    off_t     st_size;        /* Total size, in bytes */
    blksize_t st_blksize;     /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */

    /* Since Linux 2.6, the following fields are also available */
    struct timespec st_atim;  /* Time of last access */
    struct timespec st_mtim;  /* Time of last modification */
    struct timespec st_ctim;  /* Time of last status change */

    #define st_atime st_atim.tv_sec      /* Backward compatibility */
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec
};
```

`stat()` 函数返回值为 0 表示成功，-1 表示出现错误。错误信息被存储在全局变量 `errno` 中。

以下是一个简单的例子，演示如何使用 `stat()` 函数获取文件信息：

```c++
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

int main() {
    const char *filepath = "/path/to/your/file.txt";
    struct stat fileStat;

    if (stat(filepath, &fileStat) == 0) {
        std::cout << "File Size: " << fileStat.st_size << " bytes" << std::endl;
        std::cout << "Number of Blocks: " << fileStat.st_blocks << std::endl;
        // 可以获取更多文件信息，比如权限、用户ID、组ID等
    } else {
        perror("Error in stat");
    }
    return 0;
}
```

请确保你有足够的权限来访问文件，否则 `stat()` 调用可能会失败。

## errno (实际上就是一个`int` 变量，不同的值，标志着不同的错误状态)

`errno` 是一个全局变量，用于报告函数调用期间发生的错误。在C和C++中，当某个函数出现错误时，它通常会设置 `errno` 变量来指示具体的错误类型。头文件 `<errno.h>` 中包含了 `errno` 的定义，同时定义了一系列可能的错误码。

以下是 `errno` 的基本用法和一些常见的错误码：

```c++
#include <errno.h>
#include <perror.h>
#include <stdio.h>
int main() {
    FILE *file = fopen("nonexistent_file.txt", "r");
    if (file == NULL) {
        // 出现错误，查看 errno 变量
        perror("Error opening file");
        printf("Error code: %d\n", errno);

        // 可以根据错误码执行相应的处理逻辑
        if (errno == ENOENT) {
            printf("File not found.\n");
        } else if (errno == EACCES) {
            printf("Permission denied.\n");
        }

        // 重置 errno，以便后续的错误检查
        errno = 0;
    } else {
        // 文件打开成功，进行相应的操作
        fclose(file);
    }
    return 0;
}
```

在上述例子中，`perror("Error opening file");` 会打印一个描述错误的消息，其中包含 `errno` 对应的错误字符串。然后，通过 `printf("Error code: %d\n", errno);` 打印具体的错误码。在这个例子中，我们检查了两个可能的错误码：`ENOENT` 表示文件不存在，`EACCES` 表示权限被拒绝。

在不同的系统上，`errno` 可能具有不同的实现细节，但通常是一个整数。在多线程程序中，每个线程都有自己的 `errno` 值，而不同线程之间的 `errno` 不会相互影响。函数调用成功时，通常会将 `errno` 设置为零。在使用 `errno` 之前，最好先检查函数是否成功，以避免查看未定义或无效的 `errno` 值。





##宏定义`S_ISDIR`

`S_ISDIR` 是一个宏，==用于检查文件类型是否为目录==。它通常与 `POSIX` 标准一起使用，并在 `<sys/stat.h>` 头文件中定义。

它**的原理在于使用文件的权限信息（通过 `st_mode` 成员）以及文件类型位屏蔽来检查文件的类型**。`struct stat` 结构体中的 `st_mode` 字段包含了有关文件类型和访问权限的信息。`S_ISDIR` 宏检查 `st_mode` 是否表示目录。

`S_ISDIR` 的定义类似于下面的形式：

```c++
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
```

在这里：

- `S_ISDIR` 是一个宏，用于检查文件类型是否为目录。
- `mode` 是 `struct stat` 结构中的 `st_mode` 成员，包含了文件的类型和权限信息。
- `S_IFMT` 是一个位掩码，==用于提取文件类型部分的位==。
- `S_IFDIR` 是一个表示目录的文件类型位掩码。

`S_ISDIR` 宏的工作原理是通过对 `st_mode` 使用位掩码操作，将文件类型信息和 `S_IFDIR` 进行比较。如果 `st_mode` 中表示的文件类型与 `S_IFDIR` 匹配，它将返回非零值，表示文件是一个目录；否则返回零，表示文件不是目录。

这种宏的设计是为了使得文件类型的检查更为方便和可读性更高，避免了直接对 `st_mode` 进行位操作，提高了代码的可维护性。例如，你可以通过 `S_ISDIR` 来检查一个文件是否为目录，而不必直接解析 `st_mode` 字段的具体位信息。



### 宏 `S_IXUSR` `S_IXGRP` `S_IXOTH`

在Linux中，`S_IXUSR`、`S_IXGRP`、`S_IXOTH` 是用于操作文件权限的宏，它们分别表示文件的用户（owner）、用户组（group）和其他用户的执行权限位。

1. **`S_IXUSR`**：
   - **含义：** 文件所有者（用户）的执行权限位。
   - **十进制值：** 0100。
   - **八进制表示：** 100。
   - **用法示例：** `chmod u+x filename` 或者 `fileStat.st_mode |= S_IXUSR;`
2. **`S_IXGRP`**：
   - **含义：** 文件用户组的执行权限位。
   - **十进制值：** 0010。
   - **八进制表示：** 010。
   - **用法示例：** `chmod g+x filename` 或者 `fileStat.st_mode |= S_IXGRP;`
3. **`S_IXOTH`**：
   - **含义：** 其他用户的执行权限位。
   - **十进制值：** 0001。
   - **八进制表示：** 001。
   - **用法示例：** `chmod o+x filename` 或者 `fileStat.st_mode |= S_IXOTH;`

这些宏用于在文件权限掩码中设置或检查相应的执行权限位。通过与位运算结合，你可以创建一个完整的权限掩码，然后使用 `chmod` 函数将其应用到文件上。例如，如果你想将一个文件的所有者、用户组和其他用户的执行权限都设置为可执行，你可以这样做：

```c++
c#include <sys/stat.h>

int main() {
    const char *filename = "example_file";
    struct stat fileStat;

    // 获取文件信息
    if (stat(filename, &fileStat) == 0) {
        // 设置所有者、用户组和其他用户的执行权限
        fileStat.st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;

        // 在这里进行其他操作...

        // 最后，你可能需要使用 chmod 更新文件的权限
        chmod(filename, fileStat.st_mode);
    }

    return 0;
}
```

上述例子中，`S_IXUSR | S_IXGRP | S_IXOTH` 将所有者、用户组和其他用户的执行权限位都设置为可执行。





