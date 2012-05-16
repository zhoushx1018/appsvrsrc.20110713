#ifndef TIMER_H
#define TIMER_H 

#include <pthread.h>
#include "OurDef.h"


typedef void (*CALLBACK_FUN) ( void * obj, UInt32 arg );

class Timer
{
public:	
	Timer(unsigned int input);
	virtual ~Timer();
	void CountDown(long second);
	void AddSec(long second);
	 	
	void SetCallbackFun( CALLBACK_FUN fun, void * obj,  UInt32 arg );

	long GetTimeElapse();
	long GetCountDownSec();
	
private:
	void ThreadProcess();
	void HandleTimeout();
	static void * thread_main(void *p)
	{
		(static_cast<Timer*>(p))->ThreadProcess();
	}
	
private:
	//子线程ID
	pthread_t _threadId;
	
	//线程锁
	pthread_mutex_t	_mutexSleep;
	//条件变量
	pthread_cond_t _condSleep;
	
	//线程锁
	pthread_mutex_t	_mutex;
	
	//条件变量
	pthread_cond_t _cond;

	//回调参数1
	CALLBACK_FUN _fun;
	
	//回调参数2
	void * _obj;
	
	//回调参数3
	UInt32 _roleID;
	char _userID[32];

	int _stop;
    
private:
	//定时起始时间
	long _initTime;
	
 	//定时器类型 0 时间到后自我销毁Timer对象    1 时间到后不自我摧毁的对象
	unsigned int _type;
	
	//倒计时的秒数
	long _secCountDown;
    
	

};

#endif
