





`ngx_mem_pool.h`

```C++
#pragma once
#include <cstring> 
/*
	移植nginx内存池的代码，使用oop实现
*/

// 为什么这里的 next 的类型为 neg_pool_s * 而不是ngx_pool_data_t * 

typedef uintptr_t       ngx_uint_t;
typedef void(*ngx_pool_cleanup_pt)(void* data);
typedef uintptr_t       ngx_uint_t;
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
#define u_char unsigned char 
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
struct ngx_pool_s ;  

struct ngx_pool_data_t {    // 小块内存头
	unsigned char* last ; // 小块内存池可用地址的起始地址 
	unsigned char* end;  // 小块内存池可用地址的末尾地址
	ngx_pool_s * next;  //	所有小块内存都被串在了一条链表上
	unsigned int failed ;  // 记录当前小块内存池分配失败的次数
} ; 

struct ngx_pool_large_s {  // 大块内存头
	ngx_pool_large_s* next ;   
	void* alloc ;  
};
  
// 大块内存外部资源释放头
struct ngx_pool_cleanup_s { 
	ngx_pool_cleanup_pt handler ; // 定义函数指针，保存清理操作的回调函数 
	void* data ; // 传递给回调函数的参数 
	ngx_pool_cleanup_s* next; 
}; 

// 内存池的头部信息
struct ngx_pool_s
{
	ngx_pool_data_t d ; // 存储当前小块内存池的使用情况  
	size_t max;       // 
	ngx_pool_s* current ; // 指向第一个可以提供小块内存分配的小块内存池
	ngx_pool_large_s* large ; // 指向大块内存(链表)的入口地址 
	ngx_pool_cleanup_s* cleanup;  // 指向所有预制的清理操作的回调函数的入口
};


#define ngx_align(d, a)  ((d) + (a - 1 ) & ~(a  - 1 ) ) 
#define NGX_ALIGNMENT sizeof(unsigned long ) 
const int  ngx_pagesize = 4096 ; // 默认一个页面的大小 
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1; // ngx小块内存池可分配的最大空间

// 定义常量，表示一个默认的ngx内存池开辟的大小
const int NGX_DEFAULT_POOL_SIZE = 16 * 1024 ; 

const int  NGX_POOL_ALIGNMENT = 16; // 内存池大小按照16字节进行对齐
// 内存池最小要放下一个内存池的头部信息节点和两个大块内存的头部结构体 
const int  NGX_MIN_POOL_SIZE = ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)), NGX_POOL_ALIGNMENT); 

class ngx_mem_pool
{
public :
	// 创建指定大小的size的内存池，但小块内存池不超过1个页面的大小
	void* ngx_create_pool(size_t size); // 这里不需要日志参数了	
	
	// 考虑内存字节对齐，从内存池申请size大小的内存
	void* ngx_palloc(size_t size); 
	void* pnalloc(size_t size); // 不用考虑内存对齐
	void* ngx_pcalloc(size_t size); // 本质上调用ngx_palloc实现内存分配，但是会初始化0

	void ngx_pfree(void* p);  // 释放大块内存

	// 内存重置函数
	void ngx_reset_pool() ;  
	

	// 添加回调清理操作函数
	ngx_pool_cleanup_s* ngx_pool_cleanup_add( size_t size) ; 
	
	// 实际上其内部代码的逻辑就是ngx_destroy_pool中的逻辑
	~ngx_mem_pool() ;  
private:
	ngx_pool_s* pool;  // 指向nginx内存池的指针

	// 小块内存分配
	void* ngx_palloc_small( size_t size, ngx_uint_t align );
	
	// 大块内存分配
	void* ngx_palloc_large( size_t size) ;  

	// 分配新的小块内存池
	void* ngx_palloc_block( size_t size) ; 

};


```



`ngx_mem_pool.cpp`

```C++
#include "ngx_mem_pool.h"
#include <cstdlib>
// 对于ngx_mem_pool的接口的定义

// 创建指定大小的size的内存池，但小块内存池不超过一个页面
void* ngx_mem_pool::ngx_create_pool(size_t size)
{

	pool = (ngx_pool_s*)std::malloc(size) ;  // C++中无法将隐式的void*转化为其他类型，需要强制类型转换
	if (pool == nullptr) return nullptr  ; 

	// 小块内存的信息的设置
	pool->d.last = (u_char*)pool + sizeof(ngx_pool_s) ; 
	pool->d.end = (u_char*)pool + size ; 
	pool->d.next = nullptr ; 
	pool->d.failed = 0 ; 

	size = size - sizeof(ngx_pool_s) ; 
	pool->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL ;  

	// 内存头的信息的设置
	pool->current = pool ;  
	pool->large = nullptr; 
	pool->cleanup = nullptr ; 
	return pool ;  
}


// 考虑内存字节对齐，从内存池中申请size字节的内存
void* ngx_mem_pool::ngx_palloc(size_t size)
{
	if (size <= pool->max )
	{
		return ngx_palloc_small( size, 1); 
	}
	return ngx_palloc_large(size) ;  
}

void* ngx_mem_pool::pnalloc(size_t size)// 不用考虑内存对齐
{
	if (size <= pool->max)
	{
		return ngx_palloc_small(size, 0) ; 
	}
	return ngx_palloc_large(size);
}
void* ngx_mem_pool::ngx_pcalloc(size_t size) // 本质上调用ngx_palloc实现内存分配，但是会初始化0
{
	void* p;

	p = ngx_palloc(size) ; 
	if (p) {
		ngx_memzero(p, size);
	}
	return p;
}

void* ngx_mem_pool::ngx_palloc_small( size_t size, ngx_uint_t flag)
{
	u_char* m ;  
	ngx_pool_s*  p = pool->current ; 
	do {
		m = p->d.last ; 
		if ( flag ) 
		{
			// 根据对齐数调整一下m的地址
			m = ngx_align_ptr(m , NGX_ALIGNMENT ) ; 
		}
		if ((size_t)(pool->d.end - m) >= size ) 
		{
			p->d.last = m + size ;
			return m ; 
		}
		p = p->d.next ;  
	} while (p);  
	return  ngx_palloc_block(size) ;   
}

// 分配小块内存，需要挂载到内存池上
void* ngx_mem_pool::ngx_palloc_block( size_t size)
{
	u_char* m ; 
	size_t       psize;
	ngx_pool_s* p, *nw ; 

	psize = (size_t)(pool->d.end - (u_char*)pool) ; // 计算应该开辟的内存池的大小

	m = (u_char*)malloc(psize) ; 

	if (m == nullptr ) {
		return nullptr ;
	}
	
	nw = (ngx_pool_s*)m ; // 进行类型转换 

	nw->d.end = m + psize ;

	nw->d.next = nullptr ;
	
	nw->d.failed = 0 ; 


	m += sizeof(ngx_pool_data_t) ; // m指向第一个可用的地址。
	m = ngx_align_ptr(m, NGX_ALIGNMENT) ;  // 将m的地址进行对齐
	nw->d.last = m + size ; // 调整当前块的可使用的地址。 

	// 修改p->d.failed的大小
	for (p = pool->current; p->d.next; p = p->d.next) {
		if (p->d.failed++ > 4) {
			pool->current = p->d.next;
		}
	}

	// 将当前块的地址挂载到内存池上面
	p->d.next = nw ;

	return m ;  // m现在是需要返回的内存了

}

void* ngx_mem_pool::ngx_palloc_large( size_t size )
{
	void* p;
	ngx_uint_t         n;
	ngx_pool_large_s* large ; 

	p = malloc( size ) ;
	if (p == nullptr ) {
		return nullptr ;
	}

	n = 0 ;

	for (large = pool->large ; large ; large = large->next ) 
	{
		if (large->alloc == nullptr ) {
			large->alloc = p ; 
			return p ; 
		}
		// 如果前三个大块内存头都不空的话，就创建一个新的
		if (n++ > 3) {
			break;
		}
	}

	large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s), 1 ) ;
	
	// 如果大块内存头都申请不下来，直接释放之前申请的大块内存的资源
	if (large == nullptr ) {
		free(p) ;  
		return nullptr ;
	}

	large->alloc = p ; // 将大块内存挂载到新申请的大块内存头上面
	
	// 挂载大块内存头
	large->next = pool->large ;  
	pool->large = large;

	return p;
}
void ngx_mem_pool::ngx_pfree(void* p) // 释放大块内存
{
	ngx_pool_large_s* l ; 

	for (l = pool->large ; l ; l = l->next ) {
		if (p == l->alloc ) {
			free(l->alloc);
			l->alloc = nullptr; 
			return ;
		}
	}
	return  ;
}
void ngx_mem_pool::ngx_reset_pool()
{
	ngx_pool_s* p ;
	ngx_pool_large_s* l ; 

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	p = pool; 
	p->d.last = (u_char*)p + sizeof(ngx_pool_s);
	p->d.failed = 0 ;  
	for (p = pool->d.next ; p; p = p->d.next )
	{
		p->d.last = (u_char*)p + sizeof( ngx_pool_s ) ;
		p->d.failed = 0 ; 
	}

	pool->current = pool ; 
	pool->large = nullptr ;
}
	
	// 内存池销毁函数
ngx_mem_pool::~ngx_mem_pool()
{
	ngx_pool_s* p, * n;
	ngx_pool_large_s* l;
	ngx_pool_cleanup_s* c;

	// 释放大块内存上的外部资源
	for (c = pool->cleanup ; c; c = c->next) {
		if (c->handler) {
			c->handler(c->data ) ;  // 我们程序员自己需要明白c->data的类型
		}
	}

	// 释放大块内存
	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc) ; 
		}
	}

	// 释放小块内存
	for (p = pool, n = pool->d.next;  ; p = n, n = n->d.next) {
		free(p);
		if (n == nullptr ){
			break ; 
		}
	}
}

	// 添加回调清理操作函数
ngx_pool_cleanup_s* ngx_mem_pool::ngx_pool_cleanup_add(size_t size)
{
	ngx_pool_cleanup_s* c;

	// 分配一个大小为ngx_pool_cleanup_s的内存
	c = (ngx_pool_cleanup_s*)ngx_palloc(sizeof(ngx_pool_cleanup_s));
	if (c == nullptr ) {
		return nullptr ;
	}

	if (size) {
		c->data = ngx_palloc( size) ; // 为回调函数参数开辟内存
		if (c->data == nullptr ) {
			return  nullptr ;
		}

	}
	else {
		c->data = nullptr ; // 说明回调函数不需要参数  
	}

	c->handler = nullptr ; 


	c->next = pool->cleanup ; 
	pool->cleanup = c ; 

	return c ; 
}
```



`test.cpp`

```C++
#include "ngx_mem_pool.h" 
#include <stdio.h>
#include <stdlib.h>
typedef struct Data stData;
struct Data
{
    char* ptr;
    FILE* pfile;
};

void func1(void * p)
{
    char *pt  = (char*)p ;  
    printf("free ptr mem!\n");
    free(pt);
}
void func2(void * pf)
{
    FILE* pt = (FILE*)pf ;  
    printf("close file!\n") ; 
    fclose(pt) ; 
}

int  main()
{
    // 512 - sizeof(ngx_pool_t) - 4095   =>   max
    ngx_mem_pool mypool ; 
    ngx_pool_s* pool =(ngx_pool_s*) mypool.ngx_create_pool(512) ;
    if ( pool == nullptr ) 
    {
        printf("ngx_create_pool fail...");
        return -1 ;
    }

    void* p1 = mypool.ngx_palloc( 128); // 从小块内存池分配的
    if (p1 == nullptr )
    {
        printf("ngx_palloc 128 bytes fail...");
        return -1 ;
    }

    stData* p2 = (stData*)mypool.ngx_palloc(512); // 从大块内存池分配的
    if (p2 == nullptr )
    {
        printf("ngx_palloc 512 bytes fail...");
        return -1 ;
    }
    p2->ptr = (char*)malloc(12) ; 
    strcpy(p2->ptr, "hello world") ;  
    p2->pfile = fopen("data.txt", "w") ; 

    ngx_pool_cleanup_s* c1 = mypool.ngx_pool_cleanup_add( sizeof(char*));
    c1->handler = func1 ; 
    c1->data = p2->ptr ; 

    ngx_pool_cleanup_s* c2 = mypool.ngx_pool_cleanup_add(sizeof(FILE*));
    c2->handler = func2 ; 
    c2->data = p2->pfile ; 

    // 1.调用所有的预置的清理函数 2.释放大块内存 3.释放小块内存池所有内存
    // 这里可以将这个函数的逻辑写入析构函数当中

    return 0 ;
}

```

