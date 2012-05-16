// 帐号类
#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "OurDef.h"



class Account
{

public:
	Account();
	Account(UInt32 li,UInt32 li2,Byte li3,UInt32 li4);
	~Account();


public:
	
	UInt32 AccountLastloginTime();
	UInt32 AccountToploginTime();
	UInt32 LoginTime();
	Byte IsAdult();
	
	
	//属性设置
	void IsAdult(Byte input);

	
private:
	
	UInt32 _accountlastloginTime;  //帐号上次登入时间
	UInt32 _accounttopTime;        //帐号登入上限时间，防成迷的最多3小时
	Byte _isAdult;//是否是成人
	UInt32 _logintime;

	
};


#endif

