`test.cpp`

```c++
#include "ThreadPool.h"
#include <iostream> 

class myTask : public Task
{
public:
    myTask(int begin, int end)
        : begin_(begin) , end_(end ) 
    {}
    // 使用类似于C++17中的Any类型进行返回值的标识
     
    Any run() override {
        ulong sum = 0; 
        for (int i = begin_; i <= end_; ++i)
        {
            sum += i ;  
        }
        std::cout << "任务执行" << std::endl ;  
        std::this_thread::sleep_for(std::chrono::seconds(3) ) ;   
        return Any(sum) ;   
    }
private: 
    int begin_ ; 
    int end_ ; 
}; 

int main()
{

    // 如果当ThreadPool 在析构的时候，任务队列中仍然存在任务，是等线程将任务执行完毕再析构还是直接析构？
    {
        ThreadPool pool;
        pool.setMode(PoolMode::MODE_CACHED);
        //  pool.setMode(PoolMode::MODE_FIXED) ;  
        pool.setInitThreadSize(2);
        pool.start() ; 
        Result res;
        Result res2;
        Result res3;
        Result res4;
        pool.submitTask(std::make_shared<myTask>(1, 10000), &res);
        pool.submitTask(std::make_shared<myTask>(10001, 20000), &res2) ;
        pool.submitTask(std::make_shared<myTask>(10001, 20000), &res3) ;
        pool.submitTask(std::make_shared<myTask>(10001, 20000), &res4) ;
      
    }  // 在回收pool的时候，必须等到线程中的任务全部都执行完毕之后才进行回收   

    // getchar()  ; 
    std::cout << "main over" << std::endl ; 

#if 0 
    // 这块作用域中的pool如何进行一个回收
    {
        ThreadPool pool;

        // 设置属性
        pool.setInitThreadSize(4) ; 
        pool.setTaskQueMaxThreshHold(5);
        pool.setThreadQueMaxThreshHold(10);
        pool.setMode(PoolMode::MODE_CAAHED);

        // 启动线程池
        pool.start();

        // 提交任务  
        Result res;   //  task的生命周期延迟到Result 对象被销毁的时候。
        Result res2;
        Result res3;
        Result res4;
        Result res5;
        Result res6;
        Result res7;
        Result res8;
        Result res9;
        pool.submitTask(std::make_shared<myTask>(1, 10000), &res);
        pool.submitTask(std::make_shared<myTask>(10001, 20000), &res2);
        pool.submitTask(std::make_shared<myTask>(10001, 20000), &res3);
        //pool.submitTask(std::make_shared<myTask>(10001, 20000), &res4);
        //pool.submitTask(std::make_shared<myTask>(10001, 20000), &res5);
        //pool.submitTask(std::make_shared<myTask>(10001, 20000), &res6);
        //pool.submitTask(std::make_shared<myTask>(10001, 20000), &res7);
        //pool.submitTask(std::make_shared<myTask>(10001, 20000), &res8);
        //pool.submitTask(std::make_shared<myTask>(10001, 20000), &res9);


        std::cout << "数据处理中......" << std::endl ; 
        ulong  sum1 = res.get().cast_<ulong>(); // 任务未执行完的话，就会在这里进行阻塞
        ulong  sum2 = res2.get().cast_<ulong>(); // 任务未执行完的话，就会在这里进行阻塞
        ulong  sum3 = res3.get().cast_<ulong>(); // 任务未执行完的话，就会在这里进行阻塞


        std::cout << (sum1 + sum2 + sum3) << std::endl;
    }
    
    // 注意，这里主线程不能死的太快，如果死的太快的话，会造成程序的终止

    // int是已知的类型
    getchar();   

#endif 

    return 0;
}
```



`ThreadPool.h`

```c++
#pragma once
// 线程类型
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
using ulong = unsigned long long ;  

// 这个信号量的实现和C++20中的 semaphore 的实现原理是一样的。    
class Semaphore {

public: 
    Semaphore(size_t reslimit = 0 ) : resLimit_(reslimit ) {} 
    ~Semaphore(){} 


    void wait()
    {
        std::unique_lock<std::mutex > lock(mtx_) ;
        cond_.wait(lock, [this]()-> bool {return resLimit_ > 0 ; } ) ;
        resLimit_ -- ; // 数量减少一下
    }

    void post()
    {
        std::unique_lock<std::mutex > lock(mtx_) ; 
        resLimit_++ ; 
        cond_.notify_all() ;  
    }


private: 
    size_t resLimit_ ; // 资源计数  
    std::mutex mtx_; 
    std::condition_variable cond_ ;  
};


class Any {
public: 

    // 模版构造函数 ， 可以接受任意类型的数据
    template<typename T> 
    Any(T data): base_( std::make_unique< Derive<T> > (data))
    {}

    Any() = default ;
    ~Any() = default ; 
    Any(const Any&) = delete ; 
    Any& operator=(const Any&) = delete ; 
    Any(Any&&) = default ; 
    Any& operator=(Any&&) = default ;  


    // 这个方法是将 Any类型中的数据提取出来
    template<typename T >
    T cast_()
    {
        // 怎么从base_中找到它所指向的派生类Drive对象，从其中提取出来数据。
                                                     
        // 基类指针转为派生类指针 , 支持RTTI 
        Derive<T>* pd = dynamic_cast< Derive<T>* >(base_.get()) ; 
       
        // std::cout << pd << std::endl ; 
        // getchar() ;  
        if (pd == nullptr ) // 为什么会走到这里呢？？
        {
            throw "type is unmatch" ;  
           
        }     // 当 cast 中的T 和 Derive 中的 T 不同的时候，会出现这种情况
        return pd->data_ ;  
    }

private:
    class Base {
    public:
        virtual ~Base() = default ; // 如果派生类对象是在堆上创建的，最好将析构函数设置为虚函数 
        
    } ; 

    // 派生类类型
    template<typename T> 
    class Derive : public Base
    {
    public :
        Derive(T data) : data_(data) {} 

        T data_ ;  
    };
    
private: 
    std::unique_ptr<Base> base_ ; // 定义一个基类指针。
};

// 接受返回值的Result 类的实现
class Task ;  
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
    virtual Any run() = 0;  // 提供虚拟接口 
    void setResult(Result* result); 
    void exec() ;  
private:
    Result* result_;
};


// 线程池支持的模式
enum class  PoolMode {
    MODE_FIXED,  // 固定数量
    MODE_CACHED, // 动态增长
};




class Thread {

public:
    using Func = std::function<void(int )> ;  
    Thread(Func func ) ; 
    ~Thread(); 
    void start() ;  

    int getId() const ;  
private:
    Func threadFunc ; 
    static ulong generateId ;  // 注意ulong 可以天然的溢出
    ulong threadId ;  
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
    void ThreadFunc(ulong threadId ) ; 
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

    std::unordered_map<ulong, std::unique_ptr<Thread> > threadQue_ ; 


    // 任务队列相关的变量
    std::queue<std::shared_ptr<Task> > taskQue_ ; 
    

} ; 
```

`ThreadPool.cpp`

```c++
#include "ThreadPool.h"
#include <iostream>
#include <functional> 
// ThreadPool 的接口函数
ThreadPool::ThreadPool()
	: PoolIsRuning_(false) , 
	poolMode_(PoolMode::MODE_FIXED) , 
	initThreadSize_(INIT_THREAD_SIZE) , 
	curIdleThread_(0) , 
	threadCnt_(0) , 
	taskCnt_(0) , 
	maxThreadHold_(MAX_THREAD_HOLD) , 
	maxTaskHold_(MAX_TASK_HOLD) 
{}

// 为用户提供接口设置线程池的一些属性
void ThreadPool::setInitThreadSize(int size) // 设置初始线程量
{

	if (checkRuningStatus()) return;
	initThreadSize_ = size ;
}
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)  // 设置最大任务量
{
	if (checkRuningStatus()) return;
	maxTaskHold_ = threshhold ;  
}
void ThreadPool::setThreadQueMaxThreshHold(int threshhold)  // 设置最大线程量
{
	if (checkRuningStatus()) return;
	if (poolMode_ == PoolMode::MODE_CACHED ) maxThreadHold_ = threshhold ; 
}

void ThreadPool::setMode(PoolMode poolmode) // 设置线程池的模式
{
	if (checkRuningStatus()) return ;
	poolMode_ = poolmode ; 
}

// 启动线程池
void ThreadPool::start() 
{
	PoolIsRuning_ = true ; 
   
	for (int i = 0; i < initThreadSize_; ++i ) 
	{
		// make_unique 是在C++14当中的方法 ， make_shared 是在C++11当中的方法
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this , std::placeholders::_1 ) ) ; 
		ulong id = ptr->getId() ;  
		threadQue_.emplace(id , std::move(ptr ) ) ;  
		threadCnt_++ ;  
	}
	 
	// 注意这里遍历unordered_map 的方式
	for (int i = 0; i < initThreadSize_; ++i)
	{
		threadQue_[i]->start() ;  
		curIdleThread_++ ;
	}
}

void ThreadPool::submitTask(std::shared_ptr<Task> myTask , Result* result )
{
	std::unique_lock<std::mutex> lock(taskQueMtx_) ; 
	
	// 如果任务队列是满的，我们就继续等待 , 暂时不考虑提交失败的情况
	// 只要 taskCnt_ >= maxTaskHold_ 就一直等待，直到 taskCnt_ < maxTaskHold_才继续执行


	
	//notFull_.wait(lock, [this]() -> bool {return taskCnt_ < maxTaskHold_ ;  } ) ;

	// 用户提交任务，最长不能阻塞1秒，否则判断任务提交失败，返回
	while (taskCnt_ ==  maxTaskHold_  ) 
	{
		if (std::cv_status::timeout == notFull_.wait_for(lock, std::chrono::seconds(1)))
		{
			std::cerr << "线程id为: " << std::this_thread::get_id() << "的线程任务提交失败，请稍后再试...." << std::endl ;  
			 
			new (result)Result(myTask, false) ;  
			return ;  
		}
	} 
	/*
	这个wait_for 1s 之后 ，如果对谓词pred 进行求值为 false 则为false ，否则为 true   
	if(notFull_.wait_for(lock , std::chrono::seconds(1) , [this]() -> bool {return taskCnt_ < maxTaskHold_ ; } ) ) ;

	*/

	// 注意这里能使用 emplace 的话就使用 emplace 
	taskQue_.emplace(myTask) ;  
	taskCnt_++ ; 

	// 通知消费者线程进行消费
	notEmpty_.notify_all() ; 

	// cached 模式适用于小而快的任务，如果是在cached 模式，任务数量比线程数量多，并且任务数量还没到达阈值，则添加线程
	if (poolMode_ == PoolMode::MODE_CACHED && taskCnt_ > curIdleThread_ && threadCnt_ < maxThreadHold_)
	{

		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this , std::placeholders::_1 ) ) ; 
		ulong id = ptr->getId();
		threadQue_.emplace(id, std::move(ptr)) ; 
		threadQue_[id]->start() ; 
		
		// 修改相关属性的个数
		curIdleThread_++ ;  
		threadCnt_++ ; 

	}

	new (result)Result(myTask) ; // 使用placement new 
	return ;  
}

void ThreadPool::ThreadFunc(ulong threadId ) 
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	std::shared_ptr<Task> task;
	for (; ; )  // 死循环进行任务的消费
	{
		// 加上一块作用域可以实现多个线程同时执行各自获得的任务
		{
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			std::cout << "tid: " << std::this_thread::get_id() << "尝试获取任务" << std::endl;
			// 判断是否需要回收当前线程
			while (taskCnt_ == 0)
			{
				if (!PoolIsRuning_ ) 
				{
					curIdleThread_--; // 空闲线程数量也被修改
					threadCnt_--;  // 线程数量被修改。 
					// 将线程对象从线程列表容器中删除 , 成功删除了线程列表
					threadQue_.erase(threadId);
					std::cout << "thread id: " << std::this_thread::get_id() << "线程池析构，线程结束" << std::endl;
					exitCond_.notify_all();
					return;
				}
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

						if (dur.count() >= THREAD_MAX_IDLE_TIME && threadCnt_ > initThreadSize_) // 如果空闲的时间大于等于60秒。
						{
							// 相关变量的值的修改
							curIdleThread_--; // 空闲线程数量也被修改
							threadCnt_--;  // 线程数量被修改。 
							// 将线程对象从线程列表容器中删除 , 成功删除了线程列表
							threadQue_.erase(threadId);
							std::cout << "thread id: " << std::this_thread::get_id() << "线程空闲时间太久，直接结束" << std::endl;
							return;
						}
					}
				}
				else { // fixed 模式
					// 如果线程池中没有任务的话，就一直阻塞当前线程，
					notEmpty_.wait(lock);
				}
			}
			std::cout << "获取任务成功" << std::endl;
			task = taskQue_.front();
			taskQue_.pop();
			taskCnt_--;  // 任务数量减少 
			curIdleThread_--;  // 空闲线程的数量减少 

			// 通知消费者消费任务
			if (taskCnt_ > 0) notEmpty_.notify_all();

			// 通知生产者提交任务
			notFull_.notify_all();
		} // 在执行任务的时候，将锁提前进行释放 ， 出了右括号，锁就被释放了

		if (task != nullptr)
		{
			task->exec();
		}
		// 更新线程开始空闲的时间戳
		lastTime = std::chrono::high_resolution_clock().now();
		curIdleThread_++; // 执行完毕之后，线程的数量进行增加
		// 每执行完一次任务就尝试通知一下exitCond_ 上的等待线程，从而保证在析构的时候不会出问题。
		exitCond_.notify_all();

	}//在出这个右括号的时候，当前执行的这个 task 对象就会被析构掉，所以需要采用 Result(task)  而不是task(Result) ,采用Result(task) 之后，task 的生命周期会延迟到 Result 对象结束的时候。

	
}

ThreadPool::~ThreadPool() 
{
	// ~ThreadPool() 的内容
	 
   PoolIsRuning_ = false;

   std::unique_lock<std::mutex> lock(taskQueMtx_);
   // 等待所有的线程结束。
   notEmpty_.notify_all() ; 

   exitCond_.wait(lock, [this]() -> bool { return threadCnt_ == 0 ; } ) ;  // 等待每个线程被回收之后的一个notify_all() 从而当最后一个线程被结束的时候此处能够继续执行

  
   
}

bool ThreadPool::checkRuningStatus() const 
{
	return PoolIsRuning_ ;  
}


// Thread 的接口函数 

Thread::Thread(Func func )
	: threadFunc(func ) , 
	threadId( generateId++ ) // 这里的线程的id应该是之后再进行增加  
{}

Thread::~Thread(){}

void Thread::start()
{
	std::thread t(threadFunc , threadId ) ;  
	t.detach() ;  
}
ulong Thread::generateId = 0 ; 
int Thread::getId() const  // 获取当前Thread对象的id 
{
	return threadId ; 
}
// Result 方法的实现
Result::Result(std::shared_ptr<Task> task, bool isvaild  )
	: task_(task), isVaild_(isvaild)
{
	task_->setResult(this) ; // 给任务存储我们的Result*  
}


void Result::setValue(Any any)  // 存储 task 的返回值
{
	any_ = std::move(any);
	sem_.post(); // 增加信号量资源 
}

// 用户调用此方法获取返回值
Any Result::get()
{
	if (!isVaild_) return "" ;  // 为什么这里返回空了 ? ? ? 

	sem_.wait() ;
	return std::move(any_) ;  
 } 
 
              
    //  Task 类接口
Task::Task()
	: result_(nullptr)
{}

void Task::setResult(Result* result)
{
	result_ = result;
}
void Task::exec()
{
	if (result_ != nullptr) result_->setValue(run());
}
```







