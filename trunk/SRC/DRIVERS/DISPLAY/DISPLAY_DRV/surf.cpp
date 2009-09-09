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

    surf.cpp

Abstract:

    surface allocation/manipulation/free routines

Functions:


Notes:


--*/

#include "precomp.h"

#define ALIGN(x, align)        (((x) + ((align) - 1)) & ~((align) - 1))

static DWORD dwSurfaceCount = 0;

SCODE
S3C6410Disp::AllocSurfacePACS(
                        GPESurf **ppSurf,
                        int width,
                        int height,
                        EGPEFormat format)
{
    /// try to allocate physically linear address
    *ppSurf = new PACSurf(width, height, format);
    
    if (*ppSurf == NULL)
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : PACSurface allocate Failed -> Try to allocate Normal GPE Surface\n\r")));
        return E_OUTOFMEMORY;
    }
    else if ((*ppSurf)->Buffer() == NULL)
    {
        delete *ppSurf;
        *ppSurf = NULL;

        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : PACSurface Buffer is NULL -> Try to allocate Normal GPE Surface\n\r")));
    }
    else        /// PAC Allocation succeeded.
    {
        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] AllocSurface() : PACSurf() Allocated in System Memory\n\r")));    
        return S_OK;
    }
    return E_FAIL;
}
//  This method is called for all normal surface allocations from ddgpe and gpe
SCODE
S3C6410Disp::AllocSurface(
                        GPESurf **ppSurf,
                        int width,
                        int height,
                        EGPEFormat format,
                        int surfaceFlags)
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[AS] %dx%d FMT:%d F:%08x\n\r"), width, height,  format, surfaceFlags));

    // This method is only for surface in system memory
    if (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : Can not allocate GPE_REQUIRE_VIDEO_MEMORY Surface in system memory\n\r")));
        return E_OUTOFMEMORY;
    }

    if(m_G2DControlArgs.HWOnOff && m_G2DControlArgs.UsePACSurf)
    {
        /// Only Support 16bpp, 24bpp, 32bpp
        if( format == gpe16Bpp || format == gpe24Bpp || format == gpe32Bpp || format == gpeDeviceCompatible)
        {
            if(m_G2DControlArgs.SetBltLimitSize)
            {
                if(width*height*(EGPEFormatToBpp[format] >> 3) > PAC_ALLOCATION_BOUNDARY)
                {
                    if(S_OK == AllocSurfacePACS(ppSurf, width, height, format))
                    {
                        return S_OK;
                    }
                }
            }
            else
            {
                if(S_OK == AllocSurfacePACS(ppSurf, width, height, format))
                {
                    return S_OK;
                }
            }
        }
    }
    /// if allocation is failed or boundary condition is not met, just create GPESurf in normal system memory that can be non-linear physically.

    // Allocate surface from system memory
    *ppSurf = new GPESurf(width, height, format);

    if (*ppSurf != NULL)
    {
        if (((*ppSurf)->Buffer()) == NULL)
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : Surface Buffer is NULL\n\r")));
            delete *ppSurf;
            return E_OUTOFMEMORY;
        }
        else
        {
            RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] AllocSurface() : GPESurf() Allocated in System Memory\n\r")));            
            return S_OK;
        }
    }

    RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : Surface allocate Failed\n\r")));
    return E_OUTOFMEMORY;
}


//  This method is used for DirectDraw enabled surfaces
SCODE
S3C6410Disp::AllocSurface(
                        DDGPESurf **ppSurf,
                        int width,
                        int height,
                        EGPEFormat format,
                        EDDGPEPixelFormat pixelFormat,
                        int surfaceFlags)
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s(%d, %d, %d, %d, 0x%08x)\n\r"), _T(__FUNCTION__), width, height, format, pixelFormat, surfaceFlags));

    unsigned int bpp;
    unsigned int stride;
    unsigned int align_width;

    if (pixelFormat == ddgpePixelFormat_I420 || pixelFormat == ddgpePixelFormat_YV12)
    {
        // in this case, stride can't be calculated. because of planar format (non-interleaved...)
        bpp = 12;
        align_width = ALIGN(width, 16);
    }
    else if (pixelFormat == ddgpePixelFormat_YVYU || pixelFormat == ddgpePixelFormat_VYUY)
    {
        bpp = 16;
        align_width = width;
    }
    else
    {
        bpp = EGPEFormatToBpp[format];
        align_width = width;
    }

    //DISPDRV_ERR((_T("[AS] %dx%d %dbpp FMT:%d F:%08x\n\r"), width, height, bpp, format, surfaceFlags));

    //--------------------------------------
    // Try to allocate surface from video memory
    //--------------------------------------

    // stride are all 32bit aligned for Video Memory
    stride = ((bpp * align_width + 31) >> 5) << 2;

    if ((surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
        || (surfaceFlags & GPE_BACK_BUFFER)
        //|| ((surfaceFlags & GPE_PREFER_VIDEO_MEMORY) && (format == m_pMode->format)))
        || (surfaceFlags & GPE_PREFER_VIDEO_MEMORY) && (format == m_pMode->format))
    {
        SCODE rv = AllocSurfaceVideo(ppSurf, width, height, stride, format, pixelFormat);
        if (rv == S_OK)
        {
            return S_OK;
        }
        else
        {
            if (surfaceFlags & (GPE_REQUIRE_VIDEO_MEMORY|GPE_BACK_BUFFER))
            {
                RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : AllocSurfaceVideo() failed\n\r")));
                return E_OUTOFMEMORY;
            }
        }
    }

    //--------------------------------------
    // Try to allocate surface from system memory
    //--------------------------------------

    // stride and surface size for system memory surfaces
    stride = ((bpp * width + 31) >> 5) << 2;
    unsigned int surface_size = stride*height;

    // Allocate from system memory
    *ppSurf = new DDGPESurf(width, height, stride, format, pixelFormat);

    if (*ppSurf)    // check we allocated bits succesfully
    {
        if (!((*ppSurf)->Buffer()))
        {
            delete *ppSurf;
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


SCODE
S3C6410Disp::AllocSurfaceVideo(
                        DDGPESurf **ppSurf,
                        int width,
                        int height,
                        int stride,
                        EGPEFormat format,
                        EDDGPEPixelFormat pixelFormat)
{
    // align frame buffer size with 4-word unit
    DWORD dwSize = ALIGN(stride * height, 16);

    // Try to allocate surface from video memory
    SurfaceHeap *pHeap = m_pVideoMemoryHeap->Alloc(dwSize);
    if (pHeap != NULL)
    {
        DWORD dwVideoMemoryOffset = pHeap->Address() - (DWORD)m_VideoMemoryVirtualBase;
        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] AllocSurfaceVideo() : Allocated PA = 0x%08x\n\r"), dwVideoMemoryOffset+m_VideoMemoryPhysicalBase));

        *ppSurf = new S3C6410Surf(width, height, dwVideoMemoryOffset, (PVOID)pHeap->Address(), stride, format, pixelFormat, pHeap);
        if (*ppSurf  == NULL)
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurfaceVideo() : Create S3C6410Surf() Failed\n\r")));

            pHeap->Free();

            return E_OUTOFMEMORY;
        }

        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s() Allocated in Video Memory\n\r"), _T(__FUNCTION__)));

        return S_OK;
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurfaceVideo() : SurfaceHeap Alloc() Failed\n\r")));

        *ppSurf = (DDGPESurf *)NULL;

        return E_OUTOFMEMORY;
    }
}


void S3C6410Disp::SetVisibleSurface(GPESurf *pSurf, BOOL bWaitForVBlank)
{
    S3C6410Surf *pDDSurf = (S3C6410Surf *)pSurf;

    if(pDDSurf->IsOverlay() == TRUE)
    {
        m_OverlayCtxt.pPrevSurface = m_OverlayCtxt.pSurface;        // Being Flipped Surface
        m_OverlayCtxt.pSurface = pDDSurf;
    }
    else
    {
        m_pVisibleSurface = pDDSurf;
    }

    EnterCriticalSection(&m_csDevice);

    DevSetVisibleSurface(pDDSurf, bWaitForVBlank);

    LeaveCriticalSection(&m_csDevice);
}

//-----------------------------------------------------------------------------

S3C6410Surf::S3C6410Surf(int width, int height, DWORD offset, VOID *pBits, int stride,
            EGPEFormat format, EDDGPEPixelFormat pixelFormat, SurfaceHeap *pHeap)
            : DDGPESurf(width, height, pBits, stride, format, pixelFormat)
{
    dwSurfaceCount++;

    m_fInVideoMemory = TRUE;
    m_nOffsetInVideoMemory = offset;
    m_pSurfHeap = pHeap;

    if (pixelFormat == ddgpePixelFormat_I420)       // 3Plane
    {
        m_uiOffsetCb = width*height;
        m_uiOffsetCr = m_uiOffsetCb+width*height/4;
    }
    else if (pixelFormat == ddgpePixelFormat_YV12)  // 3Plane
    {
        m_uiOffsetCr = width*height;
        m_uiOffsetCb = m_uiOffsetCr+width*height/4;
    }
    else
    {
        m_uiOffsetCr = 0;
        m_uiOffsetCb = 0;
    }

    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV:INF] %S() : @ 0x%08x, %d\n\r"), _T(__FUNCTION__), m_nOffsetInVideoMemory, dwSurfaceCount));
}


S3C6410Surf::~S3C6410Surf()
{
    dwSurfaceCount--;
    if(m_pSurfHeap)
    {
        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV:INF] %s() : Heap 0x%08x Addr:0x%x, Avail:%d, Size:%d\n\r"), 
        m_pSurfHeap, m_pSurfHeap->Address(), m_pSurfHeap->Available(), m_pSurfHeap->Size()));
        
        m_pSurfHeap->Free();
        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV:INF] %s() : @ 0x%08x, %d\n\r"), _T(__FUNCTION__), m_nOffsetInVideoMemory, dwSurfaceCount));
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("ERROR, Invalid SurfaceHeap Address")));
    }
}

/**
*    @class    PACSurf
*    @desc    This Surface will try to allocate physically linear address
*
**/
/**
*    @fn        PACSurf::PACSurf
*    @brief    try to allocate memory region that is physically linear
*    @param    GPESurf **ppSurf, INT width, INT height, EGPEFormat format, int surfaceFlags
*    @sa        GPESurf
*    @note    This Surface format is compatible to GPESurf
**/
PACSurf::PACSurf(int width, int height, EGPEFormat format)
{
    RETAILMSG(DISP_ZONE_CREATE, (_T("[DISPDRV] PACSurf Constructor(%d, %d, %d)\n\r"), width, height, format));    

    // Even though "width" and "height" are int's, they must be positive.
    ASSERT(width > 0);
    ASSERT(height > 0);

    DWORD m_pPhysAddr;

    memset( &m_Format, 0, sizeof ( m_Format ) );

    m_pVirtAddr            = NULL;
    m_nStrideBytes         = 0;
    m_eFormat              = gpeUndefined;
    m_fInVideoMemory       = 0;
    m_fInUserMemory        = FALSE;
    m_fOwnsBuffer          = 0;
    m_nWidth               = 0;
    m_nHeight              = 0;
    m_nOffsetInVideoMemory = 0;
    m_iRotate              = DMDO_0;
    m_ScreenWidth          = 0;
    m_ScreenHeight         = 0;
    m_BytesPixel           = 0;
    m_nHandle              = NULL;
//    m_fPLAllocated        = 0;


    if (width > 0 && height > 0)
    {
        m_nWidth               = width;
        m_nHeight              = height;
        m_eFormat              = format;
        m_nStrideBytes         = ( (EGPEFormatToBpp[ format ] * width + 7 )/ 8 + 3 ) & ~3L;
        m_pVirtAddr  = (ADDRESS) AllocPhysMem(  m_nStrideBytes * height, PAGE_READWRITE, 0, 0,&m_pPhysAddr);
        if(m_pVirtAddr != NULL)
        {
//                m_fPLAllocated = 1;
//                m_fInVideoMemory = 1;
//                RETAILMSG(DISP_ZONE_2D,(TEXT("\nPAC Surf PA Base : 0x%x VA Base : 0x%x STRIDE : %d"), m_pPhysAddr, m_pVirtAddr, m_nStrideBytes));
        }
        else
        {
//            m_fPLAllocated = 0;
                RETAILMSG(DISP_ZONE_WARNING,(TEXT("\nPAC Surf PA Base : 0x%x VA Base : 0x%x STRIDE : %d  new unsigned char"), m_pPhysAddr, m_pVirtAddr, m_nStrideBytes));        
                m_pVirtAddr = (ADDRESS) new unsigned char[ m_nStrideBytes * height ];
        }
        m_fOwnsBuffer          = 1;
        m_BytesPixel           = EGPEFormatToBpp[m_eFormat] >> 3;
    }
}

PACSurf::~PACSurf()
{
    if( m_fOwnsBuffer )
    {
//        if(m_fPLAllocated)
//        {
          if(m_pVirtAddr)
          {
                if( !FreePhysMem((LPVOID)m_pVirtAddr) )
                {
                    RETAILMSG(DISP_ZONE_ERROR,(TEXT("PACSurface deallocation is failed\r\n")));
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("PACSurface dealloc is trying for non-contigious\r\n")));        
                    delete [] (void *)m_pVirtAddr;
                    m_pVirtAddr = NULL;
                    m_fOwnsBuffer = 0;
                }
                else
                {
                    m_pVirtAddr = NULL;
//                   m_fPLAllocated = 0;
                    m_fOwnsBuffer = 0;
                    RETAILMSG(DISP_ZONE_CREATE,(TEXT("PACSurface deallocation is succeeded\r\n")));
                }
          }

//        }
//        else if( m_pVirtAddr )
//        {
//        RETAILMSG(DISP_ZONE_WARNING,(TEXT("PACSurface dealloc is trying for non-contigious\r\n")));        
//            delete [] (void *)m_pVirtAddr;
//            m_pVirtAddr = NULL;
//            m_fOwnsBuffer = 0;
//        }
    }
}


//-----------------------------------------------------------------------------

#if    0
//------------------------------------------------------------------------------
//
//  Method:  WriteBack
//
//  Flush surface memory in cache.
//
VOID S3C6410Surf::WriteBack()
{
        ASSERT(m_pSurface != NULL);

    RETAILMSG(DBGLCD, (TEXT("S3C6410Surf::WriteBack\n")));

        if (m_pSurface != NULL)
        CacheRangeFlush((VOID*)m_pSurface, m_nStrideBytes * m_nHeight, CACHE_SYNC_WRITEBACK);
}

//------------------------------------------------------------------------------
//
//  Method:  Discard
//
//  Flush and invalidate surface memory in cache.
//
VOID S3C6410Surf::Discard()
{
        ASSERT(m_pSurface != NULL);
        RETAILMSG(DBGLCD, (TEXT("S3C6410Surf::Discard\n")));
        if (m_pSurface != NULL)
        CacheRangeFlush((VOID*)m_pSurface, m_nStrideBytes * m_nHeight, CACHE_SYNC_DISCARD);
}
#endif

