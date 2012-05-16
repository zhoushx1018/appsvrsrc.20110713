#ifndef PKGBUFF_H
#define PKGBUFF_H

class PkgBuff
{
public:
	PkgBuff();
	PkgBuff( const char * input, int len );
	PkgBuff( const PkgBuff & buf );
	void Append( const void * input, int len );
	char * GetBuff();
	int GetSize();
	void EncodeLength();
	void Clear();
	~PkgBuff();
	
private:

	char * _buff;
	int _size;
};

#endif
