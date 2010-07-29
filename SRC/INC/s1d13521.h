
#ifndef __S1D13521_H__
#define __S1D13521_H__


//////////////////////////////////////////////////////////////////////////////////
typedef enum {
	POWER_RUN=0,
	POWER_STANDBY,
	POWER_SLEEP,
	POWER_LAST,
} POWERSTATE, *PPOWERSTATE;
typedef enum {
	DSPUPD_FULL=0,
	DSPUPD_PART,
	DSPUPD_LAST,
} DSPUPDSTATE, *PDSPUPDSTATE;
typedef enum {
	WAVEFORM_INIT=0,	// 4 Sec
	WAVEFORM_DU,		// 260 mSec
	WAVEFORM_GU,		// 780 mSec
	WAVEFORM_GC,		// 780 mSec
	WAVEFORM_AUTO_DUGU,
	WAVEFORM_LAST,
} WAVEFORMMODE, *PWAVEFORMMODE;
typedef enum {
	ALIGN_TOP=0x0,
	ALIGN_LEFT=0x0,
	ALIGN_CENTER=0x1,
	ALIGN_RIGHT=0x2,
	ALIGN_VCENTER=0x4,
	ALIGN_BOTTOM=0x8,
} ALIGNIMAGE, *PALIGNIMAGE;

typedef struct
{
	UINT32 CMD;		// 000
	UINT32 DATA;	// 004
} S1D13521_REG, *PS1D13521_REG;
typedef struct {
	BYTE	bCmd;
	int		nArgc;
	WORD	pArgv[5];
} CMDARG, *PCMDARG;
typedef struct {
	PBYTE	pBuffer;	// NULL == GPE BUFFER
	PRECT	pRect;		// NULL == GPE FULL
} IMAGERECT, *PIMAGERECT;

typedef struct {
	BOOL			bWriteImage;
	PRECT			pRect;
	DSPUPDSTATE		duState;
	BOOL			bBorder;
	WAVEFORMMODE	wfMode;
} DISPUPDATE, *PDISPUPDATE;
typedef struct {
	PBYTE		pBuffer;
	int			nCount;
	ALIGNIMAGE	Align;
	int			x, y;
	PDISPUPDATE	pUpdate;
	BOOL		bIsWait;
} DISPBITMAP, *PDISPBITMAP;


#define	MSGQUEUE_NAME	_T("MSGQUEUE_DIRTYRECT")
typedef struct {
	RECT	rect;
	DWORD	time;
} DIRTYRECTINFO, *PDIRTYRECTINFO;

//////////////////////////////////////////////////////////////////////////////////
// Microsoft reserves the range 0 to 0x10000 for its escape codes.
enum {
	DRVESC_BASE=0x10010,

	DRVESC_SET_DEBUGLEVEL,		//01 DWORD			[in,ret]
	DRVESC_GET_DEBUGLEVEL,		//02 DWORD			[ret]

	DRVESC_SET_DIRTYRECT,		//03 BOOL			[in,ret]
	DRVESC_GET_DIRTYRECT,		//04 BOOL			[ret]
	DRVESC_SET_DSPUPDSTATE,		//05 DSPUPDSTATE	[in,ret]
	DRVESC_GET_DSPUPDSTATE,		//06 DSPUPDSTATE	[ret]
	DRVESC_SET_BORDER,			//07 BOOL			[in,ret]
	DRVESC_GET_BORDER,			//08 BOOL			[ret]
	DRVESC_SET_WAVEFORMMODE,	//09 WAVEFORMMODE	[in,ret]
	DRVESC_GET_WAVEFORMMODE,	//10 WAVEFORMMODE	[ret]
	DRVESC_SET_POWERSTATE,		//11 POWERSTATE		[in,ret]
	DRVESC_GET_POWERSTATE,		//12 POWERSTATE		[ret]

	DRVESC_SYSTEM_SLEEP,		//13 ...Removed...	[ret]
	DRVESC_SYSTEM_WAKEUP,		//14 ...Removed...	[ret]
	DRVESC_WAIT_HRDY,			//15 BOOL			[ret]

	DRVESC_COMMAND,				//16 CMDARG			[set]
	DRVESC_READ_DATA,			//17 WORD			[ret]
	DRVESC_WRITE_BURST,			//18 BLOB			[set]
	DRVESC_WRITE_REG,			//19 DWORD			[in]	// wReg(hi), wData(lo)
	DRVESC_READ_REG,			//20 WORD			[in,ret]
	DRVESC_WRITE_SFM,			//21 BLOB			[set]
	DRVESC_READ_SFM,			//22 BLOB			[get]
	DRVESC_GET_TEMPERATURE,		//23 WORD			[ret]

	DRVESC_WRITE_IMAGE,			//24 IMAGERECT		[set]
	DRVESC_WRITE_UPDATE,		//25 RECT			[set]
	DRVESC_IMAGE_UPDATE,		//26 IMAGERECT		[set]

	DRVESC_DISP_BITMAP,			//27 DISPBITMAP		[set]
	DRVESC_DISP_UPDATE,			//28 DISPUPDATE		[set]

	// ...

	DRVESC_TEST=(DRVESC_BASE+50),
	// +++ DRVESC_TEST +++
	DRVESC_REG16_CMD,			//51 USHORT			[set]
	DRVESC_REG16_OUTPUT,		//52 USHORT			[set]
	DRVESC_REG16_INPUT,			//53 USHORT			[ret]
	DRVESC_COMMAND2,			//54 CMDARG			[set]
	DRVESC_WRITE_REG2,			//55 DWORD			[in]	// wReg(hi), wData(lo)
	DRVESC_READ_REG2,			//56 WORD			[in,ret]
	DRVESC_DIRTYRECT_NOTIFY,	//57 BOOL			[in,ret]
	// --- DRVESC_TEST ---

	DRVESC_MAX
};
//////////////////////////////////////////////////////////////////////////////////


#endif	//__S1D13521_H__

