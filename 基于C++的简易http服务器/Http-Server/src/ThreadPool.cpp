#include "ThreadPool.h"
#include "public.h" 
#include "Thread.h" 
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

void ThreadPool::ThreadFunc( ulong_ threadId ) 
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	std::shared_ptr<Task> task;
	for (; ; )  // 死循环进行任务的消费
	{
		// 加上一块作用域可以实现多个线程同时执行各自获得的任务
		{
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			// std::cout << "tid: " << std::this_thread::get_id() << "尝试获取任务" << std::endl;
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
			// std::cout << "获取任务成功" << std::endl;
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
	 
	std::unique_lock<std::mutex> lock(taskQueMtx_) ;
	exitCond_.wait(lock, [this]() -> bool {return taskCnt_ == 0; } ) ; 
	PoolIsRuning_ = false ; 
	// 等待所有的线程结束。
	while (threadCnt_ > 0)
	{
		notEmpty_.notify_all() ; 
		exitCond_.wait(lock);  // 等待每个线程被回收之后的一个notify_all() 从而当最后一个线程被结束的时候此处能够继续执行。
	}
  
   
}

bool ThreadPool::checkRuningStatus() const 
{
	return PoolIsRuning_ ;  
}


// Thread 的接口函数 




 
              
Task::Task()
	: result_(nullptr)
{}

void Task::setResult(Result* result)
{
	result_ = result;
}
void Task::exec()
{
	if (result_ != nullptr) result_->setValue(HandlerRequest());
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