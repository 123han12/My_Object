#ifndef THREADPOOl 
#define THREADPOOl 
#include "public.h"
#include "Semaphore.h" 
#include "Any.h"

class Thread ;  
class Task ; 
// 接受返回值的Result 类的实现

// 线程池支持的模式
enum class  PoolMode {
    MODE_FIXED,  // 固定数量
    MODE_CACHED, // 动态增长
};
 
class Result {
public:
    Result() = default;
    Result(const Result&) = delete ; 
    Result& operator=(const Result&) = delete ; 
    Result(Result&&) = default ; 
    Result& operator=(Result&&) = default ;  

    Result(std::shared_ptr<Task> task, bool isvaild = true); 
    ~Result() = default;

    void setValue(Any any);   // 存储 task 的返回值
    // 用户调用此方法获取返回值
    Any get() ;  

private:
    Any any_; // 存储任务的返回值 
    Semaphore sem_;
    std::shared_ptr<Task> task_; // 指向对应的获取返回值的任务对像
    std::atomic_bool isVaild_; // 返回值是否有效
};

class Task {
public:
    Task(); 
    ~Task() = default;
    virtual Any HandlerRequest() = 0 ;  // 提供虚拟接口 
    void setResult(Result* result); 
    void exec() ;  
private:
    Result* result_;
};
// 线程池类型
class ThreadPool
{
public:
    ThreadPool() ; 

    // 为用户提供接口设置线程池的一些属性
    void setMode(PoolMode poolmode )  ;  
    void setInitThreadSize(int size) ; // 设置初始线程量
    void setTaskQueMaxThreshHold(int threshhold) ;  // 设置最大任务量
    void setThreadQueMaxThreshHold(int threshhold) ;  // 设置最大线程量

    // 启动线程池
    void start() ;   // 在start 之后 , set 无法再进行设置了

    void submitTask(std::shared_ptr<Task> myTask, Result* result) ;  


    ~ThreadPool() ;  
    ThreadPool(const ThreadPool&) = delete ; 
    ThreadPool& operator=(const ThreadPool&) = delete ;  

private: 
    void ThreadFunc(ulong_ threadId ) ; 
    bool checkRuningStatus() const  ;  
private:
    // 池子的属性
    std::atomic_bool PoolIsRuning_ ;  

    size_t  initThreadSize_ ; // 初始线程数量 
    std::atomic_int curIdleThread_ ;  //空闲的线程的个数 
    std::atomic_int threadCnt_; // 当前的线程的数量
    std::atomic_int taskCnt_ ; // 当前的任务的数量 
    

    size_t  maxThreadHold_ ;  // 最大线程值
    size_t  maxTaskHold_ ;  // 最大任务值

    PoolMode poolMode_ ; 


    std::mutex taskQueMtx_ ; 

    std::condition_variable notEmpty_ ;  // 通知消费者线程消费
    std::condition_variable notFull_ ;   // 通知生产者线程生产
    std::condition_variable exitCond_ ; //  等待线程资源全部回收
    
    
    // 线程队列相关的变量。
    std::unordered_map<ulong_, std::unique_ptr<Thread> > threadQue_ ; 
    // 任务队列相关的变量
    std::queue<std::shared_ptr<Task> > taskQue_ ; 

} ; 

#endif 