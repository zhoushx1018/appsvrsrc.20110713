#include "Timer.h"
#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "DebugData.h"
#include <string.h>

using namespace std;

Timer::Timer( unsigned int input ):
    _secCountDown(0),
    _initTime(0)
{
	_type = input;
	
	pthread_mutex_init( &_mutex, NULL );
	pthread_mutex_init( &_mutexSleep, NULL );
	
	pthread_cond_init( &_cond, NULL );
	pthread_cond_init( &_condSleep, NULL );
	
	
	pthread_create(&_threadId, NULL, thread_main, this);
	
	//回调参数
	_fun = NULL;
	_obj = NULL;
	_roleID = 0;

	_stop = 0;

}


Timer::~Timer()
{
	_stop = 1;

	pthread_mutex_destroy( &_mutex );
	pthread_mutex_destroy( &_mutexSleep );
	
	pthread_cond_destroy( &_cond );
	pthread_cond_destroy( &_condSleep );
	
} 

 void Timer::CountDown(long second )
{
	pthread_mutex_lock( &_mutex );
	_secCountDown = second;
	
//DEBUG_PRINTF2( "threadid[%ld]Timer  CountDown sec[%ld] ", pthread_self(), _secCountDown );

	_initTime = time(NULL);
	pthread_mutex_unlock( &_mutex );
	pthread_cond_signal( &_cond );
	
	pthread_cond_signal( &_condSleep );
	
}

long Timer::GetTimeElapse()
{
	return time(NULL) - _initTime;
}

long Timer::GetCountDownSec()
{
	return _secCountDown;
}



void Timer::AddSec(long second)
{
	pthread_mutex_lock( &_mutex );
	_secCountDown += second;

DEBUG_PRINTF2( "threadid[%ld]Timer  AddSec sec[%ld] ", pthread_self(), second );
	pthread_mutex_unlock( &_mutex );
	pthread_cond_signal( &_cond );
}

void Timer::ThreadProcess()
{
	while ( 0 == _stop )
	{
		//等待倒计时设置 Timer::CountDown()
		pthread_mutex_lock( &_mutex );
		while( 0 == _secCountDown )
			pthread_cond_wait( &_cond, &_mutex );
		pthread_mutex_unlock( &_mutex );
		
		//线程睡眠 _secCountDown 秒, 中途允许 Timer::CountDown() 唤起睡眠
		struct timespec tempspec;
		tempspec.tv_sec = time(NULL) + _secCountDown;
		tempspec.tv_nsec = 0;
		
//DEBUG_PRINTF1( "before sleep---->  _secCountDown[%d] ", _secCountDown );
		pthread_mutex_lock( &_mutexSleep );
		pthread_cond_timedwait( &_condSleep, &_mutexSleep, &tempspec );
		pthread_mutex_unlock( &_mutexSleep );
//DEBUG_PRINTF1( "after sleep========>elapse[%d] ", time(NULL) - _initTime );		
		
		//流逝时间记录
		pthread_mutex_lock( &_mutex );		
		_secCountDown -= time(NULL) - _initTime;
		if( _secCountDown <= 0 )
			_secCountDown = 0;		
		pthread_mutex_unlock( &_mutex );
		
		//时间事件处理
		if( 0 == _secCountDown )
			HandleTimeout();
	}
} 

void Timer::HandleTimeout()
{

DEBUG_PRINTF1( "HandleTimeout.........elapse[%d]", time(NULL) - _initTime );
	
	if( NULL != _fun )
		(*_fun)( _obj, _roleID );
	else
 		DEBUG_PRINTF( "_fun is NULL, timer HandleTimeout() do nothing ............" );

	if( 0 == _type )
		delete this;

DEBUG_PRINTF4( "_type [%d], _fun[%ld], _obj[%ld], _roleID[%d] ", _type, _fun, _obj, _roleID);
	
} 


void Timer::SetCallbackFun( CALLBACK_FUN fun, void * obj,  UInt32 arg )
{
	_fun = fun;
	_obj = obj;
	_roleID = arg;

	DEBUG_PRINTF4( "_type [%d], _fun[%ld], _obj[%ld], _roleID[%d] ", _type, _fun, _obj, _roleID);
}


