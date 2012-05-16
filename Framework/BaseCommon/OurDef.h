//公共定义文件头
#ifndef OURDEF_H
#define OURDEF_H

/************************************
 *
 *       网关专用
 *
 ************************************/
//epoll 相关
#define MAXEPOLLFDNUM					5000									//epoll 关注的最大fd数
#define	MAXEVENTS							100										//每次epoll返回能处理的事件数
#define MAXSOCKBUFF						65535									//socket 数据缓存

//服务器相关
#define MAX_SVRTYPE						255										//最大服务器类型
#define MAX_SVRSEQ						255										//最大服务器序号

//
#define GW_CONNCLOSe_INTERNAL	1											//网关连接关闭线程 定时关闭连接的时间间隔

//内外网 tcp 连接
enum
{
	INSIDE_TCP_STREAM,					//内网 TCP 连接
	OUTSIDE_TCP_STREAM					//外网 TCP 连接
};



//------------包体定义-------开始--------------------------------------------
#pragma pack(1)
//S->G 服务器连接网关
struct PKGBODY_SG_SVRCONNECT_REQ
{
	unsigned char		ucSrvType;									//应用服务器类型
	unsigned char		ucSrvSeq;										//应用服务器序号
};

//C->S 角色登录 外网包体
struct PKGBODY_CS_ROLELOGIN_REQ_R
{
	unsigned int		uiRoleID;											//角色ID
	unsigned char		ucPasswd[40];								//角色密码
	
};

//C->S 角色登录 内网包体
struct PKGBODY_CS_ROLELOGIN_REQ_L
{
	unsigned int		uiRoleID;											//角色ID
	unsigned char		ucPasswd[40];								//角色密码
	long long				llClientId;									//客户端id = ip+port+fd
};

//C->S 角色登录 内网包体  应答
struct PKGBODY_CS_ROLELOGIN_RESP_L
{
	unsigned int		uiRetCode;									//返回值
	unsigned int		uiRoleID;											//角色ID
	long long				llClientId;									//客户端id = ip+port+fd
};
#pragma pack()

//------------包体定义-------结束--------------------------------------------


/************************************
 *
 *       通用数据类型
 *
 ************************************/
 //数据类型
typedef bool								Bool;
typedef char								SByte;
typedef unsigned char				Byte;
typedef short								Int16;
typedef unsigned short			UInt16;
typedef int									Int32;
typedef unsigned int				UInt32;
typedef long long						Int64;
typedef unsigned long long	UInt64;
typedef float								Single;
typedef double							Double;

typedef long					 			LONG;
typedef unsigned long 			ULONG;

//服务器类型
#define SVRTYPE_MAIN			1									//main server
#define SVRTYPE_PK				2									//PK server

//服务器序号
#define SVRSEQ_MAIN							1				//主业务服务器序号

//消息类型 msgtype
#define		MSGTYPE_GW_SVRCONNECT							101				//网关消息:服务器连网关
#define		MSGTYPE_GW_CLOSECLIENT						102				//网关消息:关闭指定客户端

#define		MSGTYPE_CS_ROLELOGIN							101				//C_S消息:角色登录 login
#define		MSGTYPE_CS_ROLELOGOUT							104				//C_S消息:角色登出 logout


//包方向
#define	DIRECT_C_S_REQ		1
#define	DIRECT_C_S_RESP		2
#define	DIRECT_S_C_REQ		3
#define	DIRECT_S_C_RESP		4
#define	DIRECT_S_S_REQ		5
#define	DIRECT_S_S_RESP		6 
 

/************************************
 *
 *       应用服务器
 *
 ************************************/

//全局常量
#define PACKET_HEADER_LENGTH 				15	//包头长度
#define MAX_EQUIPTYPE		8					//最大装备类型
#define MAX_MODULE			99				//每个服务器中，最大应用模块数
#define MAX_SESSION 		300				//S_S 服务端并发短连接个数上限

#define RNUM_PER_SEND 20					//每次返回的用户个数，用于地图中所有用户移动信息查询
#define MAXMAPNUM			255					//最大地图个数
#define MAXROLEDIRECT	8						//角色移动方向个数

#define PKSCREEN_XLENGTH	50			//pk屏幕x轴长度
#define PKSCREEN_YLENGTH	29			//pk屏幕y轴长度

#define POSACCURACY				7				//坐标精度，单位，格； 用以说明客户端、服务器当前坐标的误差范围
#define ROLE_MOVE_SPEED		541			//角色场景移动速度 , 单位: 541 微型格/0.1秒

#define LUASCRIPTRETURN_CANACCEPT	99		//lua脚本 return语句的返回值, 99表示任务可接



//任务类型
#define TASKTYPE_MAIN			1									//主线任务
#define TASKTYPE_BRANCH		2									//支线任务
#define TASKTYPE_DAILY		3									//日常任务

//任务完成状态
#define	TASKFINISHSTATUS_NOTFINISHED		0			//任务未完成
#define	TASKFINISHSTATUS_FINISHED				1			//任务已完成
#define	TASKFINISHSTATUS_DELIVERED			2			//任务已交付

//pk ID转换
#define 	CREATUREFLAG_ROLE     100000000			//生物标志_角色,供pk数据转换用
#define   CREATUREFLAG_PET			400000000			//生物标志_宠物,供pk数据转换用


//应用操作定义

/************************************
 *
 *       错误码
 *
 ************************************/
//系统错误定义
#define ERR_SUCCESS										0			//成功              
#define ERR_SYSTEM_TIMEOUT						1			//消息超时          
#define ERR_SYSTEM_LARGEMSG						2			//消息包超过允许长度
#define ERR_SYSTEM_SVRNOTEXISTS				3			//应用服务不存在    
#define ERR_SYSTEM_SVRNOTSTART				4			//应用服务未启动    
#define ERR_SYSTEM_DBERROR 						5			//数据库操作失败    
#define ERR_SYSTEM_DBNORECORD					6			//数据库记录不存在  
#define ERR_SYSTEM_SERERROR						7			//序列化错误        
#define ERR_SYSTEM_SVRLOGINED					8			//服务器已登录      
#define ERR_SYSTEM_SVRACCESS					9			//应用服务访问故障(无法成功发送消息)
#define ERR_SYSTEM_ROLENOTLOGIN				10		//角色未登录
#define ERR_SYSTEM_DATANOTEXISTS			11		//数据不存在
#define ERR_SYSTEM_PARAM							12		//参数错误
#define ERR_SYSTEM_DATAEXISTS					13		//数据已存在


//应用错误定义
#define ERR_APP_ROLENOTEXISTS					1001	//角色不存在
#define ERR_APP_ERRPASSWD							1002	//密码错误
#define ERR_APP_OP										1003	//处理错误
#define ERR_APP_DATA									1004	//业务数据错误
#define ERR_APP_ROLENOTLOGIN					1005	//角色未登陆
#define ERR_APP_PLAYTIMEACCESS				1006	//角色处在防沉迷限制期,不允许登录
#define ERR_APP_ALREADYINMAP					1007	//角色已经进入地图

//角色错误
#define ERR_ROLE_LEVELNOTENOUGH				2001	//级别不够
#define ERR_ROLE_NOMONEY					2002	//金钱不够


#endif








