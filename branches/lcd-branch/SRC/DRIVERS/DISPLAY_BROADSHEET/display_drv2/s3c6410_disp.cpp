
#include <windows.h>
#include <types.h>
#include <winddi.h>
#include <emul.h>
#include <ceddk.h>
#include <ddrawi.h>
#include <ddgpe.h>
#include <ddhfuncs.h>
#include "dispperf.h"

#include "s3c6410_disp.h"
#include "..\broadsheet_lib\dispdrvr.h"


INSTANTIATE_GPE_ZONES(0x3,"MGDI Driver","unused1","unused2")    // Start with errors and warnings

// This prototype avoids problems exporting from .lib
BOOL APIENTRY GPEEnableDriver(ULONG           engineVersion,
							  ULONG           cj,
							  DRVENABLEDATA * data,
							  PENGCALLBACKS   engineCallbacks);
BOOL APIENTRY DrvEnableDriver(ULONG           engineVersion,
							  ULONG           cj,
							  DRVENABLEDATA * data,
							  PENGCALLBACKS   engineCallbacks)
{
	return GPEEnableDriver(engineVersion, cj, data, engineCallbacks);
}

DDGPE *gGPE = (DDGPE *)NULL;
// Main entry point for a GPE-compliant driver
GPE *GetGPE()
{
	if (!gGPE)
		gGPE = new S3C6410Disp();
	return gGPE;
}


S3C6410Disp::S3C6410Disp()
{
	ULONG fbSize;
	DMA_ADAPTER_OBJECT Adapter={0,};

	DEBUGMSG(GPE_ZONE_INIT,(TEXT("S3C6410Disp::S3C6410Disp\r\n")));

	DispDrvrInitialize();

	m_cxPhysicalScreen = m_nScreenWidth = DispDrvr_cxScreen;
	m_cyPhysicalScreen = m_nScreenHeight = DispDrvr_cyScreen;
	m_colorDepth = DispDrvr_bpp;
	m_cbScanLineLength = DispDrvr_cxScreen;

	m_iRotate = GetRotateModeFromReg();
	SetRotateParams();

	// set rest of ModeInfo values
	m_ModeInfo.modeId = 0;
	m_ModeInfo.width = m_nScreenWidth;
	m_ModeInfo.height = m_nScreenHeight;
	m_ModeInfo.Bpp = m_colorDepth;
	m_ModeInfo.frequency = 60;    // ?
	m_ModeInfo.format = DispDrvr_format;
	m_pMode = &m_ModeInfo;

	// compute physical frame buffer size
	fbSize = (m_cyPhysicalScreen * m_cbScanLineLength);
	Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
	Adapter.InterfaceType = Internal;
	m_VirtualFrameBuffer = (DWORD)HalAllocateCommonBuffer(&Adapter, fbSize, &m_PhysFrameBufferAddr, FALSE);
	if (NULL == m_VirtualFrameBuffer)
	{
		RETAILMSG(1, (L"ERROR: Unable to allocate frame buffer!!!\n"));
		return;
	}
	m_pvFlatFrameBuffer = m_PhysFrameBufferAddr.LowPart;

	memset((void *)m_VirtualFrameBuffer, 0, fbSize);
	m_FrameBufferSize = fbSize;

	m_p2DVideoMemory = new Node2D(m_cbScanLineLength*8UL / m_pMode->Bpp, fbSize / m_cbScanLineLength, 0, 0, 4);
	if (!m_p2DVideoMemory)
	{
		RETAILMSG(1, (L"new Node2D failed\n"));
		return;
	}

	if (FAILED(AllocSurface(&m_pPrimarySurface, m_nScreenWidthSave, m_nScreenHeightSave, m_pMode->format, GPE_REQUIRE_VIDEO_MEMORY)))
	{
		RETAILMSG(1, (L"Couldn't allocate primary surface\n"));
		return;
	}
	m_pPrimarySurface->SetRotation(m_nScreenWidth, m_nScreenHeight, m_iRotate);

	m_CursorVisible = FALSE;
	m_CursorDisabled = TRUE;
	m_CursorForcedOff = FALSE;
	memset(&m_CursorRect, 0x0, sizeof(m_CursorRect));

	DispDrvrSetDibBuffer((void *)m_VirtualFrameBuffer);
}

S3C6410Disp::~S3C6410Disp()
{
	if (m_VirtualFrameBuffer != NULL)
	{
		DMA_ADAPTER_OBJECT Adapter={0,};
		Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
		Adapter.InterfaceType = Internal;
		HalFreeCommonBuffer(&Adapter, 0, m_PhysFrameBufferAddr, (PVOID)m_VirtualFrameBuffer, FALSE);
		m_VirtualFrameBuffer = NULL;
	}
}

SCODE S3C6410Disp::SetMode(INT modeId, HPALETTE *palette)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::SetMode\r\n")));

	if (modeId != 0)
	{
		DEBUGMSG(GPE_ZONE_ERROR, (TEXT("S3C6410Disp::SetMode Want mode %d, only have mode 0\r\n"), modeId));
		return E_INVALIDARG;
	}

	if (palette)
		*palette = EngCreatePalette(PAL_INDEXED, DispDrvr_palSize, (ULONG *)DispDrvr_palette, 0, 0, 0);

	return S_OK;
}

SCODE S3C6410Disp::GetModeInfo(GPEMode *mode, INT modeNumber)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::GetModeInfo\r\n")));

	if (modeNumber != 0)
	{
		return E_INVALIDARG;
	}

	*mode = m_ModeInfo;

	return S_OK;
}

int S3C6410Disp::NumModes()
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::NumModes\r\n")));
	return 1;
}

void S3C6410Disp::CursorOn(void)
{
#if	0
	UCHAR *ptrScreen = (UCHAR *)m_pPrimarySurface->Buffer();
	UCHAR *ptrLine;
	UCHAR *cbsLine;
	int x, y;

	if (!m_CursorForcedOff && !m_CursorDisabled && !m_CursorVisible)
	{
		RECTL cursorRectSave = m_CursorRect;
		int iRotate;
		RotateRectl(&m_CursorRect);
		for (y=m_CursorRect.top; y<m_CursorRect.bottom; y++)
		{
			if (y < 0)
			{
				continue;
			}
			if (y >= m_nScreenHeightSave)
			{
				break;
			}

			ptrLine = &ptrScreen[y * m_pPrimarySurface->Stride()];
			cbsLine = &m_CursorBackingStore[(y - m_CursorRect.top) * (m_CursorSize.x * (m_colorDepth >> 3))];

			for (x=m_CursorRect.left; x<m_CursorRect.right; x++)
			{
				if (x < 0)
				{
					continue;
				}
				if (x >= m_nScreenWidthSave)
				{
					break;
				}

				// x' = x - m_CursorRect.left; y' = y - m_CursorRect.top;
				// Width = m_CursorSize.x;   Height = m_CursorSize.y;
				switch (m_iRotate)
				{
				case DMDO_0:
					iRotate = (y - m_CursorRect.top)*m_CursorSize.x + x - m_CursorRect.left;
					break;
				case DMDO_90:
					iRotate = (x - m_CursorRect.left)*m_CursorSize.x + m_CursorSize.y - 1 - (y - m_CursorRect.top);
					break;
				case DMDO_180:
					iRotate = (m_CursorSize.y - 1 - (y - m_CursorRect.top))*m_CursorSize.x + m_CursorSize.x - 1 - (x - m_CursorRect.left);
					break;
				case DMDO_270:
					iRotate = (m_CursorSize.x -1 - (x - m_CursorRect.left))*m_CursorSize.x + y - m_CursorRect.top;
					break;
				default:
					iRotate = (y - m_CursorRect.top)*m_CursorSize.x + x - m_CursorRect.left;
					break;
				}
				cbsLine[(x - m_CursorRect.left) * (m_colorDepth >> 3)] = ptrLine[x * (m_colorDepth >> 3)];
				ptrLine[x * (m_colorDepth >> 3)] &= m_CursorAndShape[iRotate];
				ptrLine[x * (m_colorDepth >> 3)] ^= m_CursorXorShape[iRotate];
				if (m_colorDepth > 8)
				{
					cbsLine[(x - m_CursorRect.left) * (m_colorDepth >> 3) + 1] = ptrLine[x * (m_colorDepth >> 3) + 1];
					ptrLine[x * (m_colorDepth >> 3) + 1] &= m_CursorAndShape[iRotate];
					ptrLine[x * (m_colorDepth >> 3) + 1] ^= m_CursorXorShape[iRotate];
					if (m_colorDepth > 16)
					{
						cbsLine[(x - m_CursorRect.left) * (m_colorDepth >> 3) + 2] = ptrLine[x * (m_colorDepth >> 3) + 2];
						ptrLine[x * (m_colorDepth >> 3) + 2] &= m_CursorAndShape[iRotate];
						ptrLine[x * (m_colorDepth >> 3) + 2] ^= m_CursorXorShape[iRotate];
					}
				}
			}
		}
		m_CursorRect = cursorRectSave;
		m_CursorVisible = TRUE;
	}
#endif
}

void S3C6410Disp::CursorOff(void)
{
#if	0
	UCHAR *ptrScreen = (UCHAR*)m_pPrimarySurface->Buffer();
	UCHAR *ptrLine;
	UCHAR *cbsLine;
	int x, y;

	if (!m_CursorForcedOff && !m_CursorDisabled && m_CursorVisible)
	{
		RECTL rSave = m_CursorRect;
		RotateRectl(&m_CursorRect);
		for (y=m_CursorRect.top; y<m_CursorRect.bottom; y++)
		{
			// clip to displayable screen area (top/bottom)
			if (y < 0)
			{
				continue;
			}
			if (y >= m_nScreenHeightSave)
			{
				break;
			}

			ptrLine = &ptrScreen[y * m_pPrimarySurface->Stride()];
			cbsLine = &m_CursorBackingStore[(y - m_CursorRect.top) * (m_CursorSize.x * (m_colorDepth >> 3))];

			for (x=m_CursorRect.left; x<m_CursorRect.right; x++)
			{
				// clip to displayable screen area (left/right)
				if (x < 0)
				{
					continue;
				}
				if (x >= m_nScreenWidthSave)
				{
					break;
				}

				ptrLine[x * (m_colorDepth >> 3)] = cbsLine[(x - m_CursorRect.left) * (m_colorDepth >> 3)];
				if (m_colorDepth > 8)
				{
					ptrLine[x * (m_colorDepth >> 3) + 1] = cbsLine[(x - m_CursorRect.left) * (m_colorDepth >> 3) + 1];
					if (m_colorDepth > 16)
					{
						ptrLine[x * (m_colorDepth >> 3) + 2] = cbsLine[(x - m_CursorRect.left) * (m_colorDepth >> 3) + 2];
					}
				}
			}
		}
		m_CursorRect = rSave;
		m_CursorVisible = FALSE;
	}
#endif
}

SCODE S3C6410Disp::SetPointerShape(GPESurf *pMask, GPESurf *pColorSurf, INT xHot, INT yHot, INT cX, INT cY)
{
#if	0
	UCHAR *andPtr;	// input pointer
	UCHAR *xorPtr;	// input pointer
	UCHAR *andLine;	// output pointer
	UCHAR *xorLine;	// output pointer
	char bAnd;
	char bXor;
	int row;
	int col;
	int i;
	int bitMask;

	DEBUGMSG(GPE_ZONE_CURSOR, (TEXT("S3C6410Disp::SetPointerShape(0x%X, 0x%X, %d, %d, %d, %d)\r\n"), pMask, pColorSurf, xHot, yHot, cX, cY));

	// turn current cursor off
	CursorOff();

	// release memory associated with old cursor
	if (!pMask)							// do we have a new cursor shape
	{
		m_CursorDisabled = TRUE;		// no, so tag as disabled
	}
	else
	{
		m_CursorDisabled = FALSE;		// yes, so tag as not disabled

		// store size and hotspot for new cursor
		m_CursorSize.x = cX;
		m_CursorSize.y = cY;
		m_CursorHotspot.x = xHot;
		m_CursorHotspot.y = yHot;

		andPtr = (UCHAR*)pMask->Buffer();
		xorPtr = (UCHAR*)pMask->Buffer() + (cY * pMask->Stride());

		// store OR and AND mask for new cursor
		for (row=0; row<cY; row++)
		{
			andLine = &m_CursorAndShape[cX * row];
			xorLine = &m_CursorXorShape[cX * row];

			for (col=0; col<cX/8; col++)
			{
				bAnd = andPtr[row * pMask->Stride() + col];
				bXor = xorPtr[row * pMask->Stride() + col];

				for (bitMask=0x0080, i=0; i<8; bitMask>>=1, i++)
				{
					andLine[(col * 8) + i] = bAnd & bitMask ? 0xFF : 0x00;
					xorLine[(col * 8) + i] = bXor & bitMask ? 0xFF : 0x00;
				}
			}
		}
	}
#endif
	return S_OK;
}

SCODE S3C6410Disp::MovePointer(INT xPosition, INT yPosition)
{
#if	0
	DEBUGMSG(GPE_ZONE_CURSOR, (TEXT("S3C6410Disp::MovePointer(%d, %d)\r\n"), xPosition, yPosition));

	CursorOff();

	if (xPosition != -1 || yPosition != -1)
	{
		// compute new cursor rect
		m_CursorRect.left = xPosition - m_CursorHotspot.x;
		m_CursorRect.right = m_CursorRect.left + m_CursorSize.x;
		m_CursorRect.top = yPosition - m_CursorHotspot.y;
		m_CursorRect.bottom = m_CursorRect.top + m_CursorSize.y;

		CursorOn();
	}
#endif
	return S_OK;
}

void S3C6410Disp::WaitForNotBusy(void)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::WaitForNotBusy\r\n")));
	return;
}

int S3C6410Disp::IsBusy(void)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::IsBusy\r\n")));
	return 0;
}

void S3C6410Disp::GetPhysicalVideoMemory(unsigned long *physicalMemoryBase, unsigned long *videoMemorySize)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::GetPhysicalVideoMemory\r\n")));

	*physicalMemoryBase = m_pvFlatFrameBuffer;
	*videoMemorySize    = m_FrameBufferSize;
}

void S3C6410Disp::GetVirtualVideoMemory(unsigned long *virtualMemoryBase, unsigned long *videoMemorySize)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::GetVirtualVideoMemory\r\n")));

	*virtualMemoryBase = m_VirtualFrameBuffer;
	*videoMemorySize   = m_FrameBufferSize;
}

SCODE S3C6410Disp::WrappedEmulatedLine(GPELineParms *lineParameters)
{
	SCODE retval;
	RECT  bounds;
	int   N_plus_1;	// Minor length of bounding rect + 1

	// calculate the bounding-rect to determine overlap with cursor
	if (lineParameters->dN)	// The line has a diagonal component (we'll refresh the bounding rect)
	{
		N_plus_1 = 2 + ((lineParameters->cPels * lineParameters->dN) / lineParameters->dM);
	}
	else
	{
		N_plus_1 = 1;
	}

	switch (lineParameters->iDir)
	{
	case 0:
		bounds.left = lineParameters->xStart;
		bounds.top = lineParameters->yStart;
		bounds.right = lineParameters->xStart + lineParameters->cPels + 1;
		bounds.bottom = bounds.top + N_plus_1;
		break;
	case 1:
		bounds.left = lineParameters->xStart;
		bounds.top = lineParameters->yStart;
		bounds.bottom = lineParameters->yStart + lineParameters->cPels + 1;
		bounds.right = bounds.left + N_plus_1;
		break;
	case 2:
		bounds.right = lineParameters->xStart + 1;
		bounds.top = lineParameters->yStart;
		bounds.bottom = lineParameters->yStart + lineParameters->cPels + 1;
		bounds.left = bounds.right - N_plus_1;
		break;
	case 3:
		bounds.right = lineParameters->xStart + 1;
		bounds.top = lineParameters->yStart;
		bounds.left = lineParameters->xStart - lineParameters->cPels;
		bounds.bottom = bounds.top + N_plus_1;
		break;
	case 4:
		bounds.right = lineParameters->xStart + 1;
		bounds.bottom = lineParameters->yStart + 1;
		bounds.left = lineParameters->xStart - lineParameters->cPels;
		bounds.top = bounds.bottom - N_plus_1;
		break;
	case 5:
		bounds.right = lineParameters->xStart + 1;
		bounds.bottom = lineParameters->yStart + 1;
		bounds.top = lineParameters->yStart - lineParameters->cPels;
		bounds.left = bounds.right - N_plus_1;
		break;
	case 6:
		bounds.left = lineParameters->xStart;
		bounds.bottom = lineParameters->yStart + 1;
		bounds.top = lineParameters->yStart - lineParameters->cPels;
		bounds.right = bounds.left + N_plus_1;
		break;
	case 7:
		bounds.left = lineParameters->xStart;
		bounds.bottom = lineParameters->yStart + 1;
		bounds.right = lineParameters->xStart + lineParameters->cPels + 1;
		bounds.top = bounds.bottom - N_plus_1;
		break;
	default:
		DEBUGMSG(GPE_ZONE_ERROR, (TEXT("Invalid direction: %d\r\n"), lineParameters->iDir));
		return E_INVALIDARG;
	}

	// check for line overlap with cursor and turn off cursor if overlaps
	RECTL cursorRect = m_CursorRect;
	RotateRectl(&cursorRect);

	if (m_CursorVisible && !m_CursorDisabled &&
		cursorRect.top < bounds.bottom && cursorRect.bottom > bounds.top &&
		cursorRect.left < bounds.right && cursorRect.right > bounds.left)
	{
		CursorOff();
		m_CursorForcedOff = TRUE;
	}

	// do emulated line
	retval = EmulatedLine(lineParameters);

	// see if cursor was forced off because of overlap with line bounds and turn back on
	if (m_CursorForcedOff)
	{
		m_CursorForcedOff = FALSE;
		CursorOn();
	}

	DispDrvrDirtyRectDump((LPRECT)&bounds);

	return retval;
}

SCODE S3C6410Disp::AllocSurface(GPESurf **ppSurf, int width, int height, EGPEFormat format, int surfaceFlags)
{
	DEBUGMSG(GPE_ZONE_INIT, (L"AllocSurface without pixelFormat\n"));

	if ((surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY) || (format == m_pMode->format) && (surfaceFlags & GPE_PREFER_VIDEO_MEMORY))
	{
		if (!(format == m_pMode->format))
		{
			DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Invalid format value\n"));
			return E_INVALIDARG;
		}

		// Attempt to allocate from video memory
		Node2D *pNode = m_p2DVideoMemory->Alloc(width, height);
		if (pNode)
		{
			DWORD bpp  = EGPEFormatToBpp[format];
			DWORD stride = ((bpp * width + 31) >> 5) << 2;
			ULONG offset = (m_cbScanLineLength * pNode->Top()) + ((pNode->Left() * EGPEFormatToBpp[format]) / 8);
			*ppSurf = new S3C6410DispSurf(width, height, offset, (PVOID)(m_VirtualFrameBuffer + offset), m_cbScanLineLength, format, pNode);
			if (!(*ppSurf))
			{
				pNode->Free();
				DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 1\n"));
				return E_OUTOFMEMORY;
			}
			return S_OK;
		}

		if (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
		{
			*ppSurf = (GPESurf *)NULL;
			DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 2\n"));
			return E_OUTOFMEMORY;
		}
	}

	if (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
	{
		*ppSurf = (GPESurf *)NULL;
		DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 3\n"));
		return E_OUTOFMEMORY;
	}

	// Allocate from system memory
	RETAILMSG(0, (TEXT("Creating a GPESurf in system memory. EGPEFormat = %d\r\n"), (int)format));
	*ppSurf = new GPESurf(width, height, format);
	if (*ppSurf != NULL)
	{
		// check we allocated bits succesfully
		if (((*ppSurf)->Buffer()) == NULL)
		{
			delete *ppSurf;
		}
		else
		{
			return    S_OK;
		}
	}

	DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 4\n"));
	return E_OUTOFMEMORY;
}

SCODE S3C6410Disp::AllocSurface(DDGPESurf **ppSurf, int width, int height, EGPEFormat format, EDDGPEPixelFormat pixelFormat, int surfaceFlags)
{
	if ((surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY) || (format == m_pMode->format) && (surfaceFlags & GPE_PREFER_VIDEO_MEMORY))
	{
		if (!(format == m_pMode->format))
		{
			DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Invalid format value\n"));
			return E_INVALIDARG;
		}

		// Attempt to allocate from video memory
		Node2D *pNode = m_p2DVideoMemory->Alloc(width, height);
		if (pNode)
		{
			DWORD bpp  = EGPEFormatToBpp[format];
			DWORD stride = ((bpp * width + 31) >> 5) << 2;
			ULONG offset = (m_cbScanLineLength * pNode->Top()) + ((pNode->Left() * EGPEFormatToBpp[format]) / 8);
			*ppSurf = new S3C6410DispSurf(width, height, offset, (PVOID)(m_VirtualFrameBuffer + offset), m_cbScanLineLength, format, pixelFormat, pNode);
			if (!(*ppSurf))
			{
				pNode->Free();
				DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 1\n"));
				return E_OUTOFMEMORY;
			}

			return S_OK;
		}

		if (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
		{
			*ppSurf = (DDGPESurf *)NULL;
			DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 2\n"));
			return DDERR_OUTOFVIDEOMEMORY;
		}
	}

	if (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
	{
		*ppSurf = (DDGPESurf *)NULL;
		DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 3\n"));
		return DDERR_OUTOFVIDEOMEMORY;
	}

	// Allocate from system memory
	RETAILMSG(0, (TEXT("Creating a GPESurf in system memory. EGPEFormat = %d\r\n"), (int)format));
	{
		DWORD bpp  = EGPEFormatToBpp[format];
		DWORD stride = ((bpp * width + 31) >> 5) << 2;
		DWORD nSurfaceBytes = stride * height;

		*ppSurf = new DDGPESurf(width, height, stride, format, pixelFormat);
	}

	if (*ppSurf != NULL)
	{
		// check we allocated bits succesfully
		if (((*ppSurf)->Buffer()) == NULL)
		{
			delete *ppSurf;
		}
		else
		{
			return    S_OK;
		}
	}

	DEBUGMSG(GPE_ZONE_WARNING, (L"AllocSurface - Out of Memory 4\n"));
	return E_OUTOFMEMORY;
}

SCODE S3C6410Disp::Line(GPELineParms *lineParameters, EGPEPhase phase)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::Line\r\n")));

	if (phase == gpeSingle || phase == gpePrepare)
	{
		DispPerfStart(ROP_LINE);

		if ((lineParameters->pDst != m_pPrimarySurface))
		{
			lineParameters->pLine = &GPE::EmulatedLine;
		}
		else
		{
			lineParameters->pLine = (SCODE (GPE::*)(struct GPELineParms *)) &S3C6410Disp::WrappedEmulatedLine;
		}
	}
	else if (phase == gpeComplete)
	{
		DispPerfEnd(0);
	}
	return S_OK;
}

SCODE S3C6410Disp::BltPrepare(GPEBltParms *blitParameters)
{
	RECTL rectl;
	int   iSwapTmp;

	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::BltPrepare\r\n")));

	DispPerfStart(blitParameters->rop4);

	// default to base EmulatedBlt routine
	blitParameters->pBlt = &GPE::EmulatedBlt;

	// see if we need to deal with cursor

	// check for destination overlap with cursor and turn off cursor if overlaps
	if (blitParameters->pDst == m_pPrimarySurface)	// only care if dest is main display surface
	{
		if (m_CursorVisible && !m_CursorDisabled)
		{
			if (blitParameters->prclDst != NULL)	// make sure there is a valid prclDst
			{
				rectl = *blitParameters->prclDst;	// if so, use it

				// There is no guarantee of a well ordered rect in blitParamters
				// due to flipping and mirroring.
				if (rectl.top > rectl.bottom)
				{
					iSwapTmp     = rectl.top;
					rectl.top    = rectl.bottom;
					rectl.bottom = iSwapTmp;
				}

				if (rectl.left > rectl.right)
				{
					iSwapTmp    = rectl.left;
					rectl.left  = rectl.right;
					rectl.right = iSwapTmp;
				}
			}
			else
			{
				rectl = m_CursorRect;	// if not, use the Cursor rect - this forces the cursor to be turned off in this case
			}

			if (m_CursorRect.top <= rectl.bottom && m_CursorRect.bottom >= rectl.top &&
				m_CursorRect.left <= rectl.right && m_CursorRect.right >= rectl.left)
			{
				CursorOff();
				m_CursorForcedOff = TRUE;
			}
		}
	}

	// check for source overlap with cursor and turn off cursor if overlaps
	if (blitParameters->pSrc == m_pPrimarySurface)	// only care if source is main display surface
	{
		if (m_CursorVisible && !m_CursorDisabled)
		{
			if (blitParameters->prclSrc != NULL)	// make sure there is a valid prclSrc
			{
				rectl = *blitParameters->prclSrc;	// if so, use it
			}
			else
			{
				rectl = m_CursorRect;	// if not, use the CUrsor rect - this forces the cursor to be turned off in this case
			}
			if (m_CursorRect.top < rectl.bottom && m_CursorRect.bottom > rectl.top &&
				m_CursorRect.left < rectl.right && m_CursorRect.right > rectl.left)
			{
				CursorOff();
				m_CursorForcedOff = TRUE;
			}
		}
	}

	if (blitParameters->prclDst != NULL)
	{
		rectl = *blitParameters->prclDst;	// if so, use it

		// There is no guarantee of a well ordered rect in blitParamters
		// due to flipping and mirroring.
		if (rectl.top > rectl.bottom)
		{
			iSwapTmp     = rectl.top;
			rectl.top    = rectl.bottom;
			rectl.bottom = iSwapTmp;
		}

		if (rectl.left > rectl.right)
		{
			iSwapTmp    = rectl.left;
			rectl.left  = rectl.right;
			rectl.right = iSwapTmp;
		}

		DispDrvrDirtyRectDump((LPRECT)&rectl);
	}

	return S_OK;
}

SCODE S3C6410Disp::BltComplete(GPEBltParms *blitParameters)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::BltComplete\r\n")));

	// see if cursor was forced off because of overlap with source or destination and turn back on
	if (m_CursorForcedOff)
	{
		m_CursorForcedOff = FALSE;
		CursorOn();
	}

	DispPerfEnd(0);

	return S_OK;
}

INT S3C6410Disp::InVBlank(void)
{
	static BOOL value = FALSE;
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::InVBlank\r\n")));
	value = !value;
	return value;
}

SCODE S3C6410Disp::SetPalette(const PALETTEENTRY *source, USHORT firstEntry, USHORT numEntries)
{
	DEBUGMSG(GPE_ZONE_INIT, (TEXT("S3C6410Disp::SetPalette\r\n")));

	if (firstEntry<0 || (firstEntry+numEntries)>256 || source == NULL)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

void S3C6410Disp::SetVisibleSurface(GPESurf *pTempSurf, BOOL bWaitForVBlank)
{
	S3C6410DispSurf *pSurf = (S3C6410DispSurf *)pTempSurf;
}

VOID S3C6410Disp::PowerHandler(BOOL bOff)
{
	DispDrvrPowerHandler(bOff);
}

ULONG S3C6410Disp::DrvEscape(SURFOBJ *pso, ULONG iEsc, ULONG cjIn, void *pvIn, ULONG cjOut, void *pvOut)
{
	if (iEsc == QUERYESCSUPPORT)
	{
		if (*(DWORD*)pvIn == DRVESC_GETSCREENROTATION
			|| *(DWORD*)pvIn == DRVESC_SETSCREENROTATION)
		{
			// The escape is supported.
			return 1;
		}
		else
		{
			// The escape isn't supported.
			//return 0;
			return DispDrvrDrvEscape(pso, iEsc, cjIn, pvIn, cjOut, pvOut);
		}
	}
	else if (iEsc == DRVESC_GETSCREENROTATION)
	{
		*(int *)pvOut = ((DMDO_0 | DMDO_90 | DMDO_180 | DMDO_270) << 8) | ((BYTE)m_iRotate);
		return DISP_CHANGE_SUCCESSFUL;
	}
	else if (iEsc == DRVESC_SETSCREENROTATION)
	{
		if ((cjIn == DMDO_0)   ||
			(cjIn == DMDO_90)  ||
			(cjIn == DMDO_180) ||
			(cjIn == DMDO_270) )
		{
			return DynRotate(cjIn);
		}

		return DISP_CHANGE_BADMODE;
	}
	else
	{
		return DispDrvrDrvEscape(pso, iEsc, cjIn, pvIn, cjOut, pvOut);
	}

	return 0;
}

int S3C6410Disp::GetRotateModeFromReg()
{
	HKEY hKey;
	int nRet = DMDO_0;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\GDI\\ROTATION"), 0, 0, &hKey))
	{
		DWORD dwSize, dwAngle, dwType = REG_DWORD;
		dwSize = sizeof(DWORD);
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("ANGLE"), NULL, &dwType, (LPBYTE)&dwAngle, &dwSize))
		{
			switch (dwAngle)
			{
			case 90:
				nRet = DMDO_90;
				break;
			case 180:
				nRet = DMDO_180;
				break;
			case 270:
				nRet = DMDO_270;
				break;
			case 0:
				// fall through
			default:
				nRet = DMDO_0;
				break;
			}
		}

		RegCloseKey(hKey);
	}

	return nRet;
}

void S3C6410Disp::SetRotateParams()
{
	int iswap;

	switch (m_iRotate)
	{
	case DMDO_0:
		m_nScreenHeightSave = m_nScreenHeight;
		m_nScreenWidthSave  = m_nScreenWidth;
		break;

	case DMDO_180:
		m_nScreenHeightSave = m_nScreenHeight;
		m_nScreenWidthSave  = m_nScreenWidth;
		break;

	case DMDO_90:
	case DMDO_270:
		iswap               = m_nScreenHeight;
		m_nScreenHeight     = m_nScreenWidth;
		m_nScreenWidth      = iswap;
		m_nScreenHeightSave = m_nScreenWidth;
		m_nScreenWidthSave  = m_nScreenHeight;
		break;

	default:
		m_nScreenHeightSave = m_nScreenHeight;
		m_nScreenWidthSave  = m_nScreenWidth;
		break;
	}

	return;
}

LONG S3C6410Disp::DynRotate(int angle)
{
	GPESurfRotate *pSurf = (GPESurfRotate *)m_pPrimarySurface;

	if (angle == m_iRotate)
	{
		return DISP_CHANGE_SUCCESSFUL;
	}

	CursorOff();

	m_iRotate = angle;

	switch (m_iRotate)
	{
	case DMDO_0:
	case DMDO_180:
		m_nScreenHeight = m_nScreenHeightSave;
		m_nScreenWidth  = m_nScreenWidthSave;
		break;

	case DMDO_90:
	case DMDO_270:
		m_nScreenHeight = m_nScreenWidthSave;
		m_nScreenWidth  = m_nScreenHeightSave;
		break;
	}

	m_pMode->width  = m_nScreenWidth;
	m_pMode->height = m_nScreenHeight;

	pSurf->SetRotation(m_nScreenWidth, m_nScreenHeight, angle);

	CursorOn();

	return DISP_CHANGE_SUCCESSFUL;
}

S3C6410DispSurf::S3C6410DispSurf(int width, int height, ULONG offset, void *pBits, int stride, EGPEFormat format, Node2D *pNode)
	: DDGPESurf(width, height, pBits, stride, format)
{
	m_pNode2D              = pNode;
	m_fInVideoMemory       = FALSE;
	m_nOffsetInVideoMemory = offset;
}

S3C6410DispSurf::S3C6410DispSurf(int width, int height, ULONG offset, void *pBits, int stride, EGPEFormat format, EDDGPEPixelFormat pixelFormat, Node2D *pNode)
	: DDGPESurf(width, height, pBits, stride, format, pixelFormat)
{
	m_pNode2D              = pNode;
	m_fInVideoMemory       = FALSE;
	m_nOffsetInVideoMemory = offset;
}

S3C6410DispSurf::~S3C6410DispSurf()
{
	m_pNode2D->Free();
}


ULONG gBitMasks[] = {0xf800,0x07e0,0x001f};
ULONG *APIENTRY DrvGetMasks(DHPDEV dhpdev)
{
	return gBitMasks;
}

