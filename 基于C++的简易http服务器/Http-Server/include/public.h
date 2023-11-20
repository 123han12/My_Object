#ifndef PUBLIC 
#define PUBLIC 

#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue> 
#include <atomic>
#include <functional> 
#include <thread>
#include <memory> 
#include <iostream>
#include <map>

const int INIT_THREAD_SIZE = std::thread::hardware_concurrency() ; // 初始的线程的数量是CPU核心的数量  
const int MAX_THREAD_HOLD = 20 ;  
const int MAX_TASK_HOLD = 10 ;
const int THREAD_MAX_IDLE_TIME = 60 ; 
using ulong_ = unsigned long long ; 
#endif 
