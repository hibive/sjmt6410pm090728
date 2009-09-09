
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
} CMDARGS, *PCMDARGS;
typedef struct {
	PBYTE	pBuffer;	// NULL == GPE BUFFER
	PRECT	pRect;		// NULL == GPE FULL
} IMAGEDATAS, *PIMAGEDATAS;
typedef struct {
	PBYTE		pBuffer;
	int			nCount;
	ALIGNIMAGE	Align;
	int			x, y;
} IMAGEFILES, *PIMAGEFILES;
//////////////////////////////////////////////////////////////////////////////////
// Microsoft reserves the range 0 to 0x10000 for its escape codes.
enum {
	DRVESC_BASE=0x10010,

	DRVESC_SET_DEBUGLEVEL,		// DWORD		[in,ret]
	DRVESC_GET_DEBUGLEVEL,		// DWORD		[ret]

	DRVESC_SET_DIRTYRECT,		// BOOL			[in,ret]
	DRVESC_GET_DIRTYRECT,		// BOOL			[ret]
	DRVESC_SET_DSPUPDSTATE,		// DSPUPDSTATE	[in,ret]
	DRVESC_GET_DSPUPDSTATE,		// DSPUPDSTATE	[ret]
	DRVESC_SET_BORDER,			// BOOL			[in,ret]
	DRVESC_GET_BORDER,			// BOOL			[ret]
	DRVESC_SET_WAVEFORMMODE,	// WAVEFORMMODE	[in,ret]
	DRVESC_GET_WAVEFORMMODE,	// WAVEFORMMODE	[ret]
	DRVESC_SET_POWERSTATE,		// POWERSTATE	[in,ret]
	DRVESC_GET_POWERSTATE,		// POWERSTATE	[ret]

	DRVESC_SYSTEM_SLEEP,		// BOOL			[ret]
	DRVESC_SYSTEM_WAKEUP,		// BOOL			[ret]
	DRVESC_WAIT_HRDY,			// BOOL			[ret]
	DRVESC_COMMAND,				// CMDARGS		[set]
	DRVESC_READ_DATA,			// WORD			[ret]
	DRVESC_WRITE_BURST,			// BLOB			[set]
	DRVESC_WRITE_REG,			// DWORD		[in]	// wReg(hi), wData(lo)
	DRVESC_READ_REG,			// WORD			[in,ret]
	DRVESC_WRITE_SFM,			// BLOB			[set]
	DRVESC_READ_SFM,			// BLOB			[get]
	DRVESC_GET_TEMPERATURE,		// WORD			[ret]

	DRVESC_WRITE_IMAGE,			// IMAGEDATAS	[set]
	DRVESC_WRITE_UPDATE,		// RECT			[set]
	DRVESC_IMAGE_UPDATE,		// IMAGEDATAS	[set]

	DRVESC_DISPLAY_BITMAP,		// IMAGEFILES	[set]

	DRVESC_MAX
};
//////////////////////////////////////////////////////////////////////////////////


#endif	//__S1D13521_H__

