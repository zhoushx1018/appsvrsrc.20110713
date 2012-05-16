#ifndef BASEARCHIVE_H
#define BASEARCHIVE_H

#include <vector>
#include "Serializer.h"

using namespace std;


//可序列化类的基类
//需要进行序列化操作的类型，需以该类为基类
class BaseArchive
{
public:
	//序列化的操作，留给子类实现
		//isLoading	false  序列化的save数据打包,  即 << 的操作
		//					true	 序列化的load数据解包,  即 >> 的操作
	virtual Serializer& Serialize(Serializer &serializer,bool isLoading) = 0;
	virtual ~BaseArchive();
};


#define BEGIN_SERIAL_MAP()  Serializer& Serialize(Serializer &serializer,bool isLoading){

#define SERIAL_ENTRY(col) if(!serializer.GetErrorCode()){if(isLoading){serializer>>col;}else{serializer<<col;}} 

#define END_SERIAL_MAP() return serializer;}


#endif
