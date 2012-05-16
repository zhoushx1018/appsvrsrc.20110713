#include "ini_file.h"

Option option;

int initLog(const char* cfg)
{
	IniFile _file;
	if(!_file.open(cfg)) return -1;

	string dir,size,num,priority,prevname;

	if ( _file.read("log","dir", dir) ) return -1;
	if ( _file.read("log","size", size) ) return -1;
	if ( _file.read("log","num", num) ) return -1;
	if ( _file.read("log","priority", priority) ) return -1;

	//ºöÂÔ prevname Îª¿Õ
	_file.read("log","prename", prevname);

	g_pLog = new Log(dir.c_str(),atoi(priority.c_str())
		,atoi(size.c_str()),atoi(size.c_str()),prevname.c_str());

	return 0;
}

int read_opt(char *file, Option &ini)
{
	IniFile ini_file;
	if (!ini_file.open (file))
	{
		return -1;
	}

	memset(ini.GWIP, 0, sizeof(ini.GWIP));
	memset(ini.GEIP, 0, sizeof(ini.GEIP));
	memset(ini.PKIP, 0, sizeof(ini.PKIP));

	string tmp;

	ini_file.read ("PK", "PK_INDEX", tmp);
	if (tmp.empty ())
		return -1;
	ini.srv_req = atoi (tmp.c_str());

	ini_file.read ("GW", "IP", tmp);
	strncpy (ini.GWIP, tmp.c_str(), sizeof ini.GWIP);
	ini_file.read ("GW", "PORT", tmp);
	if (tmp.empty ())
		return -1;
	ini.GWPort = atoi (tmp.c_str());

	ini_file.read ("GE", "IP", tmp);
	strncpy (ini.GEIP, tmp.c_str(), sizeof ini.GEIP);
	ini_file.read ("GE", "PORT", tmp);
	if (tmp.empty ())
		return -1;
	ini.GEPort = atoi (tmp.c_str());

	ini_file.read ("misc", "IP", tmp);
	strncpy (ini.PKIP, tmp.c_str(), sizeof ini.PKIP);
	ini_file.read ("misc", "PORT", tmp);
	if (tmp.empty ())
		return -1;
	ini.PKPort = atoi (tmp.c_str());

	ini_file.read ("misc", "SOCKET_TIMEOUT", tmp);
	if (tmp.empty ())
		return -1;
	ini.socket_timeout = atoi (tmp.c_str());

	ini_file.read ("misc", "SOCKET_BUFSIZE", tmp);
	if (tmp.empty ())
		return -1;
	ini.socket_bufsize = atoi (tmp.c_str());

	ini_file.read ("misc", "ACCEPT_BACKLOG", tmp);
	if (tmp.empty ())
		ini.backlog = 10;

	ini_file.read ("skill", "COUNT", tmp);
	if (tmp.empty ())
		return -1;
	int skill_count = atoi (tmp.c_str());
	SkillDesc SkillDesc_tmp;
	for(int skill_index = 0; skill_index<skill_count; skill_index++)
	{
		char	skill_key[16] = {0};
		sprintf(skill_key, "SKILL_%d", skill_index);
		ini_file.read ("skill", skill_key, tmp);
		if (tmp.empty ())
			return -1;
		short int skill_id =  atoi (tmp.c_str());

		memset(skill_key, 0, sizeof(skill_key));
		sprintf(skill_key, "SKILL_%d_ING", skill_index);
		ini_file.read ("skill", skill_key, tmp);
		if (tmp.empty ())
			return -1;
		SkillDesc_tmp.DoingTime = atoi (tmp.c_str());

		memset(skill_key, 0, sizeof(skill_key));
		sprintf(skill_key, "SKILL_%d_AREA", skill_index);
		ini_file.read ("skill", skill_key, tmp);
		if (tmp.empty ())
			return -1;
		SkillDesc_tmp.Area = atoi (tmp.c_str());

		char skill_cd[64+1]={0};
		memset(skill_key, 0, sizeof(skill_key));
		sprintf(skill_key, "SKILL_%d_CD", skill_index);
		ini_file.read ("skill", skill_key, tmp);
		if (tmp.empty ())
			return -1;
		strncpy (skill_cd, tmp.c_str(), sizeof(skill_cd));
		char cd[8+1] = {0};
		int cd_index = 0;
		int cd_pos = 0;
		for(int char_loop=0; ; char_loop++)
		{
			if(skill_cd[char_loop] == 0)
				break;
			if(skill_cd[char_loop] == '/')
			{
				SkillDesc_tmp.SkillCD[cd_index] = atoi(cd);
				cd_index++;
				cd_pos = 0;
				memset(cd, 0, sizeof(cd));
				continue;
			}
			cd[cd_pos] = skill_cd[char_loop];
			cd_pos++;
		}

		char skill_blue[64+1]={0};
		memset(skill_key, 0, sizeof(skill_key));
		sprintf(skill_key, "SKILL_%d_BLUE", skill_index);
		ini_file.read ("skill", skill_key, tmp);
		if (tmp.empty ())
			return -1;
		strncpy (skill_blue, tmp.c_str(), sizeof(skill_blue));
		//printf("%sxxxxxxx=%s\n:",skill_key,skill_blue);
		char blue[8+1] = {0};
		int blue_index = 0;
		int blue_pos = 0;
		for(int char_loop=0; ; char_loop++)
		{
			if(skill_blue[char_loop] == 0)
				break;
			if(skill_blue[char_loop] == '/')
			{
				SkillDesc_tmp.MP[blue_index] = atoi(blue);
				//printf("tttttttttttttttttt=%d\n:",SkillDesc_tmp.MP[blue_index]);
				blue_index++;
				blue_pos = 0;
				memset(blue, 0, sizeof(blue));
				continue;
			}
			blue[blue_pos] = skill_blue[char_loop];
			blue_pos++;
		}

		ini.skill_map.insert(make_pair (skill_id, SkillDesc_tmp));
	}

	ini_file.read ("NPC", "COUNT", tmp);
	if (tmp.empty ())
		return -1;
	int npc_count = atoi (tmp.c_str());
	char	npc_key[32] = {0};
	NPCDesc NPCDesc_tmp;
	for(int npc_index = 0; npc_index<npc_count; npc_index++)
	{
		memset(npc_key, 0, sizeof(npc_key));
		sprintf(npc_key, "NPC_%d_TYPE", npc_index);
		ini_file.read ("NPC", npc_key, tmp);
		if (tmp.empty ())
			return -1;
		NPCDesc_tmp.RoleType = atoi (tmp.c_str());

		memset(npc_key, 0, sizeof(npc_key));
		sprintf(npc_key, "NPC_%d_LEVEL", npc_index);
		ini_file.read ("NPC", npc_key, tmp);
		if (tmp.empty ())
			return -1;
		NPCDesc_tmp.Level = atoi (tmp.c_str());
	}

	ini_file.read ("map", "COUNT", tmp);
	if (tmp.empty ())
		return -1;
	ini.map_count= atoi (tmp.c_str());
	char	ini_key[16] = {0};
	char map_file[128] = {0};
	for(int map_index = 1; map_index<=ini.map_count; map_index++)
	{
		unsigned int map_id;
		memset(ini_key, 0, sizeof(ini_key));
		sprintf(ini_key, "MAP_%d", map_index);
		ini_file.read ("map", ini_key, tmp);
		if (tmp.empty ())
			return -1;
		map_id = atoi (tmp.c_str());

		memset(ini_key, 0, sizeof(ini_key));
		memset(map_file, 0, sizeof(map_file));
		sprintf(ini_key, "MAP_%d_PATH", map_index);
		ini_file.read ("map", ini_key, tmp);
		strncpy (map_file, tmp.c_str(), sizeof(map_file));
		
		if(access (map_file, F_OK) != 0)
		{
			printf("get map file:%s error\n", map_file);
			return -1;
		}

		FILE* fp;
		if(!(fp=fopen(map_file, "r")))
			return -1;

		short int Width = 0;
		short int Height = 0;
		fseek(fp, 4, SEEK_SET);
		fread(&Width, 2, 1, fp);

		fseek(fp, 6, SEEK_SET);
		fread(&Height, 2, 1, fp);

		char *map_ptr = NULL;
		map_ptr = new char[Height*Width];
		if(map_ptr == NULL)
			return -1;
		memset(map_ptr, 1, Height*Width);

		fseek(fp, 54, SEEK_SET);

		for(int Height_loop =0; Height_loop < Height; Height_loop ++)
		{
			//printf("%d:",Height_loop);
			for(int width_loop =0; width_loop < Width; width_loop ++)
			{
				fread((map_ptr+(Height_loop*Width + width_loop)) , 1, 1, fp);
				//printf("%d", *(map_ptr+(Height_loop*Width + width_loop)) );
			}
			//printf("\n");
		}

		MapDesc MapDescTmp;
		MapDescTmp.Width = Width;
		MapDescTmp.Height = Height;
		MapDescTmp.Map = map_ptr;
		ini.map_map.insert(make_pair (map_id, MapDescTmp));
		
		fclose(fp);	
	}
	
	return 0;
}
