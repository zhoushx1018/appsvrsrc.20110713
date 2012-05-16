#include <string.h>

#include "PkgBuff.h"
#include "PkgHead.h"
#include "System.h"

PkgBuff::PkgBuff( )
{
	_buff = NULL;
	_size = 0;
}

PkgBuff::PkgBuff( const char * input, int len )
{
	if( 0 == len )
	{
		_buff = NULL;
		_size = 0;
	}
	else
	{
		_buff = new char [len];
		memcpy( _buff, input, len );
		_size = len;
	}
}


PkgBuff::~PkgBuff()
{
	if( _buff != NULL )
		delete[] _buff;
		
	_buff = NULL;
}

void PkgBuff::Clear()
{
	if( _buff != NULL )
		delete[] _buff;
		
	_buff = NULL;
	_size = 0;
}


PkgBuff::PkgBuff(const PkgBuff& input)
{
	if( input._buff == NULL )
	{
		if( _buff != NULL )
			delete[] _buff;
		
		_buff = NULL;
		_size = 0;
	}
	else
	{		
		char * ptrTmp = _buff;
		
		_buff = new char[input._size];
		memcpy( _buff, input._buff, input._size );
		_size = input._size;
		
		//释放原内存
		if( ptrTmp != NULL )
			delete[] ptrTmp;
		ptrTmp = NULL;		
	}
	
}

void PkgBuff::Append( const void * input, int len )
{
	char * ptrTmp = _buff;
	
	if( len == 0 )
		return;
		
	_buff = new char[_size+len];
	memcpy( _buff, ptrTmp, _size );
	memcpy( _buff+_size, input, len );
	_size += len;
	
	//释放原内存
	if( ptrTmp != NULL )
		delete[] ptrTmp;
	ptrTmp = NULL;
}



char * PkgBuff::GetBuff()
{
	return _buff;
}


int PkgBuff::GetSize()
{
	return _size;
}

//调整包头的 '包长度' 字段
void PkgBuff::EncodeLength()
{
	PkgHead * pkgHead = (PkgHead*)_buff;
	pkgHead->usPkgLen = _size;
		
}




