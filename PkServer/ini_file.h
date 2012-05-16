#ifndef _INIFILE_H__
#define _INIFILE_H__
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <fstream>
#include "PK_PE_base.h"
#include "log.h"

using namespace std;

typedef map<string, string, less<string> > strMap;
typedef strMap::iterator strMapIt;

const char*const MIDDLESTRING = "_____***_______";

struct analyzeini{
	string strsect;
	strMap *pmap;
	analyzeini(strMap & strmap):pmap(&strmap){}	
	void operator()( const string & strini)
	{
		int first =strini.find('[');
		int last = strini.rfind(']');
		if( first != (int)string::npos && last != (int)string::npos && first != last+1)
		{
					strsect = strini.substr(first+1,last-first-1);
					return ;
		}
		if(strsect.empty())
				return ;
		if((first=strini.find('='))== (int)string::npos)
			return ;
		string strtmp1= strini.substr(0,first);
		string strtmp2=strini.substr(first+1, string::npos);
		first= strtmp1.find_first_not_of(" \t");
		last = strtmp1.find_last_not_of(" \t");
		if(first == (int)string::npos || last == (int)string::npos)
			return ;
		string strkey = strtmp1.substr(first, last-first+1);
		first = strtmp2.find_first_not_of(" \t");
    if(((last = strtmp2.find("\t#", first )) != -1) ||
            ((last = strtmp2.find(" #", first )) != -1) ||
            ((last = strtmp2.find("\t//", first )) != -1)||
            ((last = strtmp2.find(" //", first )) != -1))
    {
            strtmp2 = strtmp2.substr(0, last-first);
    }
    last = strtmp2.find_last_not_of(" \t");
    if(first == (int)string::npos || last == (int)string::npos)
    	return ;
    string value = strtmp2.substr(first, last-first+1);
	string mapkey = strsect + MIDDLESTRING;
	mapkey += strkey;
    (*pmap)[mapkey]=value;

	//	cout <<mapkey <<"=" <<value <<endl;
    return ;
	}
};

class IniFile
{
public:
    IniFile( ){};
    ~IniFile( ){};
    bool open(const char* pinipath)
	{
		return do_open(pinipath);
	}

	//读取参数
	//返回值 0成功  非0 失败或者未找到
	int read(const char*psect, const char*pkey, string &output)
	{
		string mapkey = psect;
		mapkey += MIDDLESTRING;
		mapkey += pkey;
		strMapIt it = c_inimap.find(mapkey);
		//cout << mapkey << "=" << it->second <<endl;
		if(it == c_inimap.end())
		{
			output = "";
			return -1;
		}
		else
			output = it->second;

		return 0;
	}

protected:
    bool do_open(const char* pinipath)
	{
		ifstream fin(pinipath);
		if(!fin.is_open())
			 return false;
		vector<string> strvect;
		while(!fin.eof())
		{
			string inbuf, subbuf;
			getline(fin, inbuf,'\n');
			int last_pos = inbuf.find_last_of("\r\n");
			if (last_pos != (int)string::npos) 
				subbuf = inbuf.substr(0, last_pos);
			else
				subbuf = inbuf;

			strvect.push_back(subbuf);
		}
		if(strvect.empty())
			return false;
		for_each(strvect.begin(), strvect.end(), analyzeini(c_inimap));
		return !c_inimap.empty();		
	}
    strMap    c_inimap;
};

typedef struct _SkillDesc
{
	char		DoingTime;
	char		Area;
	short int	MP[6];
	unsigned short int	SkillCD[6];
}SkillDesc;

typedef struct _NPCDesc
{
	short int 		RoleType;		//角色类型
	short int		Level;			//等级
	short int 		HP;
	short int 		MP;
	short int		SpeedMod;
	short int 		AttackPowerHign;
	short int 		AttackPowerLow;
	short int 		AttackCDTime;
	short int 		AttackIngTime;
	short int		AttackBulletMod;	
	short int 		AttackHit;
	short int 		DefenseMiss;
	short int 		DefenseArmor;
	short int		SKILLCOUNT;
	map<short int, char>			SKILL_MAP;
}NPCDesc;

typedef struct _Option
{
	char		srv_req;
	
	char 	GWIP[16];
	short int	GWPort;
	char		GEIP[16];
	short int	GEPort;
	
	char		PKIP[16];
	short int	PKPort;
	int		socket_bufsize;
	short int	socket_timeout;
	short int	backlog;

	char		log_dir[128];
	int		log_size;
	char		log_prename[16];
	short int	log_priority;
	short int	log_num;

	short int	map_count;
	map<unsigned int, MapDesc>		map_map;		//地图指针
	map<short int, SkillDesc>			skill_map;	
	map<short int, NPCDesc>			npc_map;
}Option;

int initLog(const char* cfg);
int read_opt(char *file, Option &PKOption);

extern Option option;

#endif

