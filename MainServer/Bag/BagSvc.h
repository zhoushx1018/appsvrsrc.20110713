//业务处理 server   背包相关

#ifndef BAGSVC_H
#define BAGSVC_H
//#define TOPCELL_NUM 25
#define TOPSTACK_NUM 99
#define ITEMTYPE_COM 3
#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvBagItemCell.h"
#include "ArchvBag.h"
#include "Role.h"
#include <map>

#ifndef NO_MUCH_MONEY
#define NO_MUCH_MONEY  8888
#endif

#ifndef NO_ITEM
#define NO_ITEM   8887
#endif

class MainSvc;
class NewTimer;

class BagSvc
{
public:
	BagSvc(void* service, ConnectionPool * cp);
	~BagSvc();
	void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================


	void ProcessGetItem(Session& session,Packet& packet);//msgtype 801 查询背包物品列表

	void ProcessBuyItem(Session& session,Packet& packet);//商店物品购买  802

	void ProcessUseItem(Session & session,Packet & packet);//(消耗品）物品使用803

	void ProcessDropItem(Session & session,Packet & packet);//物品丢弃。804

	void ProcessSellItem(Session & session,Packet & packet);//物品出售（给商店）805

	void ProcessSortItem(Session & session,Packet & packet);//物品整理899

	void ProcesschangeItem(Session & session,Packet & packet);//物品位置交换806

    void ProcesschangeTopcell(Session & session,Packet & packet);//改变背包上限807

	void ProcessputintoStoreBag(Session & session,Packet & packet);//把背包东西放到仓库

	void ProcessMixEquip(Session & session,Packet & packet);//装备修理809


	void ProcessUseItemFromItemID(Session & session,Packet & packet);//使用物品从ItemID 810

	void ProcessUseRune(Session & session,Packet & packet);//使用符文 811

	void ProcessQueryBagItem(Session & session,Packet & packet); //根据EntityID查询背包物品信息812

	//===================子业务=======================================


	//---------------------s-c的广播----------------------------------

	void NotifyHPandMP(UInt32 roleID);

	void NotifyBag(UInt32 roleID,List<ItemCell>& lic);

	void NotifyMoney(UInt32 roleID );

	void NotifyBuffUpdate(UInt32 roleID, UInt32 buffID, UInt32 buffTime);

	void NotifyTeamBuffUpdate(UInt32 teamRoleID, UInt32 roleID, UInt32 buffID, UInt32 buffTime);


	//--------------------物品单独处理----------------------------

	UInt16 GetItem(UInt32 roleID,UInt32 ItemID,List<ItemCell>& lic,UInt16 num=1);//得到物品

	UInt16 DropItem(UInt32 RoleID,UInt32 ItemID,List<ItemCell>& lic);//失去物品,每次都是单个

	Int16 DropItems(UInt32 RoleID,List<ItemList>& items,List<ItemCell>& lic);//失去多数物品(一个一个的失去)

	UInt32 UseRune(UInt32 roleID,UInt32 itemId,UInt32 cdTime);//使用符文
	UInt32 UseItemExcludeEquip(UInt32 roleID,UInt32 ItemType,UInt32 CdTime,UInt32 itemId,RolePtr& pRole);
	//=========================钱相关======================================

	UInt32 selectmoney(UInt32 RoleID,UInt32& BindMoney);//查询角色金钱,

	UInt32 selectGold(UInt32 RoleID);//查询金币

	Int16 Getmoney(UInt32 RoleID,UInt32 num);//得到金钱,返回1成功，返回0失败

	Int16 GetBindMoney(UInt32 RoleID,UInt32 num);//得到绑定钱，返回1成功，返回0失败

	Int16 GetBindAndMoney(UInt32 RoleID,UInt32 num1,UInt32 num2);//得到绑定钱和不绑定的

	Int16 GetGold(UInt32 RoleID,UInt32 num);//得到金币，金币充值，返回1成功，返回0失败

	Int16 DropGold(UInt32 RoleID,UInt32 num);//失去金币，返回1撑过，返回0金币不足

	Int16 Dropmoney(UInt32 RoleID,UInt32 num);//失去金钱,返回1成功，返回0金钱不足

	Int16 DropMoneyOnly(UInt32 RoleID,UInt32 num);//没有绑定的金钱的丢失

	Int16 DropMoneyAndGold(UInt32 RoleID,UInt32 Money,UInt32 Gold);

	Int16 GetMoneyAndGold(UInt32 RoleID,UInt32 Money,UInt32 Gold);
	//=================================================================

	UInt16 GetToBag(UInt32 RoleID,UInt16 cell,DBBagItem& item);//由item带进的参数获得物品

	UInt16 GetToBagItem(UInt32 RoleID,UInt32 ItemID,UInt32 EntityID,UInt32 num=1);//获得物品

	UInt16 DeletefromBag(UInt32 RoleID,List<UInt16>& cell,List<DBBagItem>& lic);//从背包中去掉几个单元格，但是不删除

	UInt16 DeletefromBagitem(UInt32 RoleID,UInt16 cell,DBBagItem& item );//从背包中去掉，但是对实体表中不删除,返回物品的具体信息

	Int16 TradeItem(UInt32 RoleID,List<UInt16>& cell,UInt32 ToRoleID,List<UInt16>& cell1 );//两个人交易，传人参数

	Int16 DigTheItem(UInt32 EntityID,UInt32 ItemID,UInt32 holdID);//打孔

	Int16 AddstoneToItem(UInt32 EntityID,UInt32 holdID, UInt32 ItemID);//镶嵌

	Int16 DleteItemForEmail(UInt32 RoleID,UInt16 CellIndex,ItemList& lis,UInt32& EntityID);

	Int16 GetItemFromEmail(UInt32 RoleID,ItemList& lis,UInt32 EntityID);


	//-----------------------CellIndex------------------
	UInt16 IfhascellIndex(UInt32 RoleID);//判断是否有一个单元,有的话，返回cellIndex,没有的话，返回0

	UInt16 Ifhassomany(UInt32 RoleID,List<UInt16>& Cell,UInt16 num);//判断是否含有多个cellIndex，带返回单元格位置的


	//--------------------------------ItemUse-------------------------------------

	UInt32 CompiteUse(UInt32 RoleID,UInt32 itemID,List<ItemCell>& lic);//合成卷轴的使用

	Int16 UseGiftBag(UInt32 RoleID, UInt32 itemID,List<ItemList>& lis);//使用礼包

	Int16 UseVIPItem(UInt32 RoleID,UInt32 itemID,Byte vip);//VIP卡的使用

	Int32 UseItemToHpMp(UInt32 RoleID,UInt32 itemID,UInt32 CdTime,RolePtr& pRole);

	Int32 UseSilverCoinCard(UInt32 roleID);//银币卡的使用

	//---------------------------------------------------------------------

	Int32 GetEmtycellNum(UInt32 roleID);//获取空的单元格数量

	Int32 SelectIfhasItems(UInt32 RoleID,List<ItemList>& items,List<ItemCell>& lic);//查询是否具有这些物品


	Int32 Updatebind(UInt32 RoleID,UInt32 EntityID);//更新背包的位置的绑定状态

	Int32 dropItemIfCell(UInt32 RoleID,ItemCell & items );//对于可以堆叠的物品进行丢弃，进行处理的函数

	//================================外调的===============================

	Int32 RoleGetItem(UInt32 roleID,List<ItemList>& items);//返回0成功，返回1失败

	int RoleGetItem(UInt32 roleID,List<UInt32>& items);//返回0成功，返回1失败

	Int32 RoleDropItem(UInt32 roleID,List<ItemList>& items);//返回0成功，返回1失败


	//-------------------------------------------------------------------------------
	Int32 RoleSelectCell(UInt32 roleID,UInt16 cellIndex,ItemCell& lic);//查询单个Cell

	Int32 RoleSelectCellList(UInt32 roleID,List<UInt16>& cellIndex,List<ItemCell>& lic);//查询多个cell


	//-----------------------装备修理-----------------------------

	UInt32 SelectMixEquipMoney(list<EquipItem>& Entity);

	Int32 UpdateMixEquip(list<EquipItem>& Entity);

	float GetTheNum(UInt32 euippos);

	UInt32 GetRoleCellNum(UInt32 roleID);//获得单元的数量

	//-----------------------回调timer---------------------
	static void TimerDecToughness( void * obj, void * arg, int argLen );
	static void TimerDecStoneness( void * obj, void * arg, int argLen );
	static void TimerDecSaintness( void * obj, void * arg, int argLen );
	static void TimerDecIntelliness( void * obj, void * arg, int argLen );
	static void TimerDecCorrectness( void * obj, void * arg, int argLen );
	static void TimerDecSpeedness( void * obj, void * arg, int argLen );
	static void TimerDecFocusness( void * obj, void * arg, int argLen );
	static void TimerDecFlyness( void * obj, void * arg, int argLen );
	static void TimerDecAngryness( void * obj, void * arg, int argLen );
	static void TimerDecExtremeness( void * obj, void * arg, int argLen );
private:
	MainSvc * _mainSvc;
	ConnectionPool *_cp;

	enum RuneTimerType
	{
		ExpRuneTimer,
		ToughRuneTimer,         //坚韧符文
		StoneRuneTimer,   ////石化符文
		SaintRuneTimer,   //神圣符文
		IntelliRuneTimer, //聪明符文
		CorrectRuneTimer, //精准符文
		SpeedRuneTimer,   //急速符文
		FocusRuneTimer,   //专注符文
		FlyRuneTimer,     //飞翔符文
		AngryRuneTimer,   //狂暴符文
		ExtremeRuneTimer,  //极限符文

		RuneTimerNum,
	};
//	NewTimer*	_runeTimer[RuneTimerNum];
	std::map<UInt32, NewTimer**> _roleRuneTimerMap;  //todo:这些还是放到Role比较好

	void RoleUseToughRune(int roleId, int itemId);
	void RoleUseStoneRune(int roleId, int itemId);
	void RoleUseSaintRune(int roleId, int itemId);
	void RoleUseIntelliRune(int roleId, int itemId);
	void RoleUseCorrectRune(int roleId, int itemId);
	void RoleUseSpeedRuneRune(int roleId, int itemId);
	void RoleUseFocusRune(int roleId, int itemId);
	void RoleUseFlyRune(int roleId, int itemId);
	void RoleUseAngryRune(int roleId, int itemId);
	void RoleUseExtremeRune(int roleId, int itemId);
};


#endif

