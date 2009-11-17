
#ifndef __S3C6410_DISP_H__
#define __S3C6410_DISP_H__


class S3C6410Disp  : public GPE
{
private:
	GPEMode	m_ModeInfo;
	void	*m_pVirtualFrameBuffer;

	SCODE	WrappedEmulatedLine(GPELineParms *pParms);
	SCODE	WrappedEmulatedBlt(GPEBltParms *pParms);

public:
					S3C6410Disp();
	virtual int 	NumModes();
	virtual SCODE 	SetMode(int modeId, HPALETTE *pPalette);
	virtual int 	InVBlank();
	virtual SCODE 	SetPalette(const PALETTEENTRY *src, unsigned short firstEntry, unsigned short numEntries);
	virtual SCODE 	GetModeInfo(GPEMode *pMode, int modeNo);
	virtual SCODE	SetPointerShape(GPESurf *pMask, GPESurf *pColorSurf, int xHot, int yHot, int cx, int cy);
	virtual SCODE	MovePointer(int x, int y);
	virtual void	WaitForNotBusy();
	virtual int		IsBusy();
	virtual void	GetPhysicalVideoMemory(unsigned long *pPhysicalMemoryBase, unsigned long *pVideoMemorySize);
	virtual SCODE	AllocSurface(GPESurf **ppSurf, int width, int height, EGPEFormat format, int surfaceFlags);
	virtual SCODE	Line(GPELineParms *pLineParms, EGPEPhase phase);
	virtual SCODE	BltPrepare(GPEBltParms *pBltParms);
	virtual SCODE	BltComplete(GPEBltParms *pBltParms);

	virtual VOID	PowerHandler(BOOL bOff);
	virtual ULONG	DrvEscape(SURFOBJ *pso, ULONG iEsc, ULONG cjIn, PVOID pvIn, ULONG cjOut, PVOID pvOut);

};


#endif	//__S3C6410_DISP_H__

