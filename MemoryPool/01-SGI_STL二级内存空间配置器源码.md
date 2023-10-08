> `SGI STL`空间配置器介绍
> --------------
>
> `SGI STL`包含了一级空间配置器和二级空间配置器，其中一级空间配置器`allocator`采用**`malloc`和`free`**来管理内存，和C++标准库中提供的`allocator`是一样的，但其二级空间配置器`allocator`采用了基于**`freelist`自由链表**原理的内存池机制实现内存管理。
>
> `SGI STL`的二级空间配置器的内存池主要是给 `C++`的容器底层进行内存管理的使用。
>
> 在内存管理的时候，因为小块内存的频繁开辟`malloc` 释放`free` 会影响性能，产生内存碎片，导致大块连续内存的缺少，所以在申请内存时**以128字节为分界线**，将小于等于128字节 和 大于128字节的内存开辟分开。当小于等于128字节时，由内存池负责开辟；当大于128字节时，由一级空间配置器负责开辟释放
>
> 源码剖析
> ----------------------------------------------------------------------------------
>
> ### 1\. 空间配置器
>
> 首先，我们先从认识一下两级空间配置器的内存管理类的名称，本篇主要涉及二级空间配置器的源码，一级空间配置器会略有提及
>
> ```C++
> // 一级空间配置器内存管理类
> template <int __inst>
> class __malloc_alloc_template;
> 
> // 对一级空间配置器的一个简单封装
> typedef __malloc_alloc_template<0> malloc_alloc;
> 
> // 二级空间配置器内存管理类
> template <bool threads, int inst>
> class __default_alloc_template;
> ```
>
>
> 由下面代码容器的定义，可以看到容器的默认空间配置器是 `__STL_DEFAULT_ALLOCATOR( _Tp)`，它是一个==宏定义==，由该宏，可以看到`__STL_DEFAULT_ALLOCATOR`通过宏控制有两种实现，一种是allocator< T >，另一种是`alloc`，这两种分别就是`SGI STL`的一级空间配置器和二级空间配置器的实现
>
> ```C++
> // stl_vector.h
> // vecor的类模板
> template <class _Tp, class _Alloc = __STL_DEFAULT_ALLOCATOR(_Tp) >
> class vector : protected _Vector_base<_Tp, _Alloc> 
> 
> // stl_config.h
> // __STL_DEFAULT_ALLOCATOR
> # ifndef __STL_DEFAULT_ALLOCATOR
> #   ifdef __STL_USE_STD_ALLOCATORS
> #     define __STL_DEFAULT_ALLOCATOR(T) allocator< T >
> #   else
> #     define __STL_DEFAULT_ALLOCATOR(T) alloc
> #   endif
> # endif
> ```
>
>
> ### 2\. 重要的类型和变量定义
>
> ```C++
> // 内存池的粒度信息
> enum {_ALIGN = 8};		 // 小块区域的上调信息 
> enum {_MAX_BYTES = 128}; // 大小快内存的边界
> enum {_NFREELISTS = 16}; // _MAX_BYTES/_ALIGN
> 
> // 每一个chunk块的头信息
> union _Obj {
> 	union _Obj* _M_free_list_link;  // 指向下一个chunk块，相当于链表的next域
>     char _M_client_data[1];    /* The client sees this. */
> };
> 
> // 组织所有自由链表的数组，数组的每一个元素的类型是_Obj*，全部初始化为0
> static _Obj* __STL_VOLATILE _S_free_list[_NFREELISTS] ; 
> 
> // 记录内存池分配情况
> static char* _S_start_free;	// 内存池的起始地址，初始值为0
> static char* _S_end_free;	// 内存池的结束位置，初始值为0
> static size_t _S_heap_size;	// 内存池申请的总的空间的大小，初始值为0
> ```
>
> 由上面的定义可以可知，二级空间配置器使用了一个长度为16的指针数组`_S_free_list`管理16个自由链表。为了方便管理，`SGI STL`二级空间配置器将每次申请的内存都调整为8的倍数，即每个自由链表各自管理大小为8，16，24，32，40，48，56，64，72，80，88，96，104，112，120，128字节的`chunk`块，每个`chunk`块起始地址会被强转为`Obj*`类型，并且头部存储下一个`chunk`块的地址。结构如下图所示： 
> ![](https://img-blog.csdnimg.cn/20200416183641934.png)
>
> ### 3\. 两个重要的辅助函数
>
> ```c++
> // 将 _bytes 上调至最邻近8的倍数
> static size_t  _S_round_up(size_t __bytes) 
>   { return (((__bytes) + (size_t) _ALIGN-1) & ~((size_t) _ALIGN - 1)); }
> 
> // 返回 __bytes 大小的chunk块位于 _S_free_list 中的编号， 找到对应的槽
> static  size_t _S_freelist_index(size_t __bytes) {
>       return (((__bytes) + (size_t)_ALIGN-1)/(size_t)_ALIGN - 1);
> }
> ```
>
> 关于第一个对齐的函数，会将输入的字节调整为比其大的最邻近8的倍数，这里从二进制的角度出发，当二者进行与运算后，得到的结果只能为8的倍数，即可做如下理解：【`size_t`的类型一般是`unsigned int`】
> ![](https://img-blog.csdnimg.cn/20200416184253850.png)
>
> ### 4\. 内存池管理函数
>
> ```C++
> // 分配内存函数
> static void* allocate(size_t __n)
> 
> // 把分配好的chunk块进行连接，添加到自由链表当中
> static void* _S_refill(size_t __n);
> 
> // 分配相应内存字节大小的chunk块，并且初始化_S_start_free、_S_end_free、_S_heap_size
> static char* _S_chunk_alloc(size_t __size, int& __nobjs);
> 
> // 把chunk块归还到内存池
> static void deallocate(void* __p, size_t __n);
> 
> // 内存池扩容/缩容函数
> template <bool threads, int inst>
> void* __default_alloc_template<threads, inst>::reallocate(void* __p, size_t __old_sz, size_t __new_sz);
> ```
>
>
> #### 4.1 allocate函数
>
> ```C++
>   static void* allocate(size_t __n)
>   {
>     void* __ret = 0;
> 
> 	// 申请的字节大于128，调用一级空间配置器的allocate函数
>     if (__n > (size_t) _MAX_BYTES) {
>       __ret = malloc_alloc::allocate(__n);
>     }
>     else {
>       // 将__my_free_list指向对应的自由链表的槽的地址
>       _Obj* __STL_VOLATILE* __my_free_list = _S_free_list + _S_freelist_index(__n);
> 
> 	  // 加锁，保证线程安全，出作用域自动解锁
>       _Lock __lock_instance;
>       
>       // 指向只有链表的头节点
>       _Obj* __RESTRICT __result = *__my_free_list;
>       if (__result == 0)    // 该自由链表没有可使用的chunk块，则申请内存
>         __ret = _S_refill(_S_round_up(__n));
>       else {  				// 将第一个chunk块返回给用户，可以理解为头删，时间复杂度为O(1)
>         *__my_free_list = __result -> _M_free_list_link;
>         __ret = __result;
>       }
>     }
> 
>     return __ret;
>   };
> ```
>
> 该函数的逻辑为：首先判断申请字节是否大于128，大于则调用一级空间配置器，否则继续；紧接着判断需要分配内存的自由链表下有没有可分配的`chunk`块：如果有，就将第一个`chunk`块返还给用户；否则就需要申请新的`chunk`块添加到该链表下。 
> ![](https://img-blog.csdnimg.cn/20200416185811380.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
> 如上图所示，假设申请的字节为16字节，且对应的自由链表有可分配的内存，就将第一个`chunk`块分配出去，将`_S_free_list[1]`指向下一个`chunk`块。
>
> #### 4.2 \_S\_refill函数
>
> ```C++
> template <bool __threads, int __inst>
> void* __default_alloc_template<__threads, __inst>::_S_refill(size_t __n) // 这里传入的一定是8的倍数
> {
> 	
>     int __nobjs = 20;								// 默认申请20个chunk块
>     
>     // 分配相应个数的chunk块所需的内存，__nobjs传的是引用
>     char* __chunk = _S_chunk_alloc(__n, __nobjs);	
>     _Obj* __STL_VOLATILE* __my_free_list;			// 指向相应的槽
>     _Obj* __result;									// 返回给用户的chunk块的起始地址
>     _Obj* __current_obj;							// 指向前一个chunk块
>     _Obj* __next_obj;								// 指向下一个chunk块
>     int __i;
> 
>     if (1 == __nobjs) return(__chunk);				// 如果只申请了一个chunk块，则直接返回
>     
>    // 下述操作是将申请的chunk块进行一个链表形式的串联
>    __my_free_list = _S_free_list + _S_freelist_index(__n);
> 
>     /* Build free list in chunk */
>       __result = (_Obj*)__chunk;
>       *__my_free_list = __next_obj = (_Obj*)(__chunk + __n) ; // 槽内存储第二chunk的地址
> 
> 	  // 将分配的chunk块以链表的形式连接起来
>       for (__i = 1; ; __i++) {
>         __current_obj = __next_obj;
>         __next_obj = (_Obj*)((char*)__next_obj + __n);
>         if (__nobjs - 1 == __i) {  // 如果是最后一个chunk块，则将该chunk块的next域置为0
>             __current_obj -> _M_free_list_link = 0;
>             break;
>         } else {
>             __current_obj -> _M_free_list_link = __next_obj;
>         }
>       }
>     return(__result);
> }
> ```
>
>
> 若需要分配内存的自由链表下没有空闲的`chunk`块，就需要调用该函数申请空间，默认申请的是20个`__n`字节的chunk块，申请成功后，将第一个`chunk`块返回给用户，再将其余的`chunk`块以链表的形式连接起来。
>
> 需要注意的是，这里调用了`_S_chunk_alloc`申请`chunk`块，其中`nobjs`传的是引用参数，因为`_S_chunk_alloc`函数会根据内存池中未分配给自由链表的内存的大小来动态的调整`nobjs`的个数，详情见`_S_chunk_alloc`函数详解。 
> ![](https://img-blog.csdnimg.cn/20200416222634883.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
> 如上图所示，以8字节为例，指针数组`_S_free_list[0]`所指向的内存块就是`_S_chunk_alloc`所申请的，申请成功后，将第一个`chunk`块返回给用户，将下面剩余的`chunk`块依此以链表形式连接等待用户调用
>
> #### 4.3 `_S_chunk_alloc`函数
>
> ```C++
> template <bool __threads, int __inst>
> char* __default_alloc_template<__threads, __inst>::_S_chunk_alloc(size_t __size, int& __nobjs)
> {
>     char* __result;
>     size_t __total_bytes = __size * __nobjs;			// 需要申请的字节数
>     size_t __bytes_left = _S_end_free - _S_start_free;	// 内存池剩余空间
> 
>     if (__bytes_left >= __total_bytes) {
>     	// 内存池剩余空间大于等于需要申请的内存，偏移_S_start_free后，直接返回
>         __result = _S_start_free;
>         _S_start_free += __total_bytes ; 
>         return(__result) ; 
>     } else if (__bytes_left >= __size) {
>     	// 内存池剩余空间不能满足所有需求，但是可以分配一个以上的chunk块
>         __nobjs = (int)(__bytes_left/__size);  // 计算所能分配的chunk块的个数
>         __total_bytes = __size * __nobjs;	   // 分配的总的字节数
>         __result = _S_start_free;
>         _S_start_free += __total_bytes;
>         return(__result);
>     } else {
>     	// 内存池剩余空间连一个chunk块的大小都无法提供，需要开辟空间
>         size_t __bytes_to_get = 2 * __total_bytes + _S_round_up(_S_heap_size >> 4);
>         
>         // 若内存池有剩余的空间，剩余的大小一定是8的倍数，则将该剩余空间放到相应的自由链表下
>         if (__bytes_left > 0) {
>         	// 寻找相应的槽的地址。
>             _Obj* __STL_VOLATILE* __my_free_list = _S_free_list + _S_freelist_index(__bytes_left);
> 			// 将该剩余空间头插到对应的自由链表
>             ((_Obj*)_S_start_free) -> _M_free_list_link = *__my_free_list;
>             *__my_free_list = (_Obj*)_S_start_free;
>         }
>         _S_start_free = (char*)malloc(__bytes_to_get) ; // 调用malloc进行内存的分配
> 		// 内存开辟失败
>         if (0 == _S_start_free) {
>             size_t __i;
>             _Obj* __STL_VOLATILE* __my_free_list;
> 	    		_Obj* __p;
>             
>             // 从_size 大小的 chunk块后开始找，找出一块空闲的chunk块的返回
>             for (__i = __size; __i <= (size_t) _MAX_BYTES ; __i += (size_t) _ALIGN) {
>                 __my_free_list = _S_free_list + _S_freelist_index(__i);
>                 __p = *__my_free_list;
>                 if (0 != __p) {		// 该自由链表中有尚未使用的chunk块
>                 	// 将第一个chunk块返回
>                     *__my_free_list = __p -> _M_free_list_link;
>                     _S_start_free = (char*)__p;
>                     _S_end_free = _S_start_free + __i;
>                     // 调用自身，修正__nobjs
>                     return(_S_chunk_alloc(__size, __nobjs)) ; 
>                 }
>             }
>             // 若后面的自由链表均没有空闲的chunk块，则调用一级空间配置器
> 	    		_S_end_free = 0;	// In case of exception.
>         		_S_start_free = (char*)malloc_alloc::allocate(__bytes_to_get);
>         }
>         _S_heap_size += __bytes_to_get ; 
>         _S_end_free = _S_start_free + __bytes_to_get;
>         // 调用自身，修正__nobjs
>         return(_S_chunk_alloc(__size, __nobjs));
>     }
> }
> ```
>
> 上述函数以`_S_end_free - _S_start_free`来判断内存池的剩余空间。如果剩余空间充足，就直接调出20个`chunk`块返回给 `_S_free_list`。如果剩余空间不足以提供20个`chunk`块，但还足够供应一个以上的`chunk`块，就拨出这不足20个`chunk`块的空间出去。这时候其引用传递的 `nobjs`参数将被修改为实际能够供应的区块数。如果内存池连一个区块空间都无法供应，此时便需利用 `malloc()`从`heap`中配置内存，为内存池添加内存以应付需求。新剩余空间的大小为需求量的两倍，==再加上一个随着配置次数增加而愈来愈大的附加量。== 
> ![](https://img-blog.csdnimg.cn/20200417124612622.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
> 上图的例子表示在初始状态所有自由链表为空的情况下，用户申请8字节，这时系统会申请320个字节，其中将160字节下挂到自由链表下，剩余的等待再次申请内存时使用。当第一次用户申请的20个`chunk`块使用完成后，再次申请时就会再次调用该函数，为自由链表再次提供20个`chunk`块。当第三次申请时，系统申请的内存大小会比320字节还要大，因为频繁的申请内存，说明对内存的需求较为强烈，所以`_S_heap_size`会逐次增加。 
> ![](https://img-blog.csdnimg.cn/20200417125239590.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70)  
> 上图显示了该函数调用的其他情况：当第一次申请8字节后，内存池的剩余空间为160字节，这时申请128字节，就会进入第二个条件else if { … }，将`nobjs`的个数调整为1，此时内存池的剩余空间就为32字节。下一次再申请40字节时，内存池剩余空间不足以分配一个`chunk`块，故需要重新开辟。
>
> 当开辟内存失败后，系统将会先在从大于该字节的第一个自由链表开始寻找有没有空闲的`chunk`块，有的话将`chunk`块返回并调整`nobjs`的个数；若后续的所有自由链表都为空，就需要调用一级空间配置器申请内存；一级空间配置器`malloc`失败，就会调用用户自定义的回调函数，一直循环调用直至有内存可以开辟。
>
> #### 4.4 deallocate函数
>
> ```C++
>  static void deallocate(void* __p, size_t __n)
>   {
>     if (__n > (size_t) _MAX_BYTES)		// 释放的空间大于128字节，调用一级空间配置器
>       malloc_alloc::deallocate(__p, __n);
>     else {
>       // 寻找到相应的自由链表 
>       _Obj* __STL_VOLATILE*  __my_free_list = _S_free_list + _S_freelist_index(__n);
>       _Obj* __q = (_Obj*)__p;
> 	  // 加锁保证线程安全，出作用域自动解锁
>       _Lock __lock_instance;
> 	  // 将需要释放的内存放到自由链表的头部
>       __q -> _M_free_list_link = *__my_free_list;
>       *__my_free_list = __q;
>     }
>   }
> ```
>
> 该函数首先判断chunk块大小，大于128 字节就调用第一级配置器，小于128字节就找出对应的 free list，将区块回收。  
> ![](https://img-blog.csdnimg.cn/20200417130741551.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MzcyNTIwMg==,size_16,color_FFFFFF,t_70) 
> 上图是一个简单的例子，是将8字节大小的第一个`chunk`块释放，归还到内存池中
>
> #### 4.5 reallocate函数
>
> ```C++
> template <bool threads, int inst>
> void* __default_alloc_template<threads, inst>::reallocate(void* __p,
>                                                     size_t __old_sz,
>                                                     size_t __new_sz)
> {
>     void* __result;
>     size_t __copy_sz;
> 
> 	// 需要扩容/缩容前后的大小均大于128字节，则调用realloc函数
>     if (__old_sz > (size_t) _MAX_BYTES && __new_sz > (size_t) _MAX_BYTES) {
>         return(realloc(__p, __new_sz));
>     }
>     
>     // 扩容/缩容后的大小仍在一个chunk块的大小之内，则直接返回 
>     if (_S_round_up(__old_sz) == _S_round_up(__new_sz)) return(__p);
>     
>     __result = allocate(__new_sz);							// 开辟新的空间
>     __copy_sz = __new_sz > __old_sz? __old_sz : __new_sz;	// 找出需要拷贝的最小空间数
>     memcpy(__result, __p, __copy_sz);						// 内存拷贝
>     deallocate(__p, __old_sz);								// 释放旧内存
>     return(__result) ; 
> }
> ```
>
>
> 内存池作用以及优点
> ---------
>
> ### 作用
>
> 防止小块内存频繁的分配、释放，造成很多的內存碎片出来，内存没有更多的连续的大内存块。对于小块内存的操作，一般都会使用内存池来进行管理
>
> ### 优点
>
> 1.  对于每个字节数的 chunk块分配，都是给出一部分进行使用，另一部分作为备用，这个备用可以给当前字节数使用，也可以给其它字节数使用
> 2.  对于备用内存池划分完 chunk块以后，如果还有剩余的很小的内存块，再次分配的时候，会把这 
>     些小的内存块再次分配出去，将备用内存池使用的干干净净
> 3.  当指定字节数内存分配失败以后，有一个异常处理的过程，对 `bytes ~ 128`字节所有的 chunk块 
>     进行查看，如果哪个字节数有空闲的 `chunk`块，直接借一个出去；如果内存开辟还是失败，则通过`_S_oom_malloc`调用回调函数：没设置回调，抛出异常；设置了回调，死循环调用函数直至内存开辟成功





