#include "Account.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Log.h"


Account::Account()
:_accountlastloginTime(0),
_accounttopTime(0),
_isAdult(0)
{
	
}

Account::Account(UInt32 li,UInt32 li2,Byte li3,UInt32 li4)
{
		_accountlastloginTime=li;
		_accounttopTime=li2;
		_isAdult=li3;
		_logintime=li4;
}
Account::~Account()
{
	
}

UInt32 Account::AccountLastloginTime()
{
	return _accountlastloginTime;
}
UInt32 Account::AccountToploginTime()
{
	return _accounttopTime;
}
Byte Account::IsAdult()
{
	return _isAdult;
}
UInt32 Account::LoginTime()
{
	return _logintime;
}

// Ù–‘…Ë÷√
void Account::IsAdult(Byte input)
{
		_isAdult=input;
}


