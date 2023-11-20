#ifndef THREAD 
#define THREAD 

#include "public.h"
class Thread {
public:
    using Func = std::function<void(int )> ;  
    Thread(Func func ) ; 
    ~Thread(); 
    void start() ;  

    ulong_ getId() const ;  
private:
    Func threadFunc ; 
    static ulong_ generateId ;  // 注意ulong 可以天然的溢出
    ulong_ threadId ;  
};

#endif 