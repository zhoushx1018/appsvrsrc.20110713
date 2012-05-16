#include "TimerManager.h"
#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "DebugData.h"
#include <string.h>
#include "Log.h"
#include "NewTimer.h"
#include <algorithm>

using namespace std;


bool TimerPool::init(int timerCount)
{
	NewTimer* timers = new NewTimer[timerCount];

	for (int i = 0; i < timerCount; i++)
	{
		_freeTimerList.push_back(&timers[i]);
	}

	return true;
}

TimerPool::~TimerPool()
{
	std::list<NewTimer*>::iterator timerItor = _freeTimerList.begin();
	for (; timerItor != _freeTimerList.end(); ++timerItor)
	{
		delete *timerItor;
	}
	_freeTimerList.clear();
	
	timerItor = _usedTimerList.begin();
	for (; timerItor != _usedTimerList.end(); ++timerItor)
	{
		delete *timerItor;
	}
	_usedTimerList.clear();
}

NewTimer* TimerPool::newTimer()
{
	MutexLockGuard lock(timerMutex_);
	if (!_freeTimerList.empty())
	{
		NewTimer* pTimer = _freeTimerList.front();
		_usedTimerList.push_back(pTimer);
		_freeTimerList.pop_front();
		return pTimer;
	}
	//todo:定时器不够用的时候应扩充定时器
	return 0;
}

void TimerPool::delTimer(NewTimer* timer)
{
	MutexLockGuard lock(timerMutex_);
	std::list<NewTimer*>::iterator iter = _usedTimerList.begin();
	
	if (std::find(_usedTimerList.begin(), _usedTimerList.end(), timer) !=_usedTimerList.end())
	{
		_usedTimerList.remove(timer);
		_freeTimerList.push_back(timer);
	}
}


int Slot::AddTimer(NewTimer* timer)
{
	MutexLockGuard lock(mutex_);
	_mapTimer.insert( make_pair(timer->ID(), timer) );
	return 1;
}

bool Slot::DelTimer(NewTimer* timer)
{
	MutexLockGuard lock(mutex_);
	map<UInt32, NewTimer*>::iterator iter = _mapTimer.find(timer->ID());
	if (iter != _mapTimer.end())
	{
		_mapTimer.erase(iter);
		return true;
	}
	return false;
}

//@brief	时间脉冲事件处理
//@return	已超时的循环定时器列表
list<NewTimer*> Slot::OnTick()
{
	MutexLockGuard lock(mutex_);
	int iTimeout = 0;
	list<NewTimer*>	lTimeoutTimerAll;				//所有已超时的定时器
	list<NewTimer*> lTimeoutTimerLoop;			//已超时的循环定时器
	map<UInt32,NewTimer*>::iterator itMap;

	//超时处理
	for( itMap = _mapTimer.begin(); itMap != _mapTimer.end(); itMap++ )
	{
		//超时定时器的处理
		NewTimer* pTimer = itMap->second;
		if (pTimer)
		{
			iTimeout = pTimer->OnTick();
			if(iTimeout)
			{
				//超时的定时器
				lTimeoutTimerAll.push_back(pTimer);

				//超时的循环定时器
				if(NewTimer::LoopTimer == pTimer->Type())
				{
					lTimeoutTimerLoop.push_back(pTimer);
				}
			}
		}
	}

	//清理所有超时的定时器
	std::list<NewTimer*>::iterator itList;
	for( itList = lTimeoutTimerAll.begin(); itList != lTimeoutTimerAll.end(); itList++ )
	{
		itMap = _mapTimer.find((*itList)->ID());
		if( itMap != _mapTimer.end() )
		{
			_mapTimer.erase(itMap);
		}
	}

	//返回循环定时器列表
	return lTimeoutTimerLoop;	
}

TimerManager::TimerManager():
_lastTimerID(0),
_isStop(0),
_currSlot(0)
{
	pthread_t tickThreadID;
	if( pthread_create(&tickThreadID,NULL, tick_thread_main,this))
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "create threads occurr error.");
	}

}

TimerManager::~TimerManager()
{
}

void* TimerManager::tick_thread_main(void * p)
{
	TimerManager * tm = static_cast<TimerManager*>(p);
	tm->TickThreadProcess();

	return NULL;
}

//@brief	时间脉冲的发起
void TimerManager::TickThreadProcess()
{
	struct timeval tv;

	while(0 == _isStop)
	{
		tv.tv_sec = TIMERTIMESLOT;
		tv.tv_usec = 0;
		select(0,NULL,NULL,NULL, &tv);

//DEBUG_PRINTF1( "TimerManager::TickThreadProcess().... .... _currSlot[%d] ", _currSlot );
		OnTick();
	}
}

//@brief	TimerManager 是否停止时间脉冲
Byte TimerManager::IsStop()
{
	return _isStop;
}


//@brief	停止 TimerManager 时间脉冲
void TimerManager::Stop()
{
	_isStop = 1;
}

//@brief	时间脉冲事件处理
//	currSlot++
//	触发当前 Slot 的Tick事件
void TimerManager::OnTick()
{
	//时间轮槽的时间脉冲事件处理
	list<NewTimer*> lRetsetTimer;
	lRetsetTimer = _slots[_currSlot].OnTick();

	//所有超时的循环定时器,挂到新的时间轮槽
	list<NewTimer*>::iterator it;
	for( it = lRetsetTimer.begin(); it != lRetsetTimer.end(); it++ )
	{
		AttachTimer( *it, 0 );
	}

	//时针指向下一个时间轮槽
	//	不允许越界 MAXTIMERNUM
	if( ++_currSlot >= MAXTIMERNUM )
		_currSlot = 0;
	
}


//@brief	添加定时器
//@return 0 成功  非0  失败
int TimerManager::AddTimer(NewTimer* timer, int isNewTimer)
{
	if (timer == 0)
		return -1;
	//定时器个数上限校验
	if( _lastTimerID >= MAXTIMERNUM )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "max timer num limit!!!!!!!");
		return -1;
	}
	
	//定时间隔校验
	if(timer->_interval <= 0)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "error param!, timer._interval[%d]", timer->_interval );
		return -1;
	}

	//挂接定时器
	AttachTimer(timer,isNewTimer);
	
	return 0;
}

int TimerManager::DelTimer(NewTimer* timer)
{
	for (int i = 0; i< MAXSLOTNUM; i++)
	{
		if (_slots[i].DelTimer(timer))
			break;
	}
	return 1;
}


//@brief	挂接定时器到指定slot
//@param	timer	定时器
//@param	isnewTimer	是否新定时器 0 否   1 是
void TimerManager::AttachTimer(NewTimer* timer, int isNewTimer )
{
	//定时器ID
	//	新定时器,需要设置 ID
	//	否则使用 旧的 ID
	if (timer == 0)
		return;
	if(isNewTimer)
		timer->_ID = ++_lastTimerID;
		
//	timer->_maxTimerNum = MAXTIMERNUM;				//最大定时器个数
//	timer->_timeSlot = TIMERTIMESLOT;				//时间片时长 单位 秒
	timer->_ticks = timer->_interval/TIMERTIMESLOT;			//时间片数
	timer->_round = timer->_ticks/MAXSLOTNUM;			//时间轮圈数
	int slotNum = (_currSlot + (timer->_ticks%MAXSLOTNUM))%MAXSLOTNUM;		//时间轮槽号
	timer->_tm = this;
/*
DEBUG_PRINTF( "-----------------------" );
DEBUG_PRINTF1( "timer._ID[%d]", timer._ID );
DEBUG_PRINTF1( "timer._type[%d]", timer._type);
DEBUG_PRINTF1( "timer._maxTimerNum[%d]", timer._maxTimerNum );
DEBUG_PRINTF1( "timer._timeSlot[%d]", timer._timeSlot );
DEBUG_PRINTF1( "timer._ticks[%d]", timer._ticks );
DEBUG_PRINTF1( "timer._round[%d]", timer._round );
DEBUG_PRINTF1( "_currSlot[%d]", _currSlot );
DEBUG_PRINTF1( "timer._slotNum[%d]", timer._slotNum );
DEBUG_PRINTF1( "timer._callBack[%d]", timer._callBack);
DEBUG_PRINTF1( "timer._obj[%d]", timer._obj);
DEBUG_PRINTF1( "timer._arg[%d]", timer._arg);
DEBUG_PRINTF1( "timer._argLen[%d]", timer._argLen);
DEBUG_PRINTF( "========================" );
*/
//LOG(LOG_ERROR,__FILE__,__LINE__,"timer: ID[%d],slotNum[%d]", timer._ID, timer._slotNum);

	//时间轮槽添加定时器
	_slots[slotNum].AddTimer(timer);
}

