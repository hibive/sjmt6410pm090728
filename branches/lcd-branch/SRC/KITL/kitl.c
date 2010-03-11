//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include <windows.h>
#include <bsp.h>
#include <kitl_cfg.h>
#include <devload.h>
#include <usbdbgser.h>
#include <usbdbgrndis.h>

static volatile S3C6410_GPIO_REG *g_pGPIOReg;
static void InitSROMC_CS8900(void);


//------------------------------------------------------------------------------
//
// Platform entry point for KITL. Called when KITLIoctl (IOCTL_KITL_STARTUP, ...) is called.
//

BOOL OEMKitlStartup(void)
{
    OAL_KITL_ARGS KITLArgs;
    OAL_KITL_ARGS *pKITLArgs;
    BOOL bRet = FALSE;
    UCHAR *szDeviceId,buffer[OAL_KITL_ID_SIZE]="\0";

    OALMSG(OAL_KITL&&OAL_FUNC, (L"[KITL] ++OEMKitlStartup()\r\n"));

    // Look for bootargs left by the bootloader or left over from an earlier boot.
    //
    pKITLArgs = (OAL_KITL_ARGS *)OALArgsQuery(OAL_ARGS_QUERY_KITL);
    szDeviceId = (UCHAR*)OALArgsQuery(OAL_ARGS_QUERY_DEVID);

    // If no KITL arguments were found (typically provided by the bootloader), then select
    // some default settings.
    //
    if (pKITLArgs == NULL)
    {
        memset(&KITLArgs, 0, sizeof(OAL_KITL_ARGS));
        
        // By default, enable KITL and use USB Serial
        KITLArgs.flags |= OAL_KITL_FLAGS_ENABLED;

        KITLArgs.devLoc.IfcType     = Internal;
        KITLArgs.devLoc.BusNumber   = 0;
        KITLArgs.devLoc.PhysicalLoc = (PVOID)S3C6410_BASE_REG_PA_USBOTG_LINK;
        KITLArgs.devLoc.LogicalLoc  = (DWORD)KITLArgs.devLoc.PhysicalLoc;

        pKITLArgs = &KITLArgs;

        pKITLArgs->flags |= OAL_KITL_FLAGS_POLL;        
    }

    if (pKITLArgs->devLoc.LogicalLoc == BSP_BASE_REG_PA_CS8900A_IOBASE)
    {
        // Ethernet specific initialization

        //configure chipselect for cs8900a
        InitSROMC_CS8900();

        //setting EINT10 as IRQ_LAN
        if (!(pKITLArgs->flags & OAL_KITL_FLAGS_POLL))
        {
            g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
            g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<20)) | (0x2<<20);
            g_pGPIOReg->GPNPUD = (g_pGPIOReg->GPNPUD & ~(0x3<<20));                     // pull-up/down disable
            g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(0x7<<20)) | (0x1<<20);   // High Level trigger
        }
    }
    bRet = OALKitlInit ((LPCSTR)szDeviceId, pKITLArgs, g_kitlDevices);

    OALMSG(OAL_KITL&&OAL_FUNC, (L"[KITL] --OEMKitlStartup() = %d\r\n", bRet));

    return bRet;
}


DWORD OEMKitlGetSecs (void)
{
    SYSTEMTIME st;
    DWORD dwRet;
    static DWORD dwBias;
    static DWORD dwLastTime;

    OEMGetRealTime( &st );
    dwRet = ((60UL * (60UL * (24UL * (31UL * st.wMonth + st.wDay) + st.wHour) + st.wMinute)) + st.wSecond);
    dwBias = dwRet;

    if (dwRet < dwLastTime)
    {
        KITLOutputDebugString("[KITL] Time went backwards (or wrapped): cur: %u, last %u\n", dwRet,dwLastTime);
    }

    dwLastTime = dwRet;

    return (dwRet);
}

//------------------------------------------------------------------------------
//
//  Function:  OALGetTickCount
//
//  This function is called by some KITL libraries to obtain relative time
//  since device boot. It is mostly used to implement timeout in network
//  protocol.
//
UINT32 OALGetTickCount()
{
    return OEMKitlGetSecs () * 1000;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMKitlIoctl
//
//  This function is called by some KITL libraries to process platform specific
//  KITL IoCtl calls.
//
BOOL OEMKitlIoctl (DWORD code, VOID * pInBuffer, DWORD inSize, VOID * pOutBuffer, DWORD outSize, DWORD * pOutSize)
{
    BOOL fRet = FALSE;

    switch (code) {
        case IOCTL_HAL_INITREGISTRY:
            OALKitlInitRegistry();
            break;
        default:
            fRet = OALIoCtlVBridge (code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize);
    }
    return fRet;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlInitRegistry
//
//  This function is called during the initialization process to allow the
//  OAL to denote devices which are being used by the KITL connection
//  and thus shouldn't be touched during the OS initialization process.  The
//  OAL provides this information via the registry.
//

VOID OALKitlInitRegistry()
{
    HKEY Key;
    DWORD Status;
    DWORD Disposition;
    DEVICE_LOCATION devLoc;

    // Get KITL device location
    if (!OALKitlGetDevLoc(&devLoc))
        goto CleanUp;

    if (devLoc.LogicalLoc == S3C6410_BASE_REG_PA_USBOTG_LINK)
    {
        // Disable the UsbFn driver since it is used for KITL
        //

        Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\SC6410USBFN", 0, NULL, 0, 0, NULL, &Key, &Disposition);

        if (Status == ERROR_SUCCESS)
        {
            Disposition = DEVFLAGS_NOLOAD;
            // Set Flags value to indicate no loading of driver for this device
            Status = NKRegSetValueEx(Key, DEVLOAD_FLAGS_VALNAME, 0, DEVLOAD_FLAGS_VALTYPE, (PBYTE)&Disposition, sizeof(Disposition));
        }

        // Close the registry key.
        NKRegCloseKey(Key);

        if (Status != ERROR_SUCCESS)
        {
            KITL_RETAILMSG(0, ("OALKitlInitRegistry: failed to set \"no load\" key for Usbfn drvier.\r\n"));
            goto CleanUp;
        }

        KITL_RETAILMSG(ZONE_INIT, ("INFO: USB being used for KITL - disabling Usbfn drvier...\r\n"));
    }

CleanUp:
    return;
}


// for SMDK6410
#define CS8900_Tacs (0x0)   // 0clk
#define CS8900_Tcos (0x4)   // 4clk
#define CS8900_Tacc (0xd)   // 14clk
#define CS8900_Tcoh (0x1)   // 1clk
#define CS8900_Tah  (0x4)   // 4clk
#define CS8900_Tacp (0x6)   // 6clk
#define CS8900_PMC  (0x0)   // normal(1data)

static void InitSROMC_CS8900(void)
{
    volatile S3C6410_SROMCON_REG *s6410SROM = (S3C6410_SROMCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SROMCON, FALSE);

    s6410SROM->SROM_BW = (s6410SROM->SROM_BW & ~(0xF<<4)) |
                            (1<<7)| // nWBE/nBE(for UB/LB) control for Memory Bank1(0=Not using UB/LB, 1=Using UB/LB)
                            (1<<6)| // Wait enable control for Memory Bank1 (0=WAIT disable, 1=WAIT enable)
                            (1<<4); // Data bus width control for Memory Bank1 (0=8-bit, 1=16-bit)

    s6410SROM->SROM_BC1 = ((CS8900_Tacs<<28)+(CS8900_Tcos<<24)+(CS8900_Tacc<<16)+(CS8900_Tcoh<<12)  \
                            +(CS8900_Tah<<8)+(CS8900_Tacp<<4)+(CS8900_PMC));
}

