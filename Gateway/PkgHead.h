// 通讯包结构、通讯包头、个别通讯包体

#ifndef PKGHEAD_H
#define	PKGHEAD_H

#include "OurDef.h"


#pragma pack(1)
struct PkgHead
{
	unsigned short	usPkgLen;
	unsigned char		ucDirection;
	unsigned char		ucSrvType;
	unsigned char		ucSrvSeq;
	unsigned short	usMsgType;
	unsigned int		uiUniqID;
	unsigned int		uiRoleID;
	void unpacket(char *sPkgInput);
	void packet(char *sPkgOutput, int &nOutLen);
	void show_packet();
};

#pragma pack()

#endif

