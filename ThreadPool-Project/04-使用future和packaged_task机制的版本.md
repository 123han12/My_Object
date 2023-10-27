`test.cpp`

```c++
#include "ThreadPool.h"
#include <iostream> 


ulong run(int begin , int end )  {
    ulong sum = 0;
    for (int i = begin ; i <= end; ++i)
    {
        sum += i;
    }
    std::cout << "任务执行" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return sum ; 
}

int sum2(int a, int b, int c)
{
    std::this_thread::sleep_for(std::chrono::seconds(10))  ;
    return a + b + c;
}

int main()
{
    
    ThreadPool pool;
   // pool.setMode(PoolMode::MODE_CACHED) ;  
    pool.setInitThreadSize(2) ;
    pool.start();

    std::future<ulong> res = pool.submitTask(run, 1, 100) ;
    std::future<int> res2 = pool.submitTask(sum2, 100, 1000, 10000);
    std::future<int> res3 = pool.submitTask(sum2, 100, 1000, 10000);
    std::future<int> res4 = pool.submitTask(sum2, 100, 1000, 10000);
    std::future<int> res5 = pool.submitTask(sum2, 100, 1000, 10000);  
 

    std::cout << res.get() << std::endl;
    std::cout << res2.get() << std::endl;
    std::cout << res3.get() << std::endl;
    std::cout << res4.get() << std::endl ; 
    std::cout << res5.get() << std::endl ;  
    std::cout << "main over" << std::endl ; 

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
#include <unordered_map>
#include <future>

 
const int INIT_THREAD_SIZE = std::thread::hardware_concurrency(); // 初始的线程的数量是CPU核心的数量  
const int MAX_THREAD_HOLD = 20;
const int MAX_TASK_HOLD = 2 ;
const int THREAD_MAX_IDLE_TIME = 60;
using ulong = unsigned long long;
// 线程池支持的模式
enum class  PoolMode {
    MODE_FIXED,  // 固定数量
    MODE_CACHED, // 动态增长
};
class Thread {

public:
    using Func = std::function<void(int)>;
    Thread(Func func)
	: threadFunc(func),
	  threadId(generateId++) // 这里的线程的id应该是之后再进行增加  
	{}
	
	~Thread()
	{}
	
	void start()
	{
		std::thread t(threadFunc, threadId);
		t.detach();
	}

	int getId() const
	{
		return threadId;
	}
private:
    Func threadFunc;
    static ulong generateId;  // 注意ulong 可以天然的溢出
    ulong threadId ; 
};
// 线程池类型

ulong Thread::generateId = 0 ;			
class ThreadPool
{
public:
    ThreadPool() 
		: PoolIsRuning_(false),
		poolMode_(PoolMode::MODE_FIXED),
		initThreadSize_(INIT_THREAD_SIZE),
		curIdleThread_(0),
		threadCnt_(0),
		taskCnt_(0),
		maxThreadHold_(MAX_THREAD_HOLD),
		maxTaskHold_(MAX_TASK_HOLD)
	{}

    // 为用户提供接口设置线程池的一些属性
	void setMode(PoolMode poolmode)
	{
		if (checkRuningStatus()) return;
		poolMode_ = poolmode;
	}
	void setInitThreadSize(int size)// 设置初始线程量
	{
		if (checkRuningStatus()) return;
		initThreadSize_ = size;
	}
	void setTaskQueMaxThreshHold(int threshhold) // 设置最大任务量
	{
		if (checkRuningStatus()) return;
		maxTaskHold_ = threshhold;
	}
	void setThreadQueMaxThreshHold(int threshhold)  // 设置最大线程量
	{
		if (checkRuningStatus()) return;
		if (poolMode_ == PoolMode::MODE_CACHED) maxThreadHold_ = threshhold;
	}

    // 启动线程池
	void start()  // 在start 之后 , set 无法再进行设置了
	{
		PoolIsRuning_ = true;


		for (int i = 0; i < initThreadSize_; ++i)
		{
			// make_unique 是在C++14当中的方法 ， make_shared 是在C++11当中的方法
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, std::placeholders::_1));
			ulong id = ptr->getId();
			threadQue_.emplace(id, std::move(ptr));
			threadCnt_++;
		}

		// 注意这里遍历unordered_map 的方式
		for (int i = 0; i < initThreadSize_; ++i)
		{
			threadQue_[i]->start();
			curIdleThread_++;
		}
	}

	template<typename Func , typename...Args > 
	auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...) )>
	{
		// RType 现在代表返回值类型。
		using RType = decltype(func(args...)) ;
		auto task = std::make_shared<std::packaged_task<RType()> >(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...) ) ; 

		std::future<RType> result = task->get_future() ;   // 返回与承诺的结果关联的std::future  

		std::unique_lock<std::mutex> lock(taskQueMtx_);

		while (taskCnt_ == maxTaskHold_)
		{
			if (std::cv_status::timeout == notFull_.wait_for(lock, std::chrono::seconds(1)))
			{
				std::cerr << "线程id为: " << std::this_thread::get_id() << "的线程任务提交失败，请稍后再试...." << std::endl;

				// 注意这个task 的类型的写法
				
				auto task = std::make_shared<std::packaged_task<RType() > >([]()->RType {return RType(); }) ; 
				(*task)() ;  
				return task->get_future() ; 
			}
		}

		taskQue_.emplace([task]() { (*task)() ;  }); 

		taskCnt_++;

		notEmpty_.notify_all();

		if (poolMode_ == PoolMode::MODE_CACHED && taskCnt_ > curIdleThread_ && threadCnt_ < maxThreadHold_)
		{

			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, std::placeholders::_1));
			ulong id = ptr->getId();
			threadQue_.emplace(id, std::move(ptr));
			threadQue_[id]->start();

			curIdleThread_++;
			threadCnt_++;

		}
		return result ;
		
	}
	~ThreadPool()
	{

		std::unique_lock<std::mutex> lock(taskQueMtx_);
		exitCond_.wait(lock, [this]() -> bool {return taskCnt_ == 0; });
		PoolIsRuning_ = false;
		while (threadCnt_ > 0)
		{
			notEmpty_.notify_all();
			exitCond_.wait(lock);  
		}
	}
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
	void ThreadFunc(ulong threadId)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();

		Task task ; 
		for (; ; )  // 死循环进行任务的消费
		{
			{
				std::unique_lock<std::mutex> lock(taskQueMtx_);
				std::cout << "tid: " << std::this_thread::get_id() << "尝试获取任务" << std::endl;
				// 判断是否需要回收当前线程
				while (taskCnt_ == 0)
				{
					if (!PoolIsRuning_)
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
				task = taskQue_.front() ; 
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
				task() ;  // 执行任务。 
			}
			// 更新线程开始空闲的时间戳
			lastTime = std::chrono::high_resolution_clock().now();
			curIdleThread_++; // 执行完毕之后，线程的数量进行增加
			exitCond_.notify_all() ;

		}
	}
	bool checkRuningStatus() const
	{
		return PoolIsRuning_;
	}
private:
    // 池子的属性
	using Task = std::function<void() > ;  
    
	std::atomic_bool PoolIsRuning_;

    size_t  initThreadSize_; // 初始线程数量 
    std::atomic_int curIdleThread_;  //空闲的线程的个数 
    std::atomic_int threadCnt_; // 当前的线程的数量
    std::atomic_int taskCnt_; // 当前的任务的数量 


    size_t  maxThreadHold_;  // 最大线程值
    size_t  maxTaskHold_;  // 最大任务值

    PoolMode poolMode_;


    std::mutex taskQueMtx_;

    std::condition_variable notEmpty_;  // 通知消费者线程消费
    std::condition_variable notFull_;   // 通知生产者线程生产
    std::condition_variable exitCond_; //  等待线程资源全部回收


    // 线程队列相关的变量。

    std::unordered_map<ulong, std::unique_ptr<Thread> > threadQue_;


    // 任务队列相关的变量
    
	std::queue<Task> taskQue_;
};

```

