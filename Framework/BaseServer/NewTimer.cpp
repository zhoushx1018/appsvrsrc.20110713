#include "NewTimer.h"
#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "DebugData.h"
#include <string.h>
#include "Log.h"
#include "TimerManager.h"

using namespace std;

SelfDeleteCallback::SelfDeleteCallback(TIMER_CALLBACK_FUN callBack, void * obj, void* arg, int argLen ):
_callBack(callBack),
_obj(obj),
_argLen(argLen)
{
	memcpy( _arg, arg, _argLen );
}
SelfDeleteCallback::~SelfDeleteCallback()
{
}

void SelfDeleteCallback::NewThreadCallBack()
{
	pthread_t	threadID;
	int iRet = 0;
	
	//重试几次,确保线程生成
	for(int i = 0; i<5; i++)
	{
		usleep(1000);
		iRet = pthread_create(&threadID, NULL, detach_thread_main, this);
		if(0==iRet)
			break;
	}
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"SelfDeleteCallback pthread_create error!!") ;
	}

	//分离线程
	iRet = pthread_detach(threadID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"SelfDeleteCallback pthread_detach error!!") ;
	}
}

void * SelfDeleteCallback::detach_thread_main(void * p)
{
	(static_cast<SelfDeleteCallback*>(p))->DetachProcess();
}

//@brief	分离线程的处理逻辑
//				执行回调函数
void SelfDeleteCallback::DetachProcess()
{
//	DEBUG_PRINTF4( "..............DetachProcess........._callBack[%d], _obj[%d], _arg[%d], _argLen[%d] ",
//		_callBack, _obj, _arg, _argLen );
	
	
	if( NULL != _callBack && NULL != _obj )
		(*_callBack)( _obj, _arg, _argLen );

	//处理完成后,自我 delete
	delete this;
}



NewTimer::NewTimer():
_callBack(NULL),
_obj(NULL),
_argLen(0),
_ID(0),
//_maxTimerNum(0),
//_timeSlot(0),
_interval(0),
_ticks(0),
_round(0),
//_slotNum(0),
_type(OnceTimer),
_tm(0)
{	
}

NewTimer::~NewTimer()
{
}

//@brief	设置回调函数
//@param	fun	回调函数
//@param	obj	对象地址
//@param	arg	回调函数的参数地址
//@param	argLen	回调函数的参数长度
//@return	0 成功  非0 失败
int NewTimer::SetCallbackFun( TIMER_CALLBACK_FUN fun, void * obj,  void * arg, int argLen )
{
	_callBack = fun;
 	_obj = obj;
 	
 	//回调函数及其参数
	if( argLen < 0 || argLen > MAXARGLEN )
	{
		return -1;
	}
	else
	{
		_argLen = argLen;
		memcpy( _arg, arg, _argLen );
	}

	return 0;
}

void NewTimer::Interval(int input)
{
	_interval = input;
}

UInt32 NewTimer::ID()
{
	return _ID;
}

NewTimer::TimerType NewTimer::Type()
{
	return _type;
}

void NewTimer::Type(TimerType input)
{
	_type = input;
}

bool NewTimer::cancel()
{
	//todo:如果正在处理处理是否可以cancel?
	if (_tm)
	{
		bool bCancel = _tm->DelTimer(this);
		if (bCancel)
		{
			_tm = 0;
		}
		
		return bCancel;
	}
	return true;
}

bool NewTimer::start(TimerManager* tm)
{
	if (tm)
	{
		int bNewTimer = (ID() == 0);
		tm->AddTimer(this, bNewTimer);
	}
}

bool NewTimer::reStart()
{
	if (_tm)
	{
		bool bCancel = _tm->DelTimer(this);
		if (bCancel)
		{
			return _tm->AddTimer(this, 0);
		}
	}
	return false;
}


//@brief	时针事件处理
//@return 定时器是否超时	0 否	非0 已超时
int NewTimer::OnTick()
{
	int iTimeout = 0;

	//时间轮圈数 为0 触发超时处理,
	if(--_round <= 0)
	{
		iTimeout = 1;
		OnTimeout();
	}
	
	return iTimeout;
}


//@brief	定时器超时处理
void NewTimer::OnTimeout()
{
	SelfDeleteCallback * callBack = new SelfDeleteCallback(_callBack, _obj, _arg, _argLen);
	callBack->NewThreadCallBack();
	_tm = 0;
}


