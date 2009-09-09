
#ifndef __EBOOK2_DISPDRVR_H__
#define __EBOOK2_DISPDRVR_H__

#ifdef __cplusplus
extern "C" {
#endif


extern void *DispDrvrPhysicalFrameBuffer;
extern int DispDrvr_cxScreen;
extern int DispDrvr_cyScreen;
extern int DispDrvr_bpp;
extern EGPEFormat DispDrvr_format;
extern int DispDrvr_fbSize;
extern int DispDrvr_palSize;
extern RGBQUAD DispDrvr_palette[];

void  DispDrvrInitialize(void);
void  DispDrvrSetDibBuffer(void *pvDibBuffer);
void  DispDrvrDirtyRectDump(LPCRECT prc);
void  DispDrvrPowerHandler(BOOL bOff);
void  DispDrvrMoveCursor(INT32 xLocation, INT32 yLocation);
ULONG DispDrvrDrvEscape(SURFOBJ *pso, ULONG iEsc,
	ULONG cjIn, PVOID pvIn, ULONG cjOut, PVOID pvOut);


#ifdef __cplusplus
}
#endif

#endif	//__EBOOK2_DISPDRVR_H__
