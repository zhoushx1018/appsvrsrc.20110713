#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include <malloc.h>
#include <string.h>

class DataBuffer
{
public:
	DataBuffer(unsigned int size)
		:_size(size)
		,_delete(true)
	{
		_pData = (char*)malloc(_size);
		_pRead = _pWrite = _pData;
	}

	DataBuffer(char* data,unsigned int size)
		:_size(size)
		,_delete(false)
	{
		_pData = data;
		_pRead = _pWrite = _pData;
	}


	~DataBuffer()
	{
		if(_pData!=0&&_delete) free(_pData);
	}

	inline unsigned int GetBufferSize() const
	{
		return _size;
	}

	inline unsigned int GetDataSize() const
	{
		return _pWrite - _pRead;
	}

	inline char* GetDataPtr()
	{
		return _pData;
	}

	int Write(const void *src,unsigned int length)
	{
		if( static_cast<unsigned int>(_pWrite-_pData)>_size - length ) return -1;

		memcpy(_pWrite,src,length);

		_pWrite += length;

		return 0;
	}

	int Read(void *dest,unsigned int length)
	{
		if(_pRead + length > _pWrite) return -1;

		memcpy(dest,_pRead,length);

		_pRead += length;

		return 0;
	}

	inline char* GetReadPtr() const
	{
		return _pRead;
	}

	inline int MoveReadPtr(int offset)
	{
		if(_pRead+offset >= _pData
			&& _pRead+offset <= _pWrite)
		{
			_pRead += offset;
			return 0;
		}
		
		return -1;
	}

	inline char* GetWritePtr() const
	{
		return _pWrite;
	}

	inline int MoveWritePtr(int offset)
	{
		if(_pWrite+offset >= _pData
			&&_pWrite+offset <= _pData+_size)
		{
			_pWrite += offset;
			return 0;
		}
		
		return -1;
	}

	inline void Reset()
	{
		_pRead = _pWrite = _pData;
	}
	
private:
	unsigned int _size;
	char *_pData;
	char *_pRead;
	char *_pWrite;
	bool _delete;
	
};
#endif //DATA_BUFFER_H

