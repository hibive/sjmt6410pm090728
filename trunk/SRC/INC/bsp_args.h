#ifndef __BSP_ARGS_H
#define __BSP_ARGS_H

//------------------------------------------------------------------------------
//
// File:        bsp_args.h
//
// Description: This header file defines device structures and constant related
//              to boot configuration. BOOT_CFG structure defines layout of
//              persistent device information. It is used to control boot
//              process. BSP_ARGS structure defines information passed from
//              boot loader to kernel HAL/OAL. Each structure has version
//              field which should be updated each time when structure layout
//              change.
//
//------------------------------------------------------------------------------

#include "oal_args.h"
#include "oal_kitl.h"

#define BSP_ARGS_VERSION    1

#define BSP_ARGS_QUERY_HIVECLEAN        (BSP_ARGS_QUERY)        // Query hive clean flag.
#define BSP_ARGS_QUERY_CLEANBOOT        (BSP_ARGS_QUERY+1)        // Query clean boot flag.
#define BSP_ARGS_QUERY_FORMATPART    (BSP_ARGS_QUERY+2)        // Query format partition flag.

typedef struct
{
    OAL_ARGS_HEADER    header;
    UINT8                deviceId[16];            // Device identification
    OAL_KITL_ARGS        kitl;
    UINT8                uuid[16];
    BOOL                bUpdateMode;            // TRUE = Enter update mode on reboot.
    BOOL                bHiveCleanFlag;            // TRUE = Clean hive at boot
    BOOL                bCleanBootFlag;            // TRUE = Clear RAM, user objectstore at boot
    BOOL                bFormatPartFlag;        // TRUE = Format partion when mounted at boot
    DWORD                nfsblk;                    // for NAND Lock Tight
#ifdef	OMNIBOOK_VER
	BYTE	bBoardRevision;

	BOOL	bSDMMCCH2CardDetect;

	WORD	BS_wRevsionCode;
	WORD	BS_wProductCode;
	WORD	CMD_wType;				// (little-endian, 0x0000 or 'bs' -> 0x6273)
	BYTE	CMD_bMinor;
	BYTE	CMD_bMajor;
	DWORD	WFM_dwFileSize;
	DWORD	WFM_dwSerialNumber;		// (little-endian)
	BYTE	WFM_bRunType;			// (0x00=[B]aseline, 0x01=[T]est/trial, 0x02=[P]roduction, 0x03=[Q]ualification)
	BYTE	WFM_bFPLPlatform;		// (0x00=2.0, 0x01=2.1, 0x02=2.3; 0x03=Vizplex 110; other values undefined)
	WORD	WFM_wFPLLot;			// (little-endian)
	BYTE	WFM_bModeVersion;		// (0x01 -> 0 INIT, 1 DU, 2 GC16, 3 GC4)
	BYTE	WFM_bWaveformVersion;	// (BCD)
	BYTE	WFM_bWaveformSubVersion;// (BCD)
	BYTE	WFM_bWaveformType;		// (0x0B=TE, 0x0E=WE; other values undefined)
	BYTE	WFM_bFPLSize;			// (0x32=5", 0x3C=6", 0x50=8", 0x61=9.7")
	BYTE	WFM_bMFGCode;			// (0x01=PVI, 0x02=LGD)

	DWORD	dwBatteryFault;

	SYSTEMTIME stBootloader;
	SYSTEMTIME stWinCE;

	// ...

#else	//!OMNIBOOK_VER
    HANDLE                 g_SDCardDetectEvent;    //For USB MSF , check SD Card insert & remove.
    DWORD                 g_SDCardState;            //For USB MSF , check SD Card insert & remove.
#endif	OMNIBOOK_VER
} BSP_ARGS;

//------------------------------------------------------------------------------
//
//  Function:  OALArgsInit
//
//  This function is called by other OAL modules to intialize value of argument
//  structure.
//
VOID OALArgsInit(BSP_ARGS *pBSPArgs);

//------------------------------------------------------------------------------

#endif    // __BSP_ARGS_H