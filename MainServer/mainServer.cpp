#include "Daemon.h"
#include "MainSvc.h"
#include "Log.h"
#include "DebugData.h"

int main(int argc,char* argv[])
{
	if (argc < 2)
	{
		printf ("usage: Avatar config_file\n");
		return -1;
	}

	char szFileName[256] = {0};
	strncpy (szFileName ,argv[1], sizeof (szFileName)-1 );

	if(access (szFileName, F_OK) != 0)
	{
		DEBUG_PRINTF1("ini file [%s] not found !!!!!!!!!", szFileName );
		return -1;
	}
	
	if(Daemon<GWProxy<MainSvc> >::Run(szFileName))
		LOG(LOG_ERROR,__FILE__,__LINE__,"daemon run error!!" );
	
	return 0;

}

