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

    halsurf.cpp

Abstract:

    The Implementation HAL function to support DirectDraw

Functions:

    HalCreateSurface, ...

Notes:

--*/

#include "precomp.h"

DWORD
WINAPI
HalCreateSurface(LPDDHAL_CREATESURFACEDATA lpcsd)
{
    DEBUGENTER( HalCreateSurface );
    /*
    typedef struct _DDHAL_CREATESURFACEDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDSURFACEDESC lpDDSurfaceDesc;
        LPDDRAWI_DDRAWSURFACE_LCL lplpSList;
        DWORD dwSCnt;
        HRESULT ddRVal;
    } DDHAL_CREATESURFACEDATA;
    */

    DWORD dwCaps = lpcsd->lpDDSurfaceDesc->ddsCaps.dwCaps;
    DWORD dwFlags = lpcsd->lpDDSurfaceDesc->dwFlags;

    // Handle Overlay Surface
    if (dwCaps & DDSCAPS_OVERLAY)
    {
        EGPEFormat format;
        EDDGPEPixelFormat pixelFormat;
        S3C6410Disp *pDDGPE = (S3C6410Disp *)GetDDGPE();

        lpcsd->ddRVal = pDDGPE->DetectPixelFormat(dwCaps, &lpcsd->lpDDSurfaceDesc->ddpfPixelFormat, &format, &pixelFormat);
        RETAILMSG(TRUE,(TEXT("CreateSurface:%d\n"),lpcsd->ddRVal));
        if (FAILED(lpcsd->ddRVal))
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalCreateSurface() : Unsupported format\n\r")));

            return DDHAL_DRIVER_HANDLED;
        }

        // Create Overlay Surface with DDGPECreateSurface() function
        return DDGPECreateSurface(lpcsd);
    }
    else
    {
        // Pass to Non-overlay surface to DDGPECreateSurface() function
        return DDGPECreateSurface(lpcsd);
    }
}

DWORD WINAPI HalCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpccsd)
{
    DEBUGENTER( HalCanCreateSurface );
    /*
    typedef struct _DDHAL_CANCREATESURFACEDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDSURFACEDESC lpDDSurfaceDesc;
        DWORD bIsDifferentPixelFormat;
        HRESULT ddRVal;
    } DDHAL_CANCREATESURFACEDATA;

    */

    DDPIXELFORMAT *pddpf = &lpccsd->lpDDSurfaceDesc->ddpfPixelFormat;
    DWORD dwCaps = lpccsd->lpDDSurfaceDesc->ddsCaps.dwCaps;
    DWORD dwWidth = lpccsd->lpDDSurfaceDesc->dwWidth;
    DWORD dwHeight = lpccsd->lpDDSurfaceDesc->dwHeight;

    RETAILMSG(DISP_ZONE_TEMP, (_T("HalCanCreateSurface. dwCaps : 0x%x, dwWidth:%d, dwHeight:%d\n"), dwCaps, dwWidth, dwHeight));    

    S3C6410Disp *pDDGPE = (S3C6410Disp *)GetDDGPE();

    // We do Not allow Primary Surface in System Memory
    if ((dwCaps & DDSCAPS_PRIMARYSURFACE) && (dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalCanCreateSurface() : Can Not create Primary Surface in System Memory\n\r")));
        goto CannotCreate;
    }

    if ((dwCaps & DDSCAPS_OVERLAY) && (dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalCanCreateSurface() : Can Not create Overlay Surface in System Memory\n\r")));
        goto CannotCreate;
    }

    if (dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("DiffPixel:%d\n"), lpccsd->bIsDifferentPixelFormat));
        if (lpccsd->bIsDifferentPixelFormat)
        {
            goto CannotCreate;
        }
        else
        {
            goto CanCreate;
        }
    }
    else if (dwCaps & DDSCAPS_OVERLAY)
    {
        EGPEFormat Format;
        EDDGPEPixelFormat PixelFormat;

        lpccsd->ddRVal = pDDGPE->DetectPixelFormat(dwCaps, pddpf, &Format, &PixelFormat);
        if (FAILED(lpccsd->ddRVal))
        {
            goto CannotCreate;
        }
        else
        {
            RETAILMSG(TRUE, (_T("PixelFormat :%d\n"), PixelFormat));
            switch(PixelFormat)
            {
            case ddgpePixelFormat_565:
            //case ddgpePixelFormat_8880:    // FIMD can not support Packed RGB888
            case ddgpePixelFormat_8888:
            case ddgpePixelFormat_I420:
            case ddgpePixelFormat_YV12:
            case ddgpePixelFormat_YUYV:
            case ddgpePixelFormat_YUY2:
            case ddgpePixelFormat_UYVY:
             case ddgpePixelFormat_YVYU:
            case ddgpePixelFormat_VYUY:
                goto CanCreate;
                break;
            default:
                goto CannotCreate;
                break;
            }
        }
    }
    else        // Non Primary, Non Overlay Surface (Maybe Offscreen Surface)
    {
        return DDGPECanCreateSurface(lpccsd);
    }

CanCreate:

    RETAILMSG(DISP_ZONE_ENTER,(_T("[DDHAL] HalCanCreateSurface() OK\n\r")));
    lpccsd->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;

CannotCreate:

    RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalCanCreateSurface() : Unsupported Surface\n\r")));
    lpccsd->ddRVal = DDERR_UNSUPPORTEDFORMAT;

    return DDHAL_DRIVER_HANDLED;
}

//////////////////////////// DDHAL_DDSURFACECALLBACKS ////////////////////////////

DWORD
WINAPI
HalFlip(LPDDHAL_FLIPDATA lpfd)
{
    DEBUGENTER( HalFlip );
    /*
    typedef struct _DDHAL_FLIPDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDRAWI_DDRAWSURFACE_LCL lpSurfCurr;
        LPDDRAWI_DDRAWSURFACE_LCL lpSurfTarg;
        DWORD dwFlags;
        HRESULT ddRVal;
    } DDHAL_FLIPDATA;
    */

    S3C6410Disp *pDDGPE = (S3C6410Disp *)GetDDGPE();
    DWORD dwFlags = lpfd->dwFlags;

    if (dwFlags & (DDFLIP_INTERVAL1|DDFLIP_INTERVAL2|DDFLIP_INTERVAL4))
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalFlip() : DDFLIP_INTERVAL is not supported\n\r")));
        lpfd->ddRVal = DDERR_UNSUPPORTED;
    }
    else
    {
        if (dwFlags & DDFLIP_WAITNOTBUSY)
        {
            // Our H/W always not busy.. This will be skipped
            while(((S3C6410Disp *)GetDDGPE())->IsBusy());
        }

        DDGPESurf* surfTarg = DDGPESurf::GetDDGPESurf(lpfd->lpSurfTarg);

        if (dwFlags & DDFLIP_WAITVSYNC)
        {
            pDDGPE->SetVisibleSurface(surfTarg, TRUE);
        }
        else
        {
            pDDGPE->SetVisibleSurface(surfTarg, FALSE);
        }

        lpfd->ddRVal = DD_OK;
    }

    DEBUGLEAVE( HalFlip );

    return DDHAL_DRIVER_HANDLED;
}

DWORD WINAPI HalGetBltStatus( LPDDHAL_GETBLTSTATUSDATA lpgbsd )
{
    DEBUGENTER( HalGetBltStatus );
    /*
    typedef struct _DDHAL_GETBLTSTATUSDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSurface;
        DWORD dwFlags;
        HRESULT ddRVal;
    } DDHAL_GETBLTSTATUSDATA;
    */

    // Implementation
    lpgbsd->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;
}

DWORD WINAPI HalGetFlipStatus( LPDDHAL_GETFLIPSTATUSDATA lpgfsd)
{
    DEBUGENTER( HalGetFlipStatus );
    /*
    typedef struct _DDHAL_GETFLIPSTATUSDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSurface;
        DWORD dwFlags;
        HRESULT ddRVal;
    } DDHAL_GETFLIPSTATUSDATA;
    */

    lpgfsd->ddRVal = DD_OK;

    // NOTE: DDGBS_CANFLIP always return DD_OK
    // Actually second flip request in same display frame is blocked to next frame

    return DDHAL_DRIVER_HANDLED;
}

DWORD WINAPI HalUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA lpuod)
{
    DEBUGENTER( HalUpdateOverlay );
    /*
    typedef struct _DDHAL_UPDATEOVERLAYDATA {
          LPDDRAWI_DIRECTDRAW_GBL lpDD;
          LPDDRAWI_DDRAWSURFACE_LCL lpDDDestSurface;
          RECT rDest;
          LPDDRAWI_DDRAWSURFACE_LCL lpDDSrcSurface;
          RECT rSrc;
          DWORD dwFlags;
          DDOVERLAYFX overlayFX;
          HRESULT ddRVal;
    } DDHAL_UPDATEOVERLAYDATA;
    */

    S3C6410Disp    *pDDGPE;
    S3C6410Surf    *pSrcSurf;
    S3C6410Surf    *pDestSurf;
    LPDDRAWI_DDRAWSURFACE_LCL lpSrcLCL;
    LPDDRAWI_DDRAWSURFACE_LCL lpDestLCL;

    BOOL bEnableOverlay = FALSE;

    /* 'Source' is the overlay surface, 'destination' is the surface to
    * be overlayed:
    */

    lpSrcLCL = lpuod->lpDDSrcSurface;
    lpDestLCL = lpuod->lpDDDestSurface;

    pDDGPE = (S3C6410Disp *)GetDDGPE();
    pSrcSurf = (S3C6410Surf *)DDGPESurf::GetDDGPESurf(lpSrcLCL);
    pDestSurf = (S3C6410Surf *)DDGPESurf::GetDDGPESurf(lpDestLCL);

    if (lpuod->dwFlags & DDOVER_HIDE)
    {
        // If overlay surface is valid, Turn off overlay
        if (pSrcSurf->OffsetInVideoMemory() != NULL)
        {
            if ( (pSrcSurf == pDDGPE->GetCurrentOverlaySurf())
                || (pSrcSurf == pDDGPE->GetPreviousOverlaySurf()) )
            {
                pDDGPE->OverlayDisable();
            }

            lpuod->ddRVal = DD_OK;
        }
        else
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalUpdateOverlay() : pSrcSurf->OffsetInVideoMemory() = NULL\n\r")));
            lpuod->ddRVal = DDERR_INVALIDPARAMS;
        }

        return (DDHAL_DRIVER_HANDLED);
    }
    else if (lpuod->dwFlags & DDOVER_SHOW)
    {
        if (pSrcSurf->OffsetInVideoMemory() != NULL)
        {
            if ( (pSrcSurf != pDDGPE->GetCurrentOverlaySurf())
                && (pSrcSurf != pDDGPE->GetPreviousOverlaySurf())
                && (pDDGPE->GetCurrentOverlaySurf() != NULL))
            {
                // Some other overlay surface is already visible:
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalUpdateOverlay() : Overlay is already in use by another surface\n\r")));

                lpuod->ddRVal = DDERR_OUTOFCAPS;

                return (DDHAL_DRIVER_HANDLED);
            }
            else
            {
                // Initialize Overlay
                if (pDDGPE->OverlayInitialize(pSrcSurf, &lpuod->rSrc, &lpuod->rDest) == FALSE)
                {
                    RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalUpdateOverlay() : OverlayInitialize() Failed\n\r")));

                    lpuod->ddRVal = DDERR_OUTOFCAPS;

                    return (DDHAL_DRIVER_HANDLED);
                }

                // Enable Overlay below... after set up blending
                bEnableOverlay = TRUE;
            }
        }
        else
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalUpdateOverlay() : pSrcSurf->OffsetInVideoMemory() = NULL\n\r")));

            lpuod->ddRVal = DDERR_INVALIDPARAMS;
            return (DDHAL_DRIVER_HANDLED);
        }
    }
    else
    {
        // If overlay surface is not visiable,  Nothing to do
        lpuod->ddRVal = DD_OK;

        return (DDHAL_DRIVER_HANDLED);
    }

    if ((lpuod->dwFlags & (DDOVER_KEYSRC|DDOVER_KEYSRCOVERRIDE|DDOVER_KEYDEST|DDOVER_KEYDESTOVERRIDE))
        && (lpuod->dwFlags & (DDOVER_ALPHASRC|DDOVER_ALPHACONSTOVERRIDE)))
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalUpdateOverlay() : Driver Not Support ColorKey & Alpha at the same time (dwFlags = 0x%08x)\n\r"), lpuod->dwFlags));
    }

    // Source Color Key
    if ((lpuod->dwFlags & (DDOVER_KEYSRCOVERRIDE | DDOVER_KEYSRC))
        ||(lpSrcLCL->dwFlags & DDRAWISURF_HASCKEYSRCOVERLAY)
        )
    {
        DWORD dwColorKey;

        if (lpuod->dwFlags & DDOVER_KEYSRCOVERRIDE)
        {
            dwColorKey = lpuod->overlayFX.dckSrcColorkey.dwColorSpaceLowValue;
        }
        else
        {
            dwColorKey = lpSrcLCL->ddckCKSrcOverlay.dwColorSpaceLowValue;
        }

        pDDGPE->OverlaySetColorKey(TRUE, pSrcSurf->PixelFormat(), dwColorKey);
    }
    // Destination Color Key
    else if ((lpuod->dwFlags & (DDOVER_KEYDESTOVERRIDE | DDOVER_KEYDEST))
        ||(lpSrcLCL->dwFlags & DDRAWISURF_HASCKEYDESTOVERLAY)
        )
    {
        DWORD dwColorKey;

        if (lpuod->dwFlags & DDOVER_KEYDESTOVERRIDE)
        {
            dwColorKey = lpuod->overlayFX.dckDestColorkey.dwColorSpaceLowValue;
        }
        else if(lpuod->dwFlags & DDOVER_KEYDEST)
        {
            dwColorKey = lpDestLCL->ddckCKDestOverlay.dwColorSpaceLowValue;
        }
        else
        {
            dwColorKey = lpSrcLCL->ddckCKDestOverlay.dwColorSpaceLowValue;
        }

        pDDGPE->OverlaySetColorKey(FALSE, pDestSurf->PixelFormat(), dwColorKey);
    }
    // Alpha Blending
    else if ((lpuod->dwFlags & DDOVER_ALPHASRC)
        || (lpuod->dwFlags & DDOVER_ALPHACONSTOVERRIDE))
    {
        if (lpuod->dwFlags & DDOVER_ALPHACONSTOVERRIDE)    // Per Plane Alpha Blending
        {
            pDDGPE->OverlaySetAlpha(FALSE, lpuod->overlayFX.dwAlphaConst);
        }
        else
        {
            pDDGPE->OverlaySetAlpha(TRUE, lpuod->overlayFX.dwAlphaConst);
        }
    }
    // No Blending Effect
    else
    {
        pDDGPE->OverlayBlendDisable();
    }

    // Enable Overlay after set up blending
    if (bEnableOverlay) pDDGPE->OverlayEnable();

    lpuod->ddRVal = DD_OK;

    return (DDHAL_DRIVER_HANDLED);
}

DWORD WINAPI HalSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA lpsopd)
{
    DEBUGENTER( HalSetOverlayPosition );
    /*
    typedef struct _DDHAL_SETOVERLAYPOSITIONDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSrcSurface;
        LPDDRAWI_DDRAWSURFACE_LCL lpDDDestSurface;
        LONG lXPos;
        LONG lYPos;
        HRESULT ddRVal;
    } DDHAL_SETOVERLAYPOSITIONDATA;
    */

    S3C6410Disp    *pDDGPE;

    pDDGPE = (S3C6410Disp *)GetDDGPE();

    pDDGPE->OverlaySetPosition((unsigned int)lpsopd->lXPos, (unsigned int)lpsopd->lYPos);
    lpsopd->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;
}

DWORD WINAPI HalSetColorKey(LPDDHAL_SETCOLORKEYDATA lpdsckd)
{
    DEBUGENTER(HalSetColorKey);
    /*
    typedef struct _DDHAL_SETCOLORKEYDATA
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDD;
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSurface;
        DWORD dwFlags;
        DDCOLORKEY ckNew;
        HRESULT ddRVal;
    } DDHAL_SETCOLORKEYDATA;
    */

    DDGPESurf* pSurf = DDGPESurf::GetDDGPESurf(lpdsckd->lpDDSurface);
    if (pSurf != NULL)
    {
        if (lpdsckd->dwFlags == DDCKEY_COLORSPACE)
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalSetColorKey() : Color Space CKEY is Not Supported\n\r")));
            lpdsckd->ddRVal = DDERR_NOCOLORKEYHW;
        }
        else if ((lpdsckd->dwFlags == DDCKEY_SRCBLT)
            || (lpdsckd->dwFlags == DDCKEY_DESTBLT))
        {
            // NOTE: Currently our driver do not support HW CKEY BLT but CETK DDraw Test #210, #310 trying to use
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalSetColorKey() : Color Space CKEY or CKEY BLT is Not Supported\n\r")));

            pSurf->SetColorKeyLow(lpdsckd->ckNew.dwColorSpaceLowValue);
            pSurf->SetColorKeyHigh(lpdsckd->ckNew.dwColorSpaceHighValue);
            lpdsckd->ddRVal = DD_OK;
        }
        else     if ((lpdsckd->dwFlags == DDCKEY_SRCOVERLAY)
            || (lpdsckd->dwFlags == DDCKEY_DESTOVERLAY))
        {
            pSurf->SetColorKeyLow(lpdsckd->ckNew.dwColorSpaceLowValue);
            pSurf->SetColorKeyHigh(lpdsckd->ckNew.dwColorSpaceHighValue);
            lpdsckd->ddRVal = DD_OK;
        }
        else
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DDHAL:ERR] HalSetColorKey() : Invalid dwFlags = 0x%08x\n\r"), lpdsckd->dwFlags));
            lpdsckd->ddRVal = DDERR_INVALIDOBJECT;
        }
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DDHAL:ERR] HalSetColorKey() : Surface Object is Null\n\r")));
        lpdsckd->ddRVal = DDERR_INVALIDOBJECT;
    }

    DEBUGLEAVE(HalSetColorKey);

    return DDHAL_DRIVER_HANDLED;
}
