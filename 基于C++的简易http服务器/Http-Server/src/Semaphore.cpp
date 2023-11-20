#include "Semaphore.h"
Semaphore::Semaphore(size_t reslimit ) : resLimit_(reslimit ) {} 

Semaphore::~Semaphore(){} 

void Semaphore::wait()
{
    std::unique_lock<std::mutex > lock(mtx_) ;
    cond_.wait(lock, [this]()-> bool {return resLimit_ > 0 ; } ) ;
    resLimit_ -- ; // 数量减少一下
}
void Semaphore::post()
{
   // std::cout << "hello world" << std::endl ; 
    std::unique_lock<std::mutex > lock(mtx_) ; 
    resLimit_++ ; 
    cond_.notify_all() ;  
}
