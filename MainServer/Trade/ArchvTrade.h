/**
 *	½»Ò× ĞòÁĞ»¯ÓÃµÄÀà
 *	
 */

#ifndef ARCHVTRADE_H
#define ARCHVTRADE_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"
#include "ArchvBagItemCell.h"

//¼òÒªÈÎÎñĞÅÏ¢
class ArchvTrade
	:public BaseArchive
{
	
public:
	//³ÉÔ±±äÁ¿¸³³õÖµ
	//	·Ç stringÀàĞÍ¡¢·ÇListÄ£°åÀàĞÍµÄ³ÉÔ±±äÁ¿£¬½¨Òé¶¼¸³ÉÏ³õÖµ
	ArchvTrade():RoleID(0),Money(0),Gold(0)
	{}
	
public:
	//³ÉÔ±±äÁ¿
	UInt32	RoleID;
	UInt32 Money;
	UInt32 Gold;
	List<UInt16> lic;

//³ÉÔ±±äÁ¿ĞòÁĞ»¯²Ù×÷£¬SerializerÀà½«°´×Ö¶ÎË³Ğò½øĞĞĞòÁĞ»¯
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
  	
  Byte         isOnTrade;   //½ÇÉ«ÊÇ·ñÕıÔÚ½»Ò× 0 ²»ÔÚ½»Ò×£¬ 1 ÕıÔÚ½»Ò×
  UInt32       tradeRoleID; //½»Ò×µÄ½ÇÉ«(ºÍË­½»Ò×)
  Byte         isLockTrade; //ÊÇ·ñËø¶¨½»Ò×  0 Î´Ëø¶¨£ 1 Ëø¶¨
  Byte         isTrade;     //ÊÇ·ñµã»÷ÁË½»Ò× 0 Î´½»Ò×£¬1 ½»Ò×
  UInt32       tradeMoney;  //½»Ò×µÄ·Ç°ó¶¨Òø±Ò
  UInt32       tradeGold;   //½»Ò×µÄ½ğ±Ò
  List<UInt16> tradeCellIndex; //½ÇÉ«½»Ò×µÄ±³°üµ¥Ôª¸ñÎ»ÖÃ

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

