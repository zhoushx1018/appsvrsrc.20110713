#ifndef PACKET_H
#define PACKET_H

#include "OurDef.h"
#include "DataBuffer.h"
#include <string>

using namespace std;

class Packet
{
public:
	Packet();

	Packet(DataBuffer* buffer);

	Packet(unsigned int bufferSize);
	
	~Packet();

	
	void MakeHeader(UInt16 mid,Byte dir,Byte type, Byte seq, UInt32 RoleID =0,UInt32 tsn=0);

	
	bool PackHeader();

	
	bool UpdatePacketLength();

	
	bool UnpackHeader();

	//数据缓冲区
	inline DataBuffer* GetBuffer()
	{
		return _buffer;
	}

	inline void SetBuffer(DataBuffer* buffer)
	{
		if(_buffer!=NULL&&_ownedBuffer) delete _buffer; 
		
		_buffer = buffer;
		_ownedBuffer = false;
	}

	
	void CopyHeader(const Packet& packet);

public:
	//包长度(包括包头)
	UInt16 Length;

	//数据发送方向
	Byte   Direction;

	//服务器类型
	Byte   SvrType;

	//服务器序号
	Byte   SvrSeq;

	//消息类型 
	UInt16 MsgType;

	//消息唯一 ID
	UInt32 UniqID;

	//角色ID
	UInt32 RoleID;

private:
	bool _ownedBuffer;
	DataBuffer *_buffer;
};

#endif
