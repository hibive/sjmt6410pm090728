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
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    haldd.cpp

Abstract:

    The Implementation HAL function to support DirectDraw

Functions:

    HalGetDriverInfo, ...

Notes:

--*/

#include "precomp.h"

/// Defined in HALCAPS.cpp
extern DDHAL_DDMISCELLANEOUSCALLBACKS MiscellaneousCallbacks;
extern DDHAL_DDCOLORCONTROLCALLBACKS ColorControlCallbacks;
extern unsigned char    *g_pVideoMemory;        // virtual address of video memory from client's side

//---------------------
// DDHAL_DDCALLBACKS
//---------------------

DWORD
WINAPI
HalGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpgdid)
{
    /*
    typedef struct _DDHAL_GETDRIVERINFODATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        DWORD dwSize;
        DWORD dwFlags;
        GUID guidInfo;
        DWORD dwExpectedSize;
        LPVOID lpvData;
        DWORD dwActualSize;
        HRESULT ddRVal;
    } DDHAL_GETDRIVERINFODATA;
    */

    DWORD dwSize;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DDHAL] ++HalGetDriverInfo()\n\r")));

    if (IsEqualIID(lpgdid->guidInfo, GUID_MiscellaneousCallbacks) )
    {
        DEBUGMSG(DISP_ZONE_TEMP, (_T("[DDHAL] GUID_MiscellaneousCallbacks\n\r")));
        dwSize = min(lpgdid->dwExpectedSize, sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS));
        lpgdid->dwActualSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);

        memcpy(lpgdid->lpvData, &MiscellaneousCallbacks, dwSize);
        lpgdid->ddRVal = DD_OK;
    }
    else if (IsEqualIID(lpgdid->guidInfo, GUID_ColorControlCallbacks) )
    {
        DEBUGMSG(DISP_ZONE_TEMP, (_T("[DDHAL] GUID_ColorControlCallbacks\n\r")));
        dwSize = min(lpgdid->dwExpectedSize, sizeof(DDHAL_DDCOLORCONTROLCALLBACKS));
        lpgdid->dwActualSize = sizeof(DDHAL_DDCOLORCONTROLCALLBACKS);

        memcpy(lpgdid->lpvData, &ColorControlCallbacks, dwSize);
        lpgdid->ddRVal = DD_OK;
    }
    else if (IsEqualIID(lpgdid->guidInfo, GUID_VideoPortCallbacks))
    {
        DEBUGMSG(DISP_ZONE_TEMP,(_T("[DDHAL:ERR] GUID_VideoPortCallbacks\n\r")));
        lpgdid->ddRVal = DDERR_CURRENTLYNOTAVAIL;

    }
    else if (IsEqualIID(lpgdid->guidInfo, GUID_VideoPortCaps))
    {
        DEBUGMSG(DISP_ZONE_TEMP,(_T("[DDHAL:ERR] GUID_VideoPortCaps\n\r")));
        lpgdid->ddRVal = DDERR_CURRENTLYNOTAVAIL;
    }
#if (_WIN32_WCE >= 600)
    else if (IsEqualIID(lpgdid->guidInfo, GUID_GetDriverInfo_VidMemBase))
    {
        DEBUGMSG(DISP_ZONE_TEMP,(_T("[DDHAL:ERR] GUID_GetDriverInfo_VidMemBase\n\r")));    
        dwSize = min(lpgdid->dwExpectedSize, sizeof(unsigned int));
        lpgdid->dwActualSize = sizeof(unsigned int);

        *(reinterpret_cast< unsigned char ** >(lpgdid->lpvData) ) = g_pVideoMemory;
        lpgdid->ddRVal = DD_OK;
    }
#endif
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] Unknown GUID\n\r")));
        lpgdid->ddRVal = DDERR_CURRENTLYNOTAVAIL;
    }

    RETAILMSG(DISP_ZONE_ENTER,(_T("[DDHAL] --HalGetDriverInfo()\n\r")));

    return DDHAL_DRIVER_HANDLED;
}

DWORD WINAPI HalWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpwfvbd)
{
    DEBUGENTER( HalWaitForVerticalBlank );
    /*
    typedef struct _DDHAL_WAITFORVERTICALBLANKDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        DWORD dwFlags;
        DWORD bIsInVB;
        HRESULT ddRVal;
    } DDHAL_WAITFORVERTICALBLANKDATA;
    */

    S3C6410Disp *pDDGPE = (S3C6410Disp *)GetDDGPE();

    if (lpwfvbd->dwFlags & DDWAITVB_BLOCKBEGIN)            // Returns when the vertical blank interval begins.
    {
        lpwfvbd->bIsInVB = pDDGPE->WaitForVerticalBlank(VB_FRONTPORCH);
    }
    else if (lpwfvbd->dwFlags & DDWAITVB_BLOCKEND)        // Returns when the vertical blank interval ends and display begins.
    {
        lpwfvbd->bIsInVB = pDDGPE->WaitForVerticalBlank(VB_BACKPORCH);
    }
    else if (lpwfvbd->dwFlags & DDWAITVB_I_TESTVB)        // Sets the flag to query if a vertical blank is in progress.
    {
        lpwfvbd->bIsInVB = pDDGPE->InVBlank();
    }
    else
    {
        // DDWAITVB_BLOCKBEGINEVENT:    // This flag is not currently supported. (Sets up an event to trigger when the vertical blank begins.)
    }

    lpwfvbd->ddRVal = DD_OK;

    DEBUGLEAVE( HalWaitForVerticalBlank );

    return DDHAL_DRIVER_HANDLED;
}

DWORD WINAPI HalGetScanLine(LPDDHAL_GETSCANLINEDATA lpgsld)
{
    RETAILMSG(DISP_ZONE_ENTER,(_T("[DDHAL] ++%s()\n\r"), _T(__FUNCTION__)));

    /*
    typedef struct _DDHAL_GETSCANLINEDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        DWORD dwScanLine;
        HRESULT ddRVal;
    } DDHAL_GETSCANLINEDATA;
    */

    S3C6410Disp *pDDGPE = (S3C6410Disp *)GetDDGPE();

    lpgsld->dwScanLine = pDDGPE->GetScanLine();
    lpgsld->ddRVal = DD_OK;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DDHAL] --%s()\n\r"), _T(__FUNCTION__)));

    return DDHAL_DRIVER_HANDLED;
}

