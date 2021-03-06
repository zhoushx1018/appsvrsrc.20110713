/**
 *	交易 序列化用的类
 *	
 */

#ifndef ARCHVTRADE_H
#define ARCHVTRADE_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"
#include "ArchvBagItemCell.h"

//简要任务信息
class ArchvTrade
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvTrade():RoleID(0),Money(0),Gold(0)
	{}
	
public:
	//成员变量
	UInt32	RoleID;
	UInt32 Money;
	UInt32 Gold;
	List<UInt16> lic;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(RoleID)
		SERIAL_ENTRY(Money)
		SERIAL_ENTRY(Gold)
		SERIAL_ENTRY(lic)		
	END_SERIAL_MAP()

};

class TradeItem
	  :public BaseArchive
{
 public:
  	
  TradeItem():isOnTrade(0),tradeRoleID(0),isLockTrade(0),isTrade(0),tradeMoney(0),tradeGold(0)
  {}

  ~TradeItem()
  {}

 public:
  	
  Byte         isOnTrade;   //角色是否正在交易 0 不在交易， 1 正在交易
  UInt32       tradeRoleID; //交易的角色(和谁交易)
  Byte         isLockTrade; //是否锁定交易  0 未锁定� 1 锁定
  Byte         isTrade;     //是否点击了交易 0 未交易，1 交易
  UInt32       tradeMoney;  //交易的非绑定银币
  UInt32       tradeGold;   //交易的金币
  List<UInt16> tradeCellIndex; //角色交易的背包单元格位置

 public:
 	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(isOnTrade)
		SERIAL_ENTRY(tradeRoleID)
		SERIAL_ENTRY(isLockTrade)
		SERIAL_ENTRY(isTrade)
		SERIAL_ENTRY(tradeMoney)
		SERIAL_ENTRY(tradeGold)
		SERIAL_ENTRY(tradeCellIndex)
	END_SERIAL_MAP()
  
};


#endif

