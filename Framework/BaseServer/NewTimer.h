#ifndef NEWTIMER_H
#define NEWTIMER_H 


#include <pthread.h>
#include "OurDef.h"


#define MAXARGLEN	128

typedef void (*TIMER_CALLBACK_FUN) ( void * obj, void * arg, int argLen );

class TimerManager;

//自我delete 的回调函数对象
class SelfDeleteCallback
{
public:
	friend class NewTimer;
	SelfDeleteCallback(TIMER_CALLBACK_FUN callBack, void * obj, void* arg, int argLen );
	~SelfDeleteCallback();
	
	void NewThreadCallBack();
	static void * detach_thread_main(void * p);
	void DetachProcess();
	

private:
	//---------------回调函数相关----------------------------
	//回调函数地址
	TIMER_CALLBACK_FUN	_callBack;
	
	//对象地址
	void * _obj;

	//回调函数的参数
	char _arg[MAXARGLEN];

	//回调函数的参数长度
	int _argLen;
};

class NewTimer
{
public:
	friend class TimerManager;
	
public:
	enum TimerType
	{
		LoopTimer,
		OnceTimer,
		TimeTypeNum,
	};
	
	NewTimer();
	virtual ~NewTimer();
	int SetCallbackFun( TIMER_CALLBACK_FUN fun, void * obj,  void * arg, int argLen );
	void Interval(int input);
	UInt32 ID();
	TimerType Type();
	void Type(TimerType input);
	int OnTick();
	void OnTimeout();
	bool cancel();
	bool start(TimerManager* tm);
	bool reStart();
	bool IsRunning() {return _tm != 0;}  //_tm不为空表示正在TimerManager进行倒计时
	void* GetCallbackFunParam() {return (void*)_arg;}
private:
	//---------------回调函数相关----------------------------
	//回调函数地址
	TIMER_CALLBACK_FUN	_callBack;
	
	//对象地址
	void * _obj;

	//回调函数的参数
	char _arg[MAXARGLEN];

	//回调函数的参数长度
	int _argLen;

   
private:
	//---------------定时器参数----------------------------
	
	//定时器类型 0 循环型(执行无限次回调)	  1 单次(执行一次回调)
	TimerType _type;

	//定时器 ID
	UInt32 _ID;
 	
	//最大 定时器数
//	int _maxTimerNum;

	//时间片 时长
//	int _timeSlot;
	
	//定时器 定时间隔
	int _interval;

	//时间片数
	//	定时间隔 所需时间片总数
	//	ticks = interval / timeslot
	int _ticks;

	//时间轮	圈数
	//	round = ticks / maxTimerNum
	//	round 为0 表示定时器时间超时,
	//	currentSlot 经过时 round--
	int _round;

	//时间轮槽号
	//	取值范围	0~ (_maxTimerNum-1)
	//	slotNum = currentSlot + (ticks % maxTimerNum)
//	int _slotNum;

	TimerManager* _tm;

};

#endif
