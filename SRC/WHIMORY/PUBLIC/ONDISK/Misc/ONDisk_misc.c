/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketMory                                                      */
/* MODULE  : Block Driver for supporting FAT File system                     */
/* FILE    : ONDisk_misc.c                                                   */
/* PURPOSE : This file implements Windows CE Block device driver interface   */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2009 SAMSUNG ELECTRONICS CO., LTD.                */
/*                          ALL RIGHTS RESERVED                              */
/*                                                                           */
/*   Permission is hereby granted to licensees of Samsung Electronics        */
/*   Co., Ltd. products to use or abstract this computer program for the     */
/*   sole purpose of implementing a product based on Samsung                 */
/*   Electronics Co., Ltd. products. No other rights to reproduce, use,      */
/*   or disseminate this computer program, whether in part or in whole,      */
/*   are granted.                                                            */
/*                                                                           */
/*   Samsung Electronics Co., Ltd. makes no representation or warranties     */
/*   with respect to the performance of this computer program, and           */
/*   specifically disclaims any responsibility for any damages,              */
/*   special or consequential, connected with the use of this program.       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* REVISION HISTORY                                                          */
/*                                                                           */
/*   30-JAN-2009 [HMSEO]   : first writing                                   */
/*                                                                           */
/*****************************************************************************/

#include <windows.h>
#include <bldver.h>
#include <windev.h>
#include <types.h>
#include <excpt.h>
#include <tchar.h>
#include <devload.h>
#include <diskio.h>
#include <storemgr.h>

#include <WMRConfig.h>
#include <WMRTypes.h>
#include <VFLBuffer.h>
#include <HALWrapper.h>
#include <config.h>
#include <WMR.h>
#include <WMR_Utils.h>

/*****************************************************************************/
/* Imported variable declarations                                            */
/*****************************************************************************/
HANDLE 	g_hThread = NULL;
BOOL    g_bNandScan = FALSE;

/*****************************************************************************/
/* Imported function declarations                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local #define                                                             */
/*****************************************************************************/
#define     SCAN_OS            (1<<1)
#define     SCAN_FS            (1<<2)

/*****************************************************************************/
// Local constant definitions
/*****************************************************************************/

/*****************************************************************************/
// Local typedefs
/*****************************************************************************/

/*****************************************************************************/
// Local function prototypes
/*****************************************************************************/
DWORD WINAPI CallFTL_Reclaim(LPVOID lpParameter);
DWORD WINAPI ForceFTL_Reclaim(LPVOID lpParameter);
DWORD WINAPI CallFTL_Scan(LPVOID lpParameter);
DWORD WINAPI ForceFTL_Scan(LPVOID lpParameter);

/*****************************************************************************/
// Function definitions
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Scan                                                            */
/* DESCRIPTION                                                               */
/*      This function is Main function to create thread                      */
/* PARAMETERS                                                                */
/* RETURN VALUES                                                             */
/*      TRUE  : Success to create thread                                     */
/*      FALSE : Fail to create thread                                        */
/*                                                                           */
/*****************************************************************************/
BOOL NAND_Scan()
{
    if ( g_bNandScan == TRUE )
        return TRUE;
        
	RETAILMSG(1,(TEXT("Create thread!!!\r\n")));
	g_bNandScan = TRUE;
    
	g_hThread = CreateThread(NULL,NULL,CallFTL_Reclaim,NULL,NULL,NULL);
	if ( g_hThread == NULL )
	{
	    RETAILMSG(1,(TEXT("CallFTL_Reclaim Thread Creat Fail!!!(LastError : 0x%x)\r\n"), GetLastError()));
	    return FALSE;
	}
	g_hThread = CreateThread(NULL,NULL,ForceFTL_Reclaim,NULL,NULL,NULL);
	if ( g_hThread == NULL )
	{
	    RETAILMSG(1,(TEXT("ForceFTL_Reclaim Thread Creat Fail!!!(LastError : 0x%x)\r\n"), GetLastError()));
	    return FALSE;
	}
	g_hThread = CreateThread(NULL,NULL,CallFTL_Scan,NULL,NULL,NULL);
	if ( g_hThread == NULL )
	{
	    RETAILMSG(1,(TEXT("CallFTL_Scan Thread Creat Fail!!!(LastError : 0x%x)\r\n"), GetLastError()));
	    return FALSE;
	}
    return TRUE;
}

// This function calls FTL_Reclaim fucntion every 1 seconds.
DWORD WINAPI CallFTL_Reclaim(LPVOID lpParameter)
{
	unsigned long dwStartTick, dwIdleSt, dwStopTick, dwIdleEd, PercentIdle;
	int i;

	RETAILMSG(1,(TEXT("CallFTL_Reclaim thread start!!!\r\n")));
    while(1)
    {
		Sleep(30*60*1000);    // Wait 30 Minutes
        while(1)
        {
    		dwStartTick = GetTickCount();
    		dwIdleSt = GetIdleTime();
    		Sleep(1000);    // 1 Seconds
    		dwStopTick = GetTickCount();
    		dwIdleEd = GetIdleTime();
    		PercentIdle = ((100*(dwIdleEd - dwIdleSt)) / (dwStopTick - dwStartTick));
    
            if ( PercentIdle > 90 )
            {
//      		RETAILMSG(1,(_T(" Idle Time : %3d%% :"),PercentIdle));
//        		RETAILMSG(1,(TEXT("Call FTL_ReadReclaim!!!\r\n")));
                FTL_ReadReclaim();
                break;
            }
        }
    }
}

// This function calls FTL_Scan fucntion every 1 seconds.
DWORD WINAPI CallFTL_Scan(LPVOID lpParameter)
{
	unsigned long dwStartTick, dwIdleSt, dwStopTick, dwIdleEd, PercentIdle;
	int i;
	UINT32 mode;

	RETAILMSG(1,(TEXT("CallFTL_Scan thread start!!!\r\n")));
    while(1)
    {
		Sleep(30*60*1000);    // Wait 30 Minutes
		while (1)
		{
    		dwStartTick = GetTickCount();
    		dwIdleSt = GetIdleTime();
    		Sleep(1000);    // 1 Seconds
    		dwStopTick = GetTickCount();
    		dwIdleEd = GetIdleTime();
    		PercentIdle = ((100*(dwIdleEd - dwIdleSt)) / (dwStopTick - dwStartTick));
    
            if ( PercentIdle > 90 )
            {
//        		RETAILMSG(1,(_T(" Idle Time : %3d%% :"),PercentIdle));
//        		RETAILMSG(1,(TEXT("Call FTL_Scan!!!\r\n")));
        		mode = (SCAN_FS|SCAN_OS);
                FTL_Scan(mode);
                break;
            }
        }
    }
}

DWORD WINAPI ForceFTL_Reclaim(LPVOID lpParameter)
{
	RETAILMSG(1,(TEXT("ForceFTL_ReadReclaim thread start!!!\r\n")));
    while(1)
    {
		Sleep(30*64*1000);  // 30 minutes...
//		RETAILMSG(1,(TEXT("Force FTL_ReadReclaim!!!\r\n")));
        FTL_ReadReclaim();
    }
}

DWORD WINAPI ForceFTL_Scan(LPVOID lpParameter)
{
    UINT32 mode;
	RETAILMSG(1,(TEXT("ForceFTL_Scan thread start!!!\r\n")));
    while(1)
    {
		Sleep(5*64*1000);  // 5 minutes...
//		RETAILMSG(1,(TEXT("Force FTL_Scan!!!\r\n")));
   		mode = (SCAN_FS|SCAN_OS);
        FTL_Scan(mode);
    }
}
