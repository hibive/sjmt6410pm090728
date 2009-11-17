
#ifndef __S3C6410_DISP_H__
#define __S3C6410_DISP_H__

class S3C6410DispSurf;

class S3C6410Disp : public DDGPE
{
private:
	GPEMode	m_ModeInfo;
	DWORD	m_cbScanLineLength;
	DWORD	m_cxPhysicalScreen;
	DWORD	m_cyPhysicalScreen;
	DWORD	m_colorDepth;

	PHYSICAL_ADDRESS	m_PhysFrameBufferAddr;
	DWORD	m_pvFlatFrameBuffer;
	DWORD	m_VirtualFrameBuffer;
	DWORD	m_FrameBufferSize;

	Node2D	*m_p2DVideoMemory;

	BOOL	m_CursorDisabled;
	BOOL	m_CursorVisible;
	BOOL	m_CursorForcedOff;
	RECTL	m_CursorRect;
	POINTL	m_CursorSize;
	POINTL	m_CursorHotspot;

	// allocate enough backing store for a 64x64 cursor on a 32bpp (4 bytes per pixel) screen
	UCHAR	m_CursorBackingStore[64 * 64 * 4];
	UCHAR	m_CursorXorShape[64 * 64];
	UCHAR	m_CursorAndShape[64 * 64];

public:
					S3C6410Disp();
	virtual			~S3C6410Disp();

	virtual int		NumModes();
	virtual SCODE	SetMode(int modeId, HPALETTE *palette);
	virtual int		InVBlank();
	virtual SCODE	SetPalette(const PALETTEENTRY *source, USHORT firstEntry, USHORT numEntries);
	virtual SCODE	GetModeInfo(GPEMode *pMode, int modeNumber);
	virtual SCODE	SetPointerShape(GPESurf *mask, GPESurf *colorSurface, int xHot, int yHot, int cX, int cY);
	virtual SCODE	MovePointer(int xPosition, int yPosition);
	virtual void	WaitForNotBusy();
	virtual int		IsBusy();
	virtual void	GetPhysicalVideoMemory(unsigned long *physicalMemoryBase, unsigned long *videoMemorySize);
	void			GetVirtualVideoMemory(unsigned long *virtualMemoryBase, unsigned long *videoMemorySize);
	virtual SCODE	AllocSurface(GPESurf **surface, int width, int height, EGPEFormat format, int surfaceFlags);
	virtual SCODE	AllocSurface(DDGPESurf **ppSurf, int width, int height, EGPEFormat format, EDDGPEPixelFormat pixelFormat, int surfaceFlags);
	virtual SCODE	Line(GPELineParms *lineParameters, EGPEPhase phase);
	virtual void	SetVisibleSurface(GPESurf *pSurf, BOOL bWaitForVBlank=FALSE);
	virtual SCODE	BltPrepare(GPEBltParms *blitParameters);
	virtual SCODE	BltComplete(GPEBltParms *blitParameters);
	virtual VOID	PowerHandler(BOOL bOff);
	virtual ULONG	DrvEscape(SURFOBJ *pso, ULONG iEsc, ULONG cjIn, void *pvIn, ULONG cjOut, void *pvOut);

	SCODE			WrappedEmulatedLine(GPELineParms *lineParameters);
	void			CursorOn();
	void			CursorOff();

	int				GetRotateModeFromReg();
	void			SetRotateParams();
	long			DynRotate(int angle);

	friend void		buildDDHALInfo(LPDDHALINFO lpddhi, DWORD modeidx);
};

class S3C6410DispSurf : public DDGPESurf
{
private:
	Node2D	*m_pNode2D;

public:
			S3C6410DispSurf(int width, int height, ULONG offset, void *pBits, int stride, EGPEFormat format, Node2D *pNode);
			S3C6410DispSurf(int width, int height, ULONG offset, void *pBits, int stride, EGPEFormat format, EDDGPEPixelFormat pixelFormat, Node2D *pNode);
	virtual	~S3C6410DispSurf();

	int		Top()	{ return m_pNode2D->Top(); }
	int		Left()	{ return m_pNode2D->Left(); }
};

#endif __S3C6410_DISP_H__

