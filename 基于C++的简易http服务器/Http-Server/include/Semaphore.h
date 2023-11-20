#ifndef SEMAPHORE
#define SEMAPHORE 

#include "public.h"

class Semaphore {
public: 
    Semaphore(size_t reslimit = 0 ) ; 
    ~Semaphore() ; 
    void wait() ; 
    void post() ; 
private: 
    size_t resLimit_ ; // 资源计数  
    std::mutex mtx_; 
    std::condition_variable cond_ ;  
};
#endif 