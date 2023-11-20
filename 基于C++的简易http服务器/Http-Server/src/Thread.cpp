#include "Thread.h" 
#include "public.h" 
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
ulong_ Thread::generateId = 0 ; 

ulong_ Thread::getId() const  // 获取当前Thread对象的id 
{
	return threadId ; 
}
