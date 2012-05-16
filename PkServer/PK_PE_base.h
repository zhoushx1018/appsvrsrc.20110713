#ifndef PK_PE_BASE_H
#define PK_PE_BASE_H

#include <list>
#include <sys/time.h>

using namespace std;

#define 	MAP_SIZE_X 			50
#define 	MAP_SIZE_Y 			29
#define 	MAP_MAX_LINE 			1000	//A*寻路上限，10为一个格子
#define	MAP_IMPACT_AREA		30		//A*检测碰撞的范围
#define	MAX_ROLE_COUNT		30		//最多支持的角色

#define 	TIME_WAITING_READY	60
#define	TIME_OFFLINE			1200

#define	LEN_PKG_HEAD 			15
#define	DIRECT_C_S_REQ		1
#define	DIRECT_C_S_RESP		2
#define	DIRECT_S_C_REQ		3
#define	DIRECT_SC_S_REQ		5
#define	DIRECT_SC_S_RESP		6
#define	SRVTYPE_GE				1
#define	SRVTYPE_PK				2
#define	MSGTYPE_C_S_READY		101
#define	MSGTYPE_C_S_ORDER		102
#define	MSGTYPE_S_S_BEGIN		101
#define	MSGTYPE_S_C_READY		101
#define	MSGTYPE_S_C_BEGIN		102
#define	MSGTYPE_S_C_MOVE			103
#define	MSGTYPE_S_C_ATTACK	 	104
#define	MSGTYPE_S_C_DEAD			107
#define	MSGTYPE_S_C_STOP			108
#define   MSGTYPE_S_C_MULTIHURT	109
#define	MSGTYPE_S_C_SKILL			118
#define	MSGTYPE_S_C_SKILL_BUFFER		119
#define	MSGTYPE_S_C_SKILL_NOLOAD		120
#define   MSGTYPE_S_C_BUFFER_RUN	121
#define   MSGTYPE_S_C_BUFFER_STOP	122
#define   MSGTYPE_S_C_FIGHTEND		127
#define   MSGTYPE_S_C_LOCATAINOFFSET		128
#define	MSGTYPE_S_C_MPCHANGE			129
#define	MSGTYPE_S_C_HPCHANGE			130

#define	MSGTYPE_S_C_STOPSKILL		110

#define	MSGTYPE_S_C_SKILL_NEWROLE	126
#define	MSGTYPE_S_S_CONNECT_GW		101
#define	MSGTYPE_S_S_END_GAME			201

#define 	ORDER_TYPE_NO			0
#define 	ORDER_TYPE_ATTACK_E	1
#define 	ORDER_TYPE_ATTACK_P	2
#define 	ORDER_TYPE_MOVE_E	3
#define 	ORDER_TYPE_MOVE_P	4
#define   ORDER_TYPE_SKILL		5
#define 	ORDER_TYPE_HOLD		6
#define 	ORDER_TYPE_STOP		7
#define	ORDER_TYPE_CASTSKILL	8


#define	ORIGIN_MAIN_ROLE		1		//主角
#define	ORIGIN_PET				2		//宠物
#define	ORIGIN_CALL			3		//召唤
#define	ORIGIN_CLONED			4		//分身

#define	OPPOSITION_1 			1
#define	OPPOSITION_2 			2
#define	OPPOSITION_NEUTRAL 	3
#define	OPPOSITION_HOSTILITY 	4

#define	ROLE_STATUS_LIVE		1
#define	ROLE_STATUS_DEAD		2
#define	ROLE_STATUS_OFFLINE	3

#define	SKILL_TYPE_YueShi				201
#define	SKILL_TYPE_HuoXueShu			202
#define	SKILL_TYPE_YaoShu				203
#define	SKILL_TYPE_JianRenFengBao		204
#define	SKILL_TYPE_YouLingLang			205
#define	SKILL_TYPE_JingXiang			206
#define	SKILL_TYPE_FengBaoZhiChui		207
#define 	SKILL_TYPE_TaoTaiZhiRen		208
#define 	SKILL_TYPE_WuGuangZhiDun		209
#define 	SKILL_TYPE_FanJiLuoXuan		210
#define 	SKILL_TYPE_DiCi					211
#define 	SKILL_TYPE_DuSheShouWei		212
#define 	SKILL_TYPE_PaoXiao				213
#define 	SKILL_TYPE_XueXing				214
#define 	SKILL_TYPE_QuSan				215
#define 	SKILL_TYPE_LianSuoShanDian 	216
#define 	SKILL_TYPE_YouBu			 	217
#define 	SKILL_TYPE_ZhiLiaoShouWei	 	218
#define 	SKILL_TYPE_LiuXingHuoYU	 	219
#define 	SKILL_TYPE_ZhanZhengJianTa	220
#define 	SKILL_TYPE_NuHou			 	221
#define 	SKILL_TYPE_ChongFeng		 	222
#define 	SKILL_TYPE_ZuiJiuYunWu		 	223
#define 	SKILL_TYPE_WuDiZhan		 	224
#define 	SKILL_TYPE_ZuZhou			 	225
#define 	SKILL_TYPE_NengLiangLiuShi	 	226
#define 	SKILL_TYPE_QiangLiYiJi		 	227
#define 	SKILL_TYPE_ShuFuZhiJian	 	228
#define 	SKILL_TYPE_WeiKunZhiJian	 	229
#define 	SKILL_TYPE_SheShouTianFu	 	230
#define 	SKILL_TYPE_BenTeng			 	231
#define 	SKILL_TYPE_FenShen			 	232
#define 	SKILL_TYPE_FeiXueZhiMao	 	233
#define 	SKILL_TYPE_XiSheng			 	234
#define 	SKILL_TYPE_ChenMo	 			235
#define	SKILL_TYPE_FuHuo				236

#define	BUFFER_TYPE_BYSKILL			3000			//通过技能来确定
#define 	BUFFER_TYPE_CALL				3001			//召唤，生命周期一定
#define 	BUFFER_TYPE_XUANYUN			3002			//眩晕，移动攻击魔法均禁止
#define 	BUFFER_TYPE_MOVESPEED		3003			//影响移动速度，但是可以移动
#define 	BUFFER_TYPE_NOMOVE			3004			//禁止移动
#define 	BUFFER_TYPE_HP				3006			//影响HP
#define 	BUFFER_TYPE_MP				3007			//影响MP
#define	BUFFER_TYPE_ARMOR			3008			//影响物理防御
#define	BUFFER_TYPE_CASTSKILL			3009			//持续施法
#define	BUFFER_TYPE_ATTACKPOWER		3010			//影响攻击力
#define	BUFFER_TYPE_MAXHP			3011			//影响最大HP值
#define	BUFFER_TYPE_MAXMP			3012			//影响最大MP值
#define	BUFFER_TYPE_NOSKILL			3013			//禁止使用技能


#define   BUFFER_EFFECT_TYPE_ABS		1		//绝对值
#define   BUFFER_EFFECT_TYPE_RELATIVE	2		//相对值



#define 	DIRECT_NORTH			1
#define 	DIRECT_NORTH_EAST		2
#define 	DIRECT_EAST			3
#define 	DIRECT_EAST_SOUTH		4
#define 	DIRECT_SOUTH			5
#define 	DIRECT_SOUTH_WEST		6
#define 	DIRECT_WEST			7
#define 	DIRECT_WEST_NORTH		8

#define 	LENGTH_STRAIGHT			1000
#define 	LENGTH_DIAGONAL			1400
#define 	MOD_STRAIGHT_DIAGONAL	0.7
#define 	MOD_DIAGONAL_STRAIGHT	1.4

#define 	PK_STATE_INIT			1
#define 	PK_STATE_READY		2
#define 	PK_STATE_PKING			3
#define 	PK_STATE_OVER			4

#define   RoleType_Player			1
#define   RoleType_Monster		2
#define   RoleType_NPC			3
#define   RoleType_Pet			4
#define   RoleType_Call			5


#define   ATTACK_EFFECT_MISS	2
#define   ATTACK_EFFECT_DOUBLE	3
#define   ATTACK_EFFECT_NORMAL 1

#define	HPCHANGE_SOURCE_ATK		1
#define	HPCHANGE_SOURCE_SKILL	2


#define	BULLET_TYPE_ATK_CLOSE		1		//子弹类型-近身物理攻击
#define	BULLET_TYPE_ATK_REMOTE	2		//子弹类型-远程物理攻击

#define	ATTACKRESULT_OK_ATTACK			0		//攻击成功
#define	ATTACKRESULT_NO_ATTACK			1		//无法攻击
#define	ATTACKRESULT_CD_LIMIT				2		//CD时间限制
#define	ATTACKRESULT_TARGETI_NVALID		3		//目标无效
#define	ATTACKRESULT_NO_MOVE			4		//需要移动但禁止移动
#define	ATTACKRESULT_ASTART_FAIL			5		//寻路失败
#define	ATTACKRESULT_MOVE_FAIL			6		//移动失败
#define	ATTACKRESULT_MOVEING				7		//移动中

#define	MOVERESULT_OK_MOVE				0		//移动成功
#define	MOVERESULT_NO_MOVE				1		//禁止移动
#define	MOVERESULT_ERROR					2		//路径长度错误

#define	UPDATABUFFER_ADD					1
#define	UPDATABUFFER_DEL					-1




typedef struct _thread_time
{
	struct timeval tv_start;
	unsigned int	 fragment_now;
}thread_time;

typedef struct _PkgHead
{
	unsigned short	usPkgLen;
	unsigned char		ucDirection;
	unsigned char		ucSrvType;
	unsigned char		ucSrvSeq;
	unsigned short	usMsgType;
	unsigned int		uiUniqID;
	unsigned int		uiRoleID;
	void packet(char *sPkgOutput);
	void unpacket(char *sPkgInput);
}PkgHead;

typedef  char DPOINTER[MAP_SIZE_X][MAP_SIZE_Y];

typedef struct _MapDesc
{
	unsigned short		Width;
	unsigned short		Height;
	char*			Map;
}MapDesc;

struct Pos
{
	short int X;
	short int Y;
	void Offset(const Pos &A) //相对A偏移
	{
		X += A.X;
		Y += A.Y;
	}
	
	bool operator==(const Pos& rhs){
		return (X==rhs.X && Y ==rhs.Y);
	}

};

//用于A*算法的结构
typedef struct _PosA
{
	Pos 		Node;
	Pos 		NodeParent;
	int 		F;
	int		G;
}PosA;

class PhysicalAttr
{
public:
	short int 	AttackArea;			//攻击范围
	unsigned short int 	AttackCDTime;	//攻击CD
	short int 	AttackIngTime;		//攻击消耗时间,攻击频率是指角色攻击一次，所需要等待的时间，单位为0.1秒
	short int	AttackBulletMod;		//子弹时间系数
	short int 	AttackHit;			//命中率
	short int 	DefenseMiss;		//闪避率
	short int 	DefenseArmor;		//护甲

	short int 	AttackPowerHign_Base;	//攻击力基础
	short int 	AttackPowerLow_Base;	//攻击力基础
	short int  AttackPowerAdd;			//攻击力加成
	short int 	AttackCDTime_Base;		//攻击CD
	short int 	AttackHit_Base;			//命中率
	short int 	DefenseMiss_Base;		//闪避率
	short int  AttackCrit_Base;			//基础暴击
	short int 	AttackCrit;				//当前暴击
	short int 	DefenseArmor_Base;		//护甲,1000点护甲会抵挡大约65%的伤害，10:1%; 20:2%;30:3%;100:10%;200:18%;300:26%;400:33%;500:40%

	short int HurtAdd;					//伤害加成

	short int ExemptCounter;			//免疫计数，为0则不免疫物理伤害

	
	unsigned int 	PreAttackTime;		//上一次攻击时间	
	short int LimitCounter;				//禁止攻击计数器，如果为0，则表示可以攻击
	char		PreTimeStatus;

	PhysicalAttr()
	{
		ExemptCounter = 0;
		PreAttackTime = 0;
		LimitCounter = 0;
		HurtAdd = 0;
		AttackPowerAdd = 0;
	}

	
};



class SkillAttr
{
public:
	char		SkillLevel;
	char		DoingTime;
	char		Area;
	short int	MP;
	unsigned short int	SkillCD;
	unsigned int LastUseTime;
	SkillAttr()
	{
		SkillLevel = 0;
		DoingTime = 0;
		Area = 0;
		MP = 0;
		SkillCD = 0;
		LastUseTime = 0;
	}
};

/*
	每一格划分为1000*1000的虚拟格子，
	我们假设用户的移动系数SpeedMod为400，时间窗口为0.1秒，
	意思是说，每0.1秒移动0.4个格子；
	这个具体的位置要参与用户的移动计算
	*/
class MoveAttr
{
public:
	short int			SpeedMod;		//最终移动系数	
	list<Pos> 		MoveLine;		//移动路径，第一个点永远是当前点
	Pos				CurrentPos;		//当前点
	Pos				CurrentPosMini;
	char				Direct;			//方向1:正北；2:东北；3:正南；4:东南；5:正南；6:西南；7:正西；8:西北
	short int			SpeedMod_Base;	//基础移动系数
	short int			LimitCounter;		//禁止移动技术器，0表示可以移动

	MoveAttr()
	{
		LimitCounter = 0;
	}
};

class MagicAttr
{
public:
	short int 			MagicArmor;			//魔法抗性
	short int 			MagicArmor_Base;	//魔法抗性
	map<short int, SkillAttr>	SkillMap;
	short int 			ExemptBadCounter;			//免疫负面魔法计数，0则不免疫
	short int 			ExemptAllCounter;			//免疫所有魔法计数，0则不免疫
	short int 			LimitCounter;			//禁止使用魔法计数器，如果为0则表示可以释放魔法
	MagicAttr()
	{
		LimitCounter = 0;
		ExemptBadCounter = 0;
		ExemptAllCounter = 0;
	}
};



 class Buff
{
public:

	short int 				Type;		//Buffer类型
	short int 				Level;		//技能等级
	short int				ParentSkill;		//引起此Buffer的技能
	unsigned int			BeginTime;		//开始时间
	unsigned int			EndTime;		//结束时间
	bool 				bCanBreak;		//是否可以被打断,true:可以，false:不可以
	bool					bSendClient;		//是否通知客户端
	char					Effecttype;		//影响属性的类型，绝对值或相对值或根据特殊的BUFFER类型定义
	int 					Parametar1;		//附加参数1			
	int 					Parametar2;		//附加参数2
	unsigned short int				RunCounter;		//计数器
	unsigned short int				RunTimeLimit;	//运行时限
	
	Buff( short int type,short int skillID,short int level,unsigned int beginTime,unsigned short int keepTime,bool bBreak,bool bClient )
	{
		Type = type;
		ParentSkill = skillID;
		BeginTime = beginTime;
		EndTime = BeginTime + keepTime;
		bCanBreak = bBreak;
		bSendClient = bClient;
		Parametar1 = 0;
		Parametar2 = 0;
		RunCounter = 0;
		RunTimeLimit = 0;
		Level = level;
		Effecttype = BUFFER_EFFECT_TYPE_ABS;
	}
	
	
};

class RoleOrder
{
public:
	short int 	Type;					// 0:no;1:attack E;2:attack P;3:move E;4:move P;5:hold;6:stop;128-65536:skill
	char		TargetType;				// 1:Pos; 2:RoleID;
	Pos 		TargetPos;
	int 		TargetRoleID;
	unsigned int	OrderTime;
	RoleOrder()
	{
		

	}
};

class HaterInfo
{
public:
	short int Distance;		//距离关系影响
	short int Hp;			//血量影响
	short int Attack;		//攻击次数影响
	short int Hit;			//受击次数影响
	short int Hurt;		//单位时间内伤害影响
	HaterInfo()
	{
		Distance = 0;
		Hp = 0;
		Attack = 0;
		Hit = 0;
		Hurt = 0;
	}
	void clear()
	{
		Distance = 0;
		Hp = 0;
		Attack = 0;
		Hit = 0;
		Hurt = 0;
	}
	short int max()
	{
		return Distance + Hp + Attack + Hit;
	}
};
//PK状态
class RoleState
{
public:
	int 				ControlID;		//控制者ID
	int 				PK_State;		//准备状态
	short int			Strength;		//力量
	short int 			Agile;			//敏捷
	short int 			Wisdom;		//智力
	int				RoleID;			//角色ID
	char 			RoleFlag;		//角色标志  1 角色  2 怪  3 npc  4 宠物
	int 				RoleType;		//角色类型
	char				Opposition;		// 1:阵容1；2:阵容2；3:中立；4:全仇恨；
	//char				Origin;		// 1:本身；2:宠物；3:召唤；4:分身
	char				Live;			// 1:活；2:死；3:离线
	short int			Level;			//等级

	short int 			HP_MAX;
	short int 			Curr_HP_MAX;
	short int 			MP_MAX;
	short int 			Curr_MP_MAX;
	short int 			HP;
	short int 			MP;
	char				AttackType;		//  1:指定，2:自动
	RoleOrder 		CurrentOrder;	//用户对这个角色的操作
	

	short int			Attack_Skill_WaitingTime;	//当前硬直时间,可以由物理攻击和技能发动引发
	MoveAttr			MoveAttrb;			//角色的位置属性
	PhysicalAttr 		PhysicalAttrib;		//角色物理状态
	MagicAttr			MagicAttrb;

	multimap<short int, Buff> multimap_buff;

	map<int, HaterInfo>	HatreMap;
	void Init()
	{	
		Curr_HP_MAX = HP_MAX;
		Curr_MP_MAX = MP_MAX;
		MoveAttrb.SpeedMod = MoveAttrb.SpeedMod_Base;
		MoveAttrb.LimitCounter = 0;
		PhysicalAttrib.AttackCDTime = PhysicalAttrib.AttackCDTime_Base;
		PhysicalAttrib.AttackHit = PhysicalAttrib.AttackHit_Base;
		PhysicalAttrib.DefenseArmor  =PhysicalAttrib.DefenseArmor_Base;
		PhysicalAttrib.DefenseMiss = PhysicalAttrib.DefenseMiss_Base;
		PhysicalAttrib.AttackCrit = PhysicalAttrib.AttackCrit_Base;
		PhysicalAttrib.LimitCounter = 0;
		PhysicalAttrib.ExemptCounter = 0;
		MagicAttrb.MagicArmor = MagicAttrb.MagicArmor_Base;
		MagicAttrb.LimitCounter = 0;
		PhysicalAttrib.HurtAdd = 0;
		MagicAttrb.ExemptBadCounter = 0;
		MagicAttrb.ExemptAllCounter = 0;
		Attack_Skill_WaitingTime = 0;
		MoveAttrb.CurrentPosMini.X = 500;
		MoveAttrb.CurrentPosMini.Y = 500;
		PhysicalAttrib.PreAttackTime = 0;
		PhysicalAttrib.PreTimeStatus = 0;
		CurrentOrder.Type = ORDER_TYPE_NO;
		CurrentOrder.OrderTime = 0;
		Live = ROLE_STATUS_LIVE;
	}
	void Dead()
	{
		Live = ROLE_STATUS_DEAD;
		HP = 0;
		CurrentOrder.Type = ORDER_TYPE_NO;
		multimap_buff.clear();
		HatreMap.clear();
		MoveAttrb.MoveLine.clear();
	}
};





typedef struct _Bullet
{
	RoleState*		Role_Attack;			//攻击者
	RoleState*		Role_Passive;		//受击者
	unsigned short int	Start_Time;			//开始时间
	Pos				CurrentPos;			//当前点
	char				Speed;				// 子弹速度
	short int			BulletType;			// 子弹类型，1近身物理攻击2远程攻击，其他为技能
	char				Level;				//技能等级
}Bullet;

typedef struct _Skill_Order
{
	RoleState*		role;			//角色
	unsigned short int	Start_Time;
	RoleOrder			order;
	char				SkillLevel;
}Skill_Order;


//返回值>0表示路径的转折点数量，其他值表示失败
int AStar(Pos StartPos, Pos EndPos, list<Pos> *MoveLine, char (*pk_map)[MAP_SIZE_Y]);

double Pnt2SegmentDist(const Pos & A,const Pos & B,const Pos & C);
double Dist(const Pos & A, const Pos & B,bool bSqrt = true);
char getDirect( const Pos & startPos,const Pos & endPos );
short int getBHitTarget( const short int attackRoleHit,const short int targetRoleMiss );

#endif

