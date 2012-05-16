#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H 
//	基于时间轮算法的定时器
//	算法描述详见 <多线程应用中的定时器管理算法>

#include <map>
#include <list>
#include "Mutex.h"
#include "OurDef.h"

#define	MAXTIMERNUM				3000						//最大定时器个数
#define MAXSLOTNUM				3000						//最大槽个数
#define TIMERTIMESLOT			1							//时间片时长	1 秒

using namespace std;

class NewTimer;

class TimerPool
{
public:
	~TimerPool();
	bool			init(int timerCount = MAXTIMERNUM);
	NewTimer*		newTimer();
	void 			delTimer(NewTimer* timer); //这个接口暂时没用到，也就是说timer一直占用
private:
	std::list<NewTimer*>    _freeTimerList; //Guarded by timerMutex_
	std::list<NewTimer*>    _usedTimerList;//Guarded by timerMutex_
	mutable MutexLock 		timerMutex_;
};


//时间轮的 轮槽
class Slot
{
public:
	int	AddTimer(NewTimer* timer);
	bool DelTimer(NewTimer* timer);
	list<NewTimer*> OnTick();

private:
	//线程锁
	mutable MutexLock mutex_;

	//定时器容器
	map<UInt32, NewTimer*> _mapTimer; //Guarded by mutex_
};

//定时器管理类
class TimerManager
{
public:
	TimerManager();
	~TimerManager();
	static void * tick_thread_main(void *);
	void TickThreadProcess();
	Byte IsStop();
	void Stop();
	void OnTick();
	int AddTimer(NewTimer* timer, int isNewTimer = 1);
	int DelTimer(NewTimer* timer);

private:	
	void AttachTimer( NewTimer* timer, int isNewTimer );

private:
	//时间轮数组
	Slot _slots[MAXSLOTNUM];
	
	//记录最后一个 timer的ID
	//		取值范围 1 ~ MAXTIMERNUM
	UInt32	_lastTimerID;

	//定时脉冲停止标志
	//	0 未停止  1 停止
	Byte	_isStop;

	//当前时间片指针
	//	取值范围	0 ~ (MAXTIMERNUM-1)
	UInt32	_currSlot;

	

};

#endif
