
#include <windows.h>
#include <winddi.h>
#include <gpe.h>

#include "ebook2_disp.h"
#include "dispdrvr.h"


INSTANTIATE_GPE_ZONES(0x3,"MGDI Driver","unused1","unused2")	 /* Start with Errors, warnings, and temporary messages */

BOOL APIENTRY GPEEnableDriver(          // This gets around problems exporting from .lib
	ULONG          iEngineVersion,
	ULONG          cj,
	DRVENABLEDATA *pded,
	PENGCALLBACKS  pEngCallbacks);
BOOL APIENTRY DrvEnableDriver(
	ULONG          iEngineVersion,
	ULONG          cj,
	DRVENABLEDATA *pded,
	PENGCALLBACKS  pEngCallbacks)
{
	return GPEEnableDriver( iEngineVersion, cj, pded, pEngCallbacks );
}


static GPE *pGPE = (GPE *)NULL;
// Main entry point for a GPE-compliant driver
GPE *GetGPE()
{
	if( !pGPE )
		pGPE = new EpdDisp();
	return pGPE;
}


EpdDisp::EpdDisp()
{
	DEBUGMSG( GPE_ZONE_INIT,(TEXT("EpdDisp::EpdDisp\r\n")));

	DispDrvrInitialize();	// call entry point to 2bpp GDI driver
	
	// When this DispDrvrInitialize returns, DispDrvrPhysicalFrameBuffer contains
	// the physical address of the screen if it is in 2bpp DIB format. - Alternatively it
	// is NULL - in which case this is a "dirty-rect" driver

	m_ModeInfo.modeId = 0;
	m_ModeInfo.width = m_nScreenWidth = DispDrvr_cxScreen;
	m_ModeInfo.height = m_nScreenHeight = DispDrvr_cyScreen;
	m_ModeInfo.Bpp = DispDrvr_bpp;
	m_ModeInfo.frequency = 60;		// not too important
	m_ModeInfo.format = DispDrvr_format;

	m_pMode = &m_ModeInfo;

	if (DispDrvrPhysicalFrameBuffer == NULL)
	{
		// it is a "dirty-rect" driver - we create a system memory bitmap to represent
		// the screen and refresh rectangles of this to the screen as they are altered
		m_pPrimarySurface = new GPESurf( m_nScreenWidth, m_nScreenHeight, DispDrvr_format );
		if (!m_pPrimarySurface)
		{
			RETAILMSG(1, (TEXT("EpdDisp::EpdDisp: Error allocating GPESurf.\r\n")));
			return;
		}

		DispDrvrSetDibBuffer( m_pPrimarySurface->Buffer() );
	}
	else
	{
		RETAILMSG(1, (TEXT("EpdDisp::EpdDisp: Error (DispDrvrPhysicalFrameBuffer != NULL)\r\n")));
	}
}

SCODE EpdDisp::SetMode( int modeId, HPALETTE *pPalette )
{
	if( modeId != 0 )
		return E_INVALIDARG;

	// Here, we use EngCreatePalette to create a palette that that MGDI will use as a
	// stock palette

	DEBUGMSG(1,(TEXT("SetMode - creating stock palette\r\n")));

	if( pPalette )
	{
		*pPalette = EngCreatePalette(
								PAL_INDEXED,
								DispDrvr_palSize,
								(ULONG *)DispDrvr_palette,
								0,
								0,
								0
							);
	}

	DEBUGMSG(1,(TEXT("SetMode done\r\n")));

	return S_OK;				// Mode is inherently set
}

SCODE EpdDisp::GetModeInfo(
	GPEMode *pMode,
	int modeNo
)
{
	if( modeNo != 0 )
		return E_INVALIDARG;

	*pMode = m_ModeInfo;

	return S_OK;
}

int EpdDisp::NumModes()
{
	return 1;
}

SCODE EpdDisp::SetPointerShape(
	GPESurf *pMask,
	GPESurf *pColorSurf,
	int xHot,
	int yHot,
	int cx,
	int cy )
{
	return S_OK;
}

SCODE EpdDisp::MovePointer(
	int x,
	int y )
{
	return S_OK;
}

void EpdDisp::WaitForNotBusy()
{
	return;
}

int EpdDisp::IsBusy()
{
	return 0;	// Never busy as there is no acceleration
}

void EpdDisp::GetPhysicalVideoMemory
(
	unsigned long *pPhysicalMemoryBase,
	unsigned long *pVideoMemorySize
)
{
	*pPhysicalMemoryBase = (unsigned long)DispDrvrPhysicalFrameBuffer;
	*pVideoMemorySize = DispDrvr_fbSize;
}


SCODE EpdDisp::AllocSurface(
	GPESurf **ppSurf,
	int width,
	int height,
	EGPEFormat format,
	int surfaceFlags )
{
	if( surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY )
		return E_OUTOFMEMORY;	// Can't allocate video-memory surfaces in the EpdDisp environment
	// Allocate from system memory
	DEBUGMSG(GPE_ZONE_CREATE,(TEXT("Creating a GPESurf in system memory. EGPEFormat = %d\r\n"), (int)format ));
	*ppSurf = new GPESurf( width, height, format );
	if( *ppSurf )
	{
		// check we allocated bits succesfully
		if( !((*ppSurf)->Buffer()) )
			delete *ppSurf;	// and then return E_OUTOFMEMORY
		else
			return S_OK;
	}
	return E_OUTOFMEMORY;
}

SCODE EpdDisp::WrappedEmulatedLine( GPELineParms *pParms )
{
	SCODE sc = EmulatedLine( pParms );	// Draw to the backup framebuffer

	if( FAILED(sc) )
		return sc;

	// Now, calculate the dirty-rect to refresh to the actual hardware
	RECT bounds;

	int N_plus_1;			// Minor length of bounding rect + 1

	if( pParms->dN )	// The line has a diagonal component (we'll refresh the bounding rect)
		N_plus_1 = 1 + ( ( pParms->cPels * pParms->dN ) / pParms->dM );
	else
		N_plus_1 = 1;

	switch( pParms->iDir )
	{
		case 0:
			bounds.left = pParms->xStart;
			bounds.top = pParms->yStart;
			bounds.right = pParms->xStart + pParms->cPels + 1;
			bounds.bottom = bounds.top + N_plus_1;
			break;
		case 1:
			bounds.left = pParms->xStart;
			bounds.top = pParms->yStart;
			bounds.bottom = pParms->yStart + pParms->cPels + 1;
			bounds.right = bounds.left + N_plus_1;
			break;
		case 2:
			bounds.right = pParms->xStart + 1;
			bounds.top = pParms->yStart;
			bounds.bottom = pParms->yStart + pParms->cPels + 1;
			bounds.left = bounds.right - N_plus_1;
			break;
		case 3:
			bounds.right = pParms->xStart + 1;
			bounds.top = pParms->yStart;
			bounds.left = pParms->xStart - pParms->cPels;
			bounds.bottom = bounds.top + N_plus_1;
			break;
		case 4:
			bounds.right = pParms->xStart + 1;
			bounds.bottom = pParms->yStart + 1;
			bounds.left = pParms->xStart - pParms->cPels;
			bounds.top = bounds.bottom - N_plus_1;
			break;
		case 5:
			bounds.right = pParms->xStart + 1;
			bounds.bottom = pParms->yStart + 1;
			bounds.top = pParms->yStart - pParms->cPels;
			bounds.left = bounds.right - N_plus_1;
			break;
		case 6:
			bounds.left = pParms->xStart;
			bounds.bottom = pParms->yStart + 1;
			bounds.top = pParms->yStart - pParms->cPels;
			bounds.right = bounds.left + N_plus_1;
			break;
		case 7:
			bounds.left = pParms->xStart;
			bounds.bottom = pParms->yStart + 1;
			bounds.right = pParms->xStart + pParms->cPels + 1;
			bounds.top = bounds.bottom - N_plus_1;
			break;
		default:
			DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid direction: %d\r\n"),pParms->iDir));
			return E_INVALIDARG;
	}

	DispDrvrDirtyRectDump( (LPRECT)&bounds );

	return sc;
}
   
			  
SCODE EpdDisp::Line(
	GPELineParms *pLineParms,
	EGPEPhase phase )
{
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("EpdDisp::Line\r\n")));

	if( phase == gpeSingle || phase == gpePrepare )
	{
		if( ( pLineParms->pDst != m_pPrimarySurface ) || ( DispDrvrPhysicalFrameBuffer != NULL ) )
			pLineParms->pLine = &GPE::EmulatedLine;
		else
			pLineParms->pLine = (SCODE (GPE::*)(struct GPELineParms *))&EpdDisp::WrappedEmulatedLine;
	}
	return S_OK;
}

#undef SWAP
#define SWAP(type,a,b) { type tmp; tmp=a; a=b; b=tmp; }

SCODE EpdDisp::WrappedEmulatedBlt( GPEBltParms *pParms )
{
	SCODE sc = EmulatedBlt( pParms );	// Draw to the backup framebuffer

	if( FAILED(sc) )
		return sc;

	// Now, calculate the dirty-rect to refresh to the actual hardware
	RECT bounds;

	bounds.left = pParms->prclDst->left;
	bounds.top = pParms->prclDst->top;
	bounds.right = pParms->prclDst->right;
	bounds.bottom = pParms->prclDst->bottom;

	if( bounds.left > bounds.right )
	{
		SWAP( int, bounds.left, bounds.right )
	}
	if( bounds.top > bounds.bottom )
	{
		SWAP( int, bounds.top, bounds.bottom )
	}

	DispDrvrDirtyRectDump( (LPRECT)&bounds );

	return sc;
}


SCODE EpdDisp::BltPrepare(
	GPEBltParms *pBltParms )
{
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("EpdDisp::BltPrepare\r\n")));

	if( ( pBltParms->pDst != m_pPrimarySurface ) || ( DispDrvrPhysicalFrameBuffer != NULL ) )
		pBltParms->pBlt = &GPE::EmulatedBlt;
	else
		pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *))&EpdDisp::WrappedEmulatedBlt;

	return S_OK;
}

// This function would be used to undo the setting of clip registers etc
SCODE EpdDisp::BltComplete( GPEBltParms *pBltParms )
{
	return S_OK;
}

int EpdDisp::InVBlank()
{
	return 0;
}

SCODE EpdDisp::SetPalette(
	const PALETTEENTRY *src,
	unsigned short firstEntry,
	unsigned short numEntries
)
{
	return S_OK;
}

VOID EpdDisp::PowerHandler(BOOL bOff)
{
	DispDrvrPowerHandler(bOff);
}

ULONG EpdDisp::DrvEscape(
	SURFOBJ *pso, ULONG iEsc,
	ULONG cjIn, PVOID pvIn,
	ULONG cjOut, PVOID pvOut)
{
	return DispDrvrDrvEscape(pso, iEsc, cjIn, pvIn, cjOut, pvOut);
}


void RegisterDDHALAPI()
{
	;	// No DDHAL support
}

ulong BitMasks[] = { 0x0001,0x0002,0x0000 };

ULONG *APIENTRY DrvGetMasks(
	DHPDEV dhpdev)
{
	return BitMasks;
}

