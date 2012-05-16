#include "PkgHead.h"
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "DebugData.h"



void PkgHead::packet(char *sPkgOutput, int &nOutLen)
{
	nOutLen = 0;
	
	memcpy(sPkgOutput+nOutLen, &usPkgLen, 2);
	nOutLen += 2;

	memcpy(sPkgOutput+nOutLen, &ucDirection, 1);
	nOutLen += 1;
	
	memcpy(sPkgOutput+nOutLen, &ucSrvType, 1);
	nOutLen += 1;

	memcpy(sPkgOutput+nOutLen, &ucSrvSeq, 1);
	nOutLen += 1;

	
	
	memcpy(sPkgOutput+nOutLen, &usMsgType, 2);
	nOutLen += 2;
	
	memcpy(sPkgOutput+nOutLen, &uiUniqID, 4);
	nOutLen += 4;
	
	
	memcpy(sPkgOutput+nOutLen, &uiRoleID, 4);
	nOutLen += 4;
	
}


void PkgHead::unpacket(char *sPkgInput)
{
	int nPosition = 0;
	
	memcpy(&usPkgLen, sPkgInput + nPosition, 2);
	nPosition += 2;

	memcpy(&ucDirection, sPkgInput + nPosition, 1);
	nPosition += 1;
	
	memcpy(&ucSrvType, sPkgInput + nPosition, 1);
	nPosition += 1;

	memcpy(&ucSrvSeq, sPkgInput + nPosition, 1);
	nPosition += 1;

	
	
	memcpy(&usMsgType, sPkgInput + nPosition, 2);
	nPosition += 2;
	
	memcpy(&uiUniqID, sPkgInput + nPosition, 4);
	nPosition += 4;
	
	memcpy(&uiRoleID, sPkgInput + nPosition, 4);
	nPosition += 4;
	
}

void PkgHead::show_packet()
{
DEBUG_PRINTF6( "show_packet: usPkLen[%d],ucSrvType[%d],ucSrvSeq[%d],usMsgType[%d],uiUniqID[%d],ucDirection[%d]\n",
		usPkgLen,		ucSrvType, ucSrvSeq,		usMsgType,
		uiUniqID,		ucDirection );
}







