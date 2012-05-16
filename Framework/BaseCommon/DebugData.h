#ifndef DEBUGDATA_H
#define	DEBUGDATA_H

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

class DebugData{
	public:
		
		/*******************************************************	
		目的：以16进制格式显示某段内存的内容
		参数：
			void *	input					内存段的起始地址
			int	iSize					显示长度
			int 	iShowTail 				是否显示末尾的文本字符 0不显示  1显示
			const char * PROGRAMNAME		源程序名，调用时填 __FILE__ 即可
			int		LINE					在第几行调用ShowHex， 调用时填 __LINE__ 即可
		返回值：
			无
		说明：
			该函数的显示格式参照Ultra Edit显示16进制数据格式
			即每一个字节用一个两位的16进制数来显示:
			00000000h: 00 00 00 00 00 00 00 00 00 00 04 4d 00 00 00 00 ; M
			00000001h: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ; 
			00000002h: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ; 
			00000003h: 00 00 00 00 00 00 00 00 00 00 00 00 ; 
			
			由于ShowHex更多是用于调试程序的目的，因此有必要将调用ShowHex的函数所在的
			源程序名和调用的行数显示出来	
		典型用法：
			int iTmp;
			ShowHex( &iTmp, sizeof(int), 0, __FILE__, __LINE__ );
		*********************************************************/
		static void ShowHex( void * input, int iSize ,int iShowTail, const char * PROGRAMNAME, int LINE)
		{
			
			const int LINECHAR = 16;		/*每行显示字节数，默认为16*/
			int		iLine = 0;				/*行数*/
			int		iCount = 0;				/*每行显示累计*/
			char	cHead[1024];			/*行头*/
			char	cHex[1024];				/*16进制字符*/
			char	cTail[1024];			/*文本字符*/
			char	cTmp[8];
			int		i = 0, iExit = 0;
			
			unsigned char * ptrChar = ( unsigned char * )input;
			
			printf( "\n File[%s] Line[%d] ShowHex, Bytes[%d]\n", PROGRAMNAME, LINE, iSize );
			printf( "-------------------------------\n" );
			if( iSize == 0 )
				return;
			
			while(1)
			{
				/* 退出标记 iExit为1 或者LINECHAR * iLine 正好等于iSize 的时候，退出循环*/
				if( iExit == 1 || ( LINECHAR * iLine >= iSize) )
					break;
					
				iCount = 0;		
				strcpy( cHex, ":" );
				strcpy( cTail, " ; " );
				
				for( i = 0; i < LINECHAR; i++ )
				{
					/*判断是否超出打印范围*/
					if( ( ++iCount + LINECHAR * iLine ) > iSize)
					{
						iExit = 1;
						break;
					}
					
					/*按字符显示*/
					sprintf( cTmp, " %02x", *ptrChar );
					strcat( cHex, cTmp );			
					sprintf( cTmp, "%c", *ptrChar );
					strcat( cTail, cTmp );			
					ptrChar ++;			
				}
				
				sprintf( cHead,"%08xh",iLine );
				strcat( cHead, cHex );
				
				if( iShowTail )
					strcat( cHead, cTail);		
				printf( "%s-\n", cHead );
					
				iLine ++;
			};
			
			printf("--\n--\n--\n");
		}
		
		/*
		*	说明：供调试用的打印函数
		*
		*	用法：
		*		debug_printf( __FILE__, __LINE__, "int[%d] string [%s], iTmp, szTmp );
		*/
		static void Printf( const char * filename, int line, const char * fmt, ... )
		{
			char szBuff[1024];
			int nSize = 0;
			memset( szBuff, 0 ,sizeof(szBuff) );
			
			sprintf( szBuff, "\n[%s] Line.[%d]-->", filename, line );
			nSize = strlen( szBuff );
			
			va_list ap;
			va_start( ap, fmt );
			
			
			vsnprintf( szBuff+nSize, sizeof( szBuff )-nSize-1, fmt, ap ); 
			strcat( szBuff, "\n" );
			
			fprintf( stderr, "%s", szBuff );
			
			va_end(ap);	
			return;
		}

};


#ifdef DEBUG
//=========================if def DEBUG=================================================

//--------debug_showhex--------
#define DEBUG_SHOWHEX(input, size, showtail, file, line)	DebugData::ShowHex( input, size, showtail, file, line)
	
//--------debug_printf--------
#define DEBUG_PRINTF(args)  DebugData::Printf( __FILE__, __LINE__, args )

#define DEBUG_PRINTF1(args, args1)  \
	DebugData::Printf( __FILE__, __LINE__, args, args1 )
	
#define DEBUG_PRINTF2(args, args1, arg2 ) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2 )
	
#define DEBUG_PRINTF3(args, args1, arg2, arg3) \
	DebugData::Printf( __FILE__, __LINE__, args, args1 , arg2, arg3)
	
#define DEBUG_PRINTF4(args, args1, arg2, arg3, arg4) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4 )
	
#define DEBUG_PRINTF5(args, args1, arg2, arg3, arg4, arg5) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5 )
	
#define DEBUG_PRINTF6(args, args1, arg2, arg3, arg4, arg5, arg6) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6 )

#define DEBUG_PRINTF7(args, args1, arg2, arg3, arg4, arg5, arg6, arg7) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7 )
	
#define DEBUG_PRINTF8(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 )
	
#define DEBUG_PRINTF9(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)

#define DEBUG_PRINTF10(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 )
	
#define DEBUG_PRINTF11(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 )

#define DEBUG_PRINTF12(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12 )

#define DEBUG_PRINTF13(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13 )

#define DEBUG_PRINTF14(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14 )

#define DEBUG_PRINTF15(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) \
	DebugData::Printf( __FILE__, __LINE__, args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15 )

	

	
#else
//=========================if not def DEBUG=================================================
//--------debug_showhex--------
#define DEBUG_SHOWHEX(input, size, showtail, file, line)	NULL;

//--------debug_printf--------
#define DEBUG_PRINTF(args)  NULL;

#define DEBUG_PRINTF1(args, args1)  \
	NULL;
	
#define DEBUG_PRINTF2(args, args1, arg2 ) \
	NULL;
	
#define DEBUG_PRINTF3(args, args1, arg2, arg3) \
	NULL;
	
#define DEBUG_PRINTF4(args, args1, arg2, arg3, arg4) \
	NULL;
	
#define DEBUG_PRINTF5(args, args1, arg2, arg3, arg4, arg5) \
	NULL;
	
#define DEBUG_PRINTF6(args, args1, arg2, arg3, arg4, arg5, arg6) \
	NULL;

#define DEBUG_PRINTF7(args, args1, arg2, arg3, arg4, arg5, arg6, arg7) \
	NULL;
	
#define DEBUG_PRINTF8(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	NULL;
	
#define DEBUG_PRINTF9(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	NULL;

#define DEBUG_PRINTF10(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
	NULL;
	
#define DEBUG_PRINTF11(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) \
	NULL;

#define DEBUG_PRINTF12(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) \
	NULL;

#define DEBUG_PRINTF13(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) \
	NULL;

#define DEBUG_PRINTF14(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) \
	NULL;

#define DEBUG_PRINTF15(args, args1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) \
	NULL;


#endif	//#ifdef DEBUG

#endif //#ifndef DEBUGDATA_H
