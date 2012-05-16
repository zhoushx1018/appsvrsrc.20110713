#include "Packet.h"

#include "DebugData.h"

Packet::Packet()
	:_ownedBuffer(false)
	,_buffer(NULL)
{
}

Packet::Packet(unsigned int bufferSize)
{
	_buffer = new DataBuffer(bufferSize);
	_ownedBuffer = true;
}

Packet::Packet(DataBuffer* buffer)
	:_buffer(buffer)
	,_ownedBuffer(false)
{
}

Packet::~Packet()
{
	if(_buffer!=NULL&&_ownedBuffer) delete _buffer;
}

//本类包头相关字段的赋值
void Packet::MakeHeader(UInt16 mid,Byte dir,Byte type, Byte seq, UInt32 roleID,UInt32 tsn)
{
	Direction = dir;
	SvrType = type;
	SvrSeq = seq;
	MsgType = mid;
	UniqID = tsn;
	
	if(roleID)
	{
		RoleID = roleID;
	}
}

//包头打包, 打包到 databuffer
bool Packet::PackHeader()
{
	//数据缓存清空
	_buffer->Reset();

	_buffer->Write(&Length,sizeof(Length));
	_buffer->Write(&Direction,sizeof(Direction));
	_buffer->Write(&SvrType,sizeof(SvrType));
	_buffer->Write(&SvrSeq,sizeof(SvrSeq));
	
	_buffer->Write(&MsgType,sizeof(MsgType));
	_buffer->Write(&UniqID,sizeof(UniqID));
	_buffer->Write(&RoleID, sizeof(RoleID));

	return true;
}

//包头 '包长度'字段更新
bool Packet::UpdatePacketLength()
{
	Length = _buffer->GetWritePtr()- _buffer->GetDataPtr();

	//写指针移到最前
	if(_buffer->MoveWritePtr(-Length)) return false;

	//重写包头 '包长度'字段
	if(_buffer->Write(&Length,sizeof(Length))) return false;

	//写指针会置
	_buffer->MoveWritePtr(Length-sizeof(Length));

	return true;
}

//包头解包 ,从 databuffer解包
bool Packet::UnpackHeader()
{
	_buffer->Read(&Length,sizeof(Length));
	_buffer->Read(&Direction,sizeof(Direction));
	_buffer->Read(&SvrType,sizeof(SvrType));
	_buffer->Read(&SvrSeq,sizeof(SvrSeq));

	
	_buffer->Read(&MsgType,sizeof(MsgType));
	_buffer->Read(&UniqID,sizeof(UniqID));
	_buffer->Read(&RoleID,sizeof(RoleID));
	
	
	return true;
}

//复制包头数据
void Packet::CopyHeader(const Packet &packet)
{
	Length = packet.Length;
	Direction = packet.Direction;
	SvrType = packet.SvrType;
	SvrSeq = packet.SvrSeq;
	MsgType = packet.MsgType;
	UniqID = packet.UniqID;
	RoleID = packet.RoleID;
}

