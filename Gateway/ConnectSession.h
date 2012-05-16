#ifndef CONNECTSESSION_H
#define CONNECTSESSION_H

#include "Mutex.h"

#include "System.h"

#define SESSION_TRYSENDNUM  20				//本session最大尝试发送次数

class ConnectSession
{
public:
	ConnectSession(long long llInputConId, char cInputConnType );
	~ConnectSession();
	
	//获取连接属性
	Byte GetConnType();
	long long GetConnId();
	u_int GetStamp();
	Byte GetLoginStatus();
	u_int GetRoleID();
	Byte GetIsSameUser();
	Byte GetSrvType();
	Byte GetSrvSeq();

	Byte GetCloseFlag();

	Byte GetClosedNum();
	
	int BuffInitCheck();
	char * GetIP();
	int GetPort();
	int GetFd();
	
	//设置连接属性
	void SetConnType( Byte ucInput );
	void SetConnId( long long llInput );
	void SetStamp();
	void SetLoginStatus( Byte ucInput );
	void SetRoleID( u_int input);
	void SetIsSameUser( Byte ucInput );
	void SetSrvType( Byte ucInput );
	void SetSrvSeq( Byte ucInput );
	void CloseConn();
	
	
	//接收数据
	int RecvMsg();
	
	//发送数据
	int SendMsg( const char *buffer, int buflen);
	
	//获取缓存数据包
	int GetDataPack( char * ptrData, int &iLen );
	
private:
	//检查缓存数据包
	int	CheckDataPack(char * ptrData, int &iLen);

	void CloseConnNoLock();
	
private:
	
	//数据保护，包括socket接收数据
	mutable MutexLock		mutexData_;
	
	//发送数据保护,  数据接收和发送分开保护, 以提高并发性能
//	mutable MutexLock		mutexSend_;

	//尝试发送次数
	//	尝试发送次数超过一定次数，则断开连接
	int _trySendNum;
	
private:
	
	//(内外网)连接类型 ： INSIDE_TCP_STREAM(内网 TCP 连接)    OUTSIDE_TCP_STREAM(外网 TCP 连接)
	Byte _connType;
	
	//(内外网)connect id 连接id值,  llConId = ip(4B)+ port(2B)+ fd(2B)
	//_connID 的 fd 一直保存不改变，用作信息保存;_fd 在关闭连接后置为 -1，用作连接关闭与否的判断
	long long _connID;
	
	//(内外网)接收数据缓存
	char* _recvBuff;
	
	//(内外网)接收数据缓存长度
	int _recvLen;
	
	//(内外网)时间戳,		记录session最近一次操作的时间
	u_int _stamp;
	
	//(内外网)登录状态	内网表示srv的登录，外网表示角色的登录			0：未登录  非0：已登录
	Byte _loginStatus;
	
	//(外网)角色ID 外网连接的key
	u_int _roleID;
	
	//(外网)是否同名角色登录, 外网连接使用
	Byte _isSameUser;
	
	//(内网)服务器类型, 内网连接 key1
	Byte _srvType;

	//(内网)服务器序号, 内网连接 key2
	Byte _srvSeq;
	
	//连接会话关闭标志，0 连接会话正常 1 连接会话即将关闭
	Byte _closeFlag;

	//被设置关闭的次数	0 零次;   1 一次;  2 二次及以上
	Byte _closedNum;
	

	//ip
	char _ip[16];

	//port
	int _port;

	//fd
	//_connID 的 fd 一直保存不改变，用作信息保存;_fd 在关闭连接后置为 -1，用作连接关闭与否的判断
	int _fd;
	
	
};

#endif

