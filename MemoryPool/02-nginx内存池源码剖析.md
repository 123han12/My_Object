`Nignx`内存池
--------

[nginx](https://so.csdn.net/so/search?q=nginx&spm=1001.2101.3001.7020)内存池是一个设计很巧妙，效率也特别高的内存池，较`SGI STL`的内存池也有许多的不同，本篇文章将会简单剖析`nginx`内存池的源码，通过分析`nginx`内存池的源码，我们也可以明显的看到该内存池与`SGI STL`内存池的一些区别。

源码剖析
----

### 1\. 重要的类型定义

#### 1.1 宏定义

首先我们需要先认识一些宏定义，下面是一些主要的宏定义：

```c++
// 可以从内存池中申请的最大的内存大小（一页，4k）
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)   // ngx_pagesize = 4096 

// 内存池的默认大小（16k）
#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

// 内存池字节对齐
#define NGX_POOL_ALIGNMENT       16

// 内存池的最小大小
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)
```


对于前面三个定义都不难理解，最后一个宏定义调用了**`ngx_align`**函数，这个函数的作用和`SGI STL`二级空间配置器的`_S_round_up`函数如出一辙，即将d的大小调整到最邻近的a的倍数，其完整定义如下：

```c++
// 把d调整为a的倍数
#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
```


#### 1.2 类型定义

接着我们需要了解内存池的基本结构，首先先看一下其中最基础得结构

```c++
// 小块内存数据头信息
typedef struct {
	u_char 			*last; 		// 可分配内存开始位置
	u_char 			*end;		// 可分配内存末尾位置
	ngx_pool_t 		*next; 		// 保存下一个内存池的地址
	ngx_uint_t 		failed; 	// 记录当前内存池分配失败的次数
} ngx_pool_data_t;

// nginx内存池的主结构体类型,一个内存池最多只有一个 ngx_pool_s 
struct ngx_pool_s {
	ngx_pool_data_t 	d;			// 内存池的数据头
	size_t				max; 		// 小块内存分配的最大值
	ngx_pool_t 			*current; 	// 小块内存池入口指针
	ngx_chain_t			*chain;		// 连接内存池，这个不重要
	ngx_pool_large_t 	*large; 	// 大块内存分配入口指针
	ngx_pool_cleanup_t 	*cleanup;   // 清理函数handler的入口指针
	ngx_log_t 			*log;		// 日志 ， 这个不重要
};
```

内存池中最主要的结构是`ngx_pool_s`，其中的`ngx_pool_data_t`也为结构体，用来存储小块内存数据的头信息，各个成员的含义已经在代码中给出，通过下图，我们可以简单的认识一下上述的结构的关系 
![](https://img-blog.csdnimg.cn/2020041720480987.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
接着我们来看内存池中的其他结构，下面给出了内存池中大块内存的类型定义`ngx_pool_large_s`以及外部清理操作的类型定义`ngx_pool_cleanup_s` ，关于下面两个结构体的功能将会在下文介绍。

```c++
// 大块内存类型定义
struct ngx_pool_large_s {
	ngx_pool_large_t 	*next; 		// 下一个大块内存
	void 				*alloc; 	// 记录分配的大块内存的起始地址
};
```


​    
```c++
typedef void (*ngx_pool_cleanup_pt)(void *data); 	// 清理回调函数的类型定义

// 清理操作的类型定义，包括一个清理回调函数，传给回调函数的数据和下一个清理操作的地址
struct ngx_pool_cleanup_s {
	ngx_pool_cleanup_pt 	handler; 	// 清理回调函数
	void 					*data;		// 传递给回调函数的指针
	ngx_pool_cleanup_t 		*next; 		// 指向下一个清理操作
};
```


因为C语言的语法，每次在使用结构体类型的时候，前面的`struct` 是不可以省略的，所以为了提高代码的可读性，这里对上述的结构体的名字进行了重命名。

```c++
typedef struct ngx_pool_s ngx_pool_t;
typedef struct ngx_pool_large_s ngx_pool_large_t;
typedef struct ngx_pool_cleanup_s ngx_pool_cleanup_t;
```

### 2\. 主要的函数接口

`nginx`内存池主要包括下列8个函数，下面将会对这8个函数进行一一解读

```c++
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log); // 创建内存池

void ngx_destroy_pool(ngx_pool_t *pool); 				  // 销毁内存池

void ngx_reset_pool(ngx_pool_t *pool); 					  // 重置内存池

void *ngx_palloc(ngx_pool_t *pool, size_t size); 		  // 内存分配函数，支持内存对齐
void *ngx_pnalloc(ngx_pool_t *pool, size_t size); 		  // 内存分配函数，不支持内存对齐
void *ngx_pcalloc(ngx_pool_t *pool, size_t size); 		  // 内存分配函数，支持内存初始化0

ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p) ; 			  // 内存释放（大块内存）

ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);  // 添加清理handler
```


#### 2.1 内存池的创建

##### 2.1.1` ngx_memalign`函数

在看内存池创建函数之前，我们先要了解`ngx_memalign`函数，其定义如下：

```C++
#if (NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)
void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log);  // #1
#else
#define ngx_memalign(alignment, size, log)  ngx_alloc(size, log)    // #2
#endif
```


当包含`NGX_HAVE_POSIX_MEMALIGN`或者`NGX_HAVE_MEMALIGN`时，调用 **#1**函数【这个版本的`#2`不同的是传递给`malloc`的大小是比`size`大的最近的`alignment`的倍数】

当没有上述两个宏，则调用 **#2**函数，即**`ngx_alloc(size, log)`**，这个函数就是对`malloc`的一个封装

```c++
void *ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "malloc(%uz) failed", size);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}
```

##### 2.1.2 `ngx_create_pool`函数

```c++
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log) // 第二个是日志参数，不重要
{
    ngx_pool_t  *p;
	// 申请size大小的空间
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log) ; // 实际上调用的是malloc, 根据宏定义的情况，选择性忽略NGX_POOL_ALIGNMENT , 开辟_size大小或者 _S_round_up(size) 大小的内存。
    if (p == NULL) {
        return NULL;
    }
    
	// 对内存池数据块结构进行初始化
    p->d.last = (u_char *) p + sizeof(ngx_pool_t) ; // 指向第一块可用内存
    p->d.end = (u_char *) p + size ; // 指向尾内存，可能开辟的是_S_round_up(size) 大小的内存
    p->d.next = NULL ; // 不存在下一个小块内存
    p->d.failed = 0;  

    size = size - sizeof(ngx_pool_t);    // 该内存池可分配的空间大小
   
    // 比较该内存池一次可分配的最大空间，小块内存一次最多给用户提供4095个字节，即使其size远大于4095
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;		// 指向第一个可以分配内存的小块内存。
    p->chain = NULL ; 
    p->large = NULL;
    p->cleanup = NULL ; // 没有设置回调函数
    p->log = log ; 

    return p;
}
```

`nginx`中的内存池是在创建的时候就设定好了大小，在以后分配小块内存的时候，如果内存不够，则是重新创建一块内存串到内存池中，而不是将原有的内存池进行扩张。==当要分配大块内存是，则是在内存池外面再分配空间进行管理的，称为大块内存池。==

创建内存池函数先申请`size`字节大小的内存，然后将**last**指针指向可分配内存的起始位置，**end**指针指向该内存池的最后位置。其中**max**指向的是该内存池一次可分配的最大的大小，若当前内存池可分配大小大于`NGX_MAX_ALLOC_FROM_POOL(4k)`，则`max`为`NGX_MAX_ALLOC_FROM_POOL`；否则max的值为当前内存池可分配大小。

#### 2.2 内存申请函数

`nginx`对内存的管理分为大内存与小内存，当某一个申请的内存大于某一个值时，就需要从大内存中分配空间，否则从小内存中分配空间。

内存申请函数有三个，实现也特别的简单，其中调用了**小块内存申请和大块内存申请**两个函数，后面我将会对那两个函数进行简单的讲解，首先我们先看一下内存申请的三个函数的实现：

```c++
// 内存分配函数，支持内存对齐
void *ngx_palloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {	// 申请小块内存
        return ngx_palloc_small(pool, size, 1) ; // 第三个传入的为true，支持内存对齐 
    }
	// 申请大块内存
    return ngx_palloc_large(pool, size);
}

// 内存分配函数，不支持内存对齐
void *ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {	// 申请小块内存
        return ngx_palloc_small(pool, size, 0) ; // 第三个参数为false不支持内存对齐
    }
	// 申请大块内存
    return ngx_palloc_large(pool, size) ; 
}

// 内存分配函数，支持内存初始化0
void *ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p ; 
    p = ngx_palloc(pool, size);		// 申请内存
    if (p) {
        ngx_memzero(p, size);		// 初始化
    }

    return p;
}
```


上面三个函数就是三种不同的申请内存函数，其中关于是否支持内存对齐是由`ngx_palloc_small`的第三个参数决定的，为0则不支持对齐，为1则支持对齐；第三个可初始化为0的函数首先调用了`ngx_palloc`申请空间，申请成功后则调用`ngx_memzero`来进行初始化，`ngx_memzero`是对`memset`函数的封装，其定义如下：

```c++
// 将buf初始化为0
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
```


#### 2.3 小块内存申请

```c++
// ngx_inline 实际是 inline 
static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;

    p = pool->current;  // 指向小块内存池的第一个可用的小块内存

    do {
        m = p->d.last;

        if (align) {		// 指针对齐
            m = ngx_align_ptr(m, NGX_ALIGNMENT) ; // 找到第一块对齐后的内存地址
        }

        if ((size_t) (p->d.end - m) >= size) {	// 内存池空间足够，可以直接分配内存
            p->d.last = m + size;
            return m;
        }
        p = p->d.next ;

    } while (p);

	// 若内存池分配都失败，则重新开辟一个内存池
    return ngx_palloc_block(pool, size);
}
```

`ngx_palloc_small`函数首先先将指针指向内存池的入口，紧接着判断是否需要内存对齐，内存对齐是一个定义的宏，其定义如下：

```c++
// 小块内存分配考虑字节对齐时的单位
#define NGX_ALIGNMENT   sizeof(unsigned long) 

// 将p指针按a对齐（将p调整为a的临近的倍数）
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
```

这是一个用来内存地址取整的宏，非常精巧，作用不言而喻，取整可以降低CPU读取内存的次数，提高性能。因为这里并没有真正意义调用`malloc`等函数申请内存，而是移动指针标记而已，所以需要自己实现该代码。 
在本函数中，将会依据不同的应用场景（32位4字节，64位 8字节）把m指针的地址 调整为与平台相关的`NGX_ALIGNMENT`（4 或者 8）的整数倍的地址上去。

指针对齐操作完成后，会依此判断当前内存池是否可以分配空间，若可以顺利分配，就返回分配内存的首地址；若均分配失败，就需要申请新的内存块。 
![](https://img-blog.csdnimg.cn/20200417232932470.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
上图就是对申请小块内存的一个简单描述。

#### 2.4 `ngx_palloc_block`内存块分配函数

当申请小块内存失败时，就需要重新申请一块内存块用来给用户分配内存，`ngx_palloc_block`函数就是解决这样一个问题，其代码如下：

```c++
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;

	// 计算内存块的大小
    psize = (size_t) (pool->d.end - (u_char *) pool);

	// 申请与之前内存块大小相同的一个内存块
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;  // 指向申请好的内存块

	// 对内存块的数据信息进行初始化
    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);		// m指向可分配空间的起始位置
    m = ngx_align_ptr(m, NGX_ALIGNMENT);// 使m指针对齐
    new->d.last = m + size;				// 偏移last指针，给用户分配内存

	// 因为小块内存分配失败，对每个failed++ ，若failed > 4 ，则更新内存池的入口指针
    for (p = pool->current; p->d.next ; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next ; 
        }
    }
   
   // p此时指向的是最后一个小块内存
    p->d.next = new;	// 将申请好的内存块连接到内存池中

    return m;
}
```

该函数的实现较为简单，主要功能就是申请一块和内存池中内存块大小相同的一个空间，再将数据块信息进行初始化，紧接着给用户分配空间。分配完成后，需要遍历整个内存池，将所有内存块的`failed+1`，当一个内存块四次内存分配都失败，说明该内存块的可用空间已经不多，就需要调整内存池的入口指针。 
![](https://img-blog.csdnimg.cn/20200417234237796.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70)  
通过上图我们就可以更好的理解该函数所进行的操作：申请好空间`new`后，先将`m`指向可分配内存的起始地址，再偏移`last`指针，为用户分配`size`大小的空间，将`end`指向该内存块的最后，然后将这个新的内存块连接到内存池中去。

#### 2.5 大块内存申请

```C++
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    p = ngx_alloc(size, pool->log);		// 通过malloc申请大块内存
    if (p == NULL) {
        return NULL;
    }

    n = 0;

	// 对管理大块内存的链表进行遍历，查看前三个大块内存头信息下时候有大块内存
	// 若前三个大块内存头信息下都有大块内存，则继续执行
	// 前三个大块内存内存头下有一个为空，就像申请的大块内存挂到该节点下
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

	// 大块内存的内存头是通过小块内存申请的,其头结构体是存储在小块内存中的
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);	// ngx_free就是free函数
        return NULL; // 连头结构体都申请不下来，就直接返回NULL了
    }
	
    large->alloc = p;			// 将alloc指向大块内存
    large->next = pool->large;	// 将该大块内存头插到内存池large处
   // 在内存池的头结构体链表的头节点中插入这个头结构体	
    pool->large = large;

    return p;
}
```

![](https://img-blog.csdnimg.cn/20200418180615470.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70)  
如上图所示，函数首先先通过`ngx_alloc`申请一块内存，然后通过循环查看大块内存链表的前三个内存头下有没有数据：如果前三个大块内存头信息下都有大块内存，则继续执行程序，在小块内存中申请一个大块内存头节点，并将该头节点以头插的方式加入链表中；若里面有一个为空，则将该内存头的`alloc`指向大块内存。

#### 2.6 内存释放函数（大块内存）

```c++
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

	// 遍历大块内存链表
    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {	// 如果alloc指向的是需要释放的内存，将该块内存释放掉，将指针置空
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc) ;	 // free(l->alloc);
            l->alloc = NULL ; 
            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}
```


我们可以看到，**内存池只提供了大块内存的释放而没有提供小块内存的释放**。实际上从内存池小块内存的分配方式来看，他也无法提供小块内存的释放，因为小块内存是通过`last`指针偏移来实现的，所以内存池小块内存的分配效率特别的高。正所谓鱼和熊掌不可兼得，小块内存仅靠两个指针就实现了内存的分配，那么对于回收就无法进行控制。

就`nginx`而言，==小块内存也是不需要进行回收的==，这与`nginx`的应用场景有关：**`nginx`的本质是`http`服务器**，`http`服务器是一个短链接的服务器，客户端（浏览器）发起一个`request`请求，到达`nginx`服务器以后，处理完成后`nginx`给客户端返回一个`response`响应，`http`服务器就主动断开`tcp`连接，这时就可以调用重置函数重置内存池。 
`http1.1`以后支持`keep-alive(60s)`即`http`服务器(`nginx`)返回响应以后，需要等待60s，60s之内客户端又发来请求，重置这个时间，否则60s之内没有客户端发来的响应， `nginx`就主动断开连接，此时 `nginx`可以调用`ngx_reset_pool`重置内存池，等待下一次该客户端的请求。

> + 长连接的服务器：不管客户端有没有又发过来请求，它和客户端之间的连接是不能断开的。资源就不可以去释放了，这样的应用场景 适合用之前的 `SGI STL`的二级空间配置器内存池。无论是大块 还是小块，从效率上而言：`SGI STL`的二级空间配置器内存池的小块申请 是绝对不如`Nginx`内存池的小块内存申请（last偏移即可）。但是`SGI STL`的二级空间配置器内存池的小块 和 大块是可以使用在任何的场景之下的，因为它提供了小块 和 大块 内存开辟和释放的。
> + `HTTP`服务器（`Nginx`）是短连接服务器，在用户发过来请求的时候，为了处理这个请求涉及的模块。可以为这些需要内存的模块 创建相应的内存池，在内存池上进行内存分配。若是给这个`client`响应之后，服务器就可以断开和这个`client`的连接了。此时，处理刚才请求涉及的所有的资源就都可以回收了。于是此时，`Nginx`可以调用`ngx_reset_pool`函数 进行重置内存池了，等待下一次client的请求：继续使用这个`Nginx`内存池了。
> + 基于短连接的服务场景，请求处理完了以后。（此次连接的内存上所有的数据都是无效的了，服务器和`client`没有任何关系了）这次请求，所有的资源就都可以进行回收了。此时就是调用`ngx_reset_pool`函数 进行重置内存池。
>   `Nginx`内存池因此就仅仅非常适合于 `http`服务器了，基于短连接的给`client`提供服务的应用场景。

#### 2.7 内存池重置函数

```c++
void ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

	// 遍历管理大块内存的链表，将所有的大块内存都释放掉
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

	// 重置所有的last指针，将其指向可分配的起始位置,   这里没有调用析构函数ok吗？
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;	// 内存池入口地址指向第一个内存块
    pool->chain = NULL;
    pool->large = NULL;
}
```


对于内存重置函数，首先先将内存池中的所有大块内存都释放掉，再将所有的内存块的last指针重置即可。因为内存池中只有第一个内存块的结构为`ngx_pool_t`，而剩余的都为`ngx_pool_data_t`，所以若按源码来看，将会造成部分的空间浪费，对于第二个for循环，我认为可做如下改进：

```c++
// 处理第一块内存池
p = pool;
p->d.last = (u_char *) p + sizeof(ngx_pool_t);
p->d.failed = 0;

// 第二块内存池到最后一块
for (p = p->d.next; p; p = p->d.next) {
   p->d.last = (u_char *) p + sizeof(ngx_pool_data_t);
   p->d.failed = 0;
}
```

#### 2.8 添加外部清理函数

```c++
ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

	// 通过小块内存开辟函数来申请清理外部资源的操作头信息
    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

	// 如果size指定大小，就为data开辟size大小的内存
    if (size) {
        c->data = ngx_palloc(p, size) ; 
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;	// 将函数句柄置为空，等待用户赋值
    c->next = p->cleanup; // 将该头信息头插到相关链表
    p->cleanup = c ; 
   
    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}
```

在说明该函数前，我们先看这样一个问题： 
![](https://img-blog.csdnimg.cn/20200418211117802.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
在该内存池的一块大块内存上，有一个指针指向一个外部资源，当我们释放内存池的时候就需要先将外部的资源释放掉，进而在释放内存池中的内存，这就需要提供一个外部清理函数，对于该函数我们可做如下运用： 
![](https://img-blog.csdnimg.cn/20200418211333874.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
将清理外部资源函数的赋予handler，并将指向资源的指针赋予`data`作为`handler`函数的参数。

#### 2.9 内存池销毁函数

```c++
void ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

	// 清理外部资源
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {	// 如果用户设置了清理回调函数，调用该函数清理外部资源
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);	// 调用handler函数
        }
    }

	// 清理大块内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

	// 清理内存池
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}
```


销毁内存池主要分为三步：先释放外部资源，再释放大块内存，最后释放内存池。这三步的顺序不可乱，因为外部资源和大块内存的地址信息均存储在内存池中，所以内存池必须最后释放，最先释放的肯定是外部资源，接着就是大块内存，这样才能保证不会造成内存泄漏。



