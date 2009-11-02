
#include <bsp.h>
#include "s1d13521.h"


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)

#define USE_S1D13521_BIGENDIAN	1


#define S1D13521_FB_WIDTH		800
#define S1D13521_FB_HEIGHT		600
#if	(LCD_BPP == 4)
#define S1D13521_FB_BPP			2	// 2bpp(0), 3bpp(1), 4bpp(2), 8bpp(3)
#define S1D13521_FB_SIZE		(S1D13521_FB_WIDTH * S1D13521_FB_HEIGHT / 2)
#define S1D13521_PIXEL_SWAP(p)	((BYTE)(((BYTE)(p)>>4)&0x0F)|(((BYTE)(p)<<4)))
#elif (LCD_BPP == 8)
#define S1D13521_FB_BPP			3	// 2bpp(0), 3bpp(1), 4bpp(2), 8bpp(3)
#define S1D13521_FB_SIZE		(S1D13521_FB_WIDTH * S1D13521_FB_HEIGHT)
#else
#error LCD_BPP must be 4 or 8
#endif

#if (LCD_WIDTH == S1D13521_FB_WIDTH)
#define S1D13521_ORIENTATION	0	// 0(0), 90(1), 180(2), 270(3)
#define S1D13521_ORI_WIDTH		S1D13521_FB_WIDTH
#define S1D13521_ORI_HEIGHT		S1D13521_FB_HEIGHT
#else
#if	(EBOOK2_VER == 3)
#define S1D13521_ORIENTATION	1	// 0(0), 90(1), 180(2), 270(3)
#elif	(EBOOK2_VER == 2)
#define S1D13521_ORIENTATION	3	// 0(0), 90(1), 180(2), 270(3)
#endif	EBOOK2_VER
#define S1D13521_ORI_WIDTH		S1D13521_FB_HEIGHT
#define S1D13521_ORI_HEIGHT		S1D13521_FB_WIDTH
#endif
#define	S1D13521_HRDY_TIMEOUT	300
#define FLASH_WFM_ADDR			0x0886
#define FLASH_PAGE_SIZE			0x100	// 256 bytes


typedef struct {
	WORD x;
	WORD y;
	WORD w;
	WORD h;
} AREA, *PAREA;



DWORD	g_dwDebugLevel = 0;

static BOOL			g_bDirtyRect = FALSE;
static DSPUPDSTATE	g_DspUpdState = DSPUPD_FULL;//DSPUPD_PART;//DSPUPD_FULL;
static BOOL			g_bBorder = TRUE;
static WAVEFORMMODE	g_WaveformMode = WAVEFORM_GU;
static POWERSTATE	g_PowerState = POWER_SLEEP;
static BYTE			g_sfmBuffer[FLASH_PAGE_SIZE];
static LPBYTE		g_lpFrameBuffer = NULL;
static volatile S1D13521_REG *g_pS1D13521Reg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOPReg = NULL;

static BOOL			g_bSleepDirtyRect = FALSE;
static POWERSTATE	g_SleepPowerState = POWER_SLEEP;



static WORD RegRead(WORD wReg);
static WORD RegRead2(WORD wReg, BOOL bWaitHrdy);
static BOOL RegWrite(WORD wReg, WORD wData);
static BOOL Command(CMDARGS CmdArgs);

static void delay(DWORD dwMSecs)
{
#ifdef	_EBOOT_
	volatile DWORD dwScaledSecs = (dwMSecs * (133000000 / 1200/*1585*/));
	while (dwScaledSecs--);
#else	_EBOOT_
	Sleep(dwMSecs);
#endif	_EBOOT_
}

static void initChip(void)
{
	CMDARGS CmdArgs;

	// PLL Configuration
	RegWrite(0x0010, 0x0003);
	RegWrite(0x0012, 0x5949);
	RegWrite(0x0014, 0x0040);
	RegWrite(0x0016, 0x0000);
	// bit0 : 0(not stable), 1(stable)
	while (!(RegRead(0x000A)&(1<<0)))
	{
		MYERR((_T("1")));
		delay(1);
	}

	// addr : Power Save Mode Register(0x0006)
	// data : Run(0), Off_Sleep_Standby(1)
	RegWrite(0x0006, 0x0000);

	MYMSG((_T("[S1D13521] INIT_SYS_RUN\r\n")));
	CmdArgs.bCmd = 0x06;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	MYERR((_T("[S1D13521] Revision Code 0x%04X\r\n"), RegRead(0x0000)));
	MYERR((_T("[S1D13521] Product Code 0x%04X\r\n"), RegRead(0x0002)));
	MYERR((_T("[S1D13521] 0x%04X, 0x%04X, 0x%04X, 0x%04X\r\n"),
		RegRead(0x0010), RegRead(0x0012), RegRead(0x0014), RegRead(0x0016)));

	RegWrite(0x0102, 0x0001);
	while (!(RegRead(0x0102)&(1<<8)))
	{
		MYERR((_T("2")));
		delay(1);
	}
	CmdArgs.bCmd = 0x08;
	CmdArgs.pArgv[0] = 0x50F0;	// 0x0100 : SDRAM Configuration
	CmdArgs.pArgv[1] = 0x0203;	// 0x0106 : SDRAM Refresh Clock Configuration
	CmdArgs.pArgv[2] = 0x0080;	// 0x0108 : SDRAM Read Data Tap Delay Select
	CmdArgs.pArgv[3] = 0x0000;	// 0x010A : SDRAM Extended Mode Configuration
	CmdArgs.nArgc = 4;
	Command(CmdArgs);
	MYERR((_T("SDRAM : 0x%04X(0x0100), 0x%04X(0x0106), 0x%04X(0x0108), 0x%04X(0x010A)\r\n"),
		RegRead(0x0100), RegRead(0x0106), RegRead(0x0108), RegRead(0x010A)));

	// addr : I2C Thermal Sensor Configuration Register(0x0210)
	// data : [10:8] I2C Thermal ID Address
	RegWrite(0x0210, (0<<8));
	// addr : I2C Thermal Sensor Clock Configuration Register(0x001A)
	// data : [3:0] I2C Thermal Sensor Clock Divide
	RegWrite(0x001A, 4);
	// addr : Themperature Device Select Register(0x0320)
	// data : [1] Themperature Device Source Select, [0] Themperature Auto Retrieval Disable
	RegWrite(0x0320, ((0<<1) | (0<<0)));

#ifdef	USE_S1D13521_BIGENDIAN
	RegWrite(0x0020, (1<<1));	// Bigendian mode
#endif	USE_S1D13521_BIGENDIAN
}
static void initDisplay(BOOL bClean)
{
	CMDARGS CmdArgs;
	int i;

	MYMSG((_T("[S1D13521] INIT_DSPE_CFG\r\n")));
	CmdArgs.bCmd = 0x09;
	CmdArgs.pArgv[0] = S1D13521_FB_WIDTH;
	CmdArgs.pArgv[1] = S1D13521_FB_HEIGHT;
	CmdArgs.pArgv[2] = ((1<<9) | (1<<8) | (100<<0));
	CmdArgs.pArgv[3] = (1<<1);
	CmdArgs.pArgv[4] = ((1<<7) | (4<<0));
	CmdArgs.nArgc = 5;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] INIT_DSPE_TMG\r\n")));
	CmdArgs.bCmd = 0x0A;
	CmdArgs.pArgv[0] = 4;
	CmdArgs.pArgv[1] = ((10<<8) | (4<<0));
	CmdArgs.pArgv[2] = 10;
	CmdArgs.pArgv[3] = ((100<<8) | (4<<0));
	CmdArgs.pArgv[4] = (6<<0);
	CmdArgs.nArgc = 5;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] INIT_ROTMODE\r\n")));
	CmdArgs.bCmd = 0x0B;
	CmdArgs.pArgv[0] = (S1D13521_ORIENTATION<<8);
	CmdArgs.nArgc = 1;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] RD_WFM_INFO\r\n")));
	CmdArgs.bCmd = 0x30;
	CmdArgs.pArgv[0] = FLASH_WFM_ADDR;
	CmdArgs.pArgv[1] = 0x0000;
	CmdArgs.nArgc = 2;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] WAIT_DSPE_TRG\r\n")));
	CmdArgs.bCmd = 0x28;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] UPD_GDRV_CLR\r\n")));
	CmdArgs.bCmd = 0x37;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] WAIT_DSPE_TRG\r\n")));
	CmdArgs.bCmd = 0x28;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	// addr : Update Buffer Configuration Register(0x0330)
	// data : [7] LUT Auto Select Enable, [2:0] LUT Index Format(P4N)
	RegWrite(0x0330, ((1<<7) | (4<<0)));

	MYMSG((_T("[S1D13521] WAIT_DSPE_TRG\r\n")));
	CmdArgs.bCmd = 0x28;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);


	MYMSG((_T("[S1D13521] LD_IMG\r\n")));
	CmdArgs.bCmd = 0x20;
	CmdArgs.pArgv[0] = (S1D13521_FB_BPP<<4);
	CmdArgs.nArgc = 1;
	Command(CmdArgs);
	MYMSG((_T("[S1D13521] WR_REG\r\n")));
	CmdArgs.bCmd = 0x11;
	CmdArgs.pArgv[0] = 0x0154;
	CmdArgs.nArgc = 1;
	Command(CmdArgs);
	for (i=0; i<S1D13521_FB_SIZE; i+=2)
		OUTREG16(&g_pS1D13521Reg->DATA, 0xFFFF);
	MYMSG((_T("[S1D13521] LD_IMG_END\r\n")));
	CmdArgs.bCmd = 0x23;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	if (TRUE == bClean)
	{
		MYMSG((_T("[S1D13521] UPD_FULL\r\n")));
		CmdArgs.bCmd = 0x33;
		CmdArgs.pArgv[0] = ((g_bBorder ? 1 : 0)<<14) | (WAVEFORM_INIT<<8);
		CmdArgs.nArgc = 1;
		Command(CmdArgs);
	}
	else
	{
		MYMSG((_T("[S1D13521] UPD_INIT\r\n")));
		CmdArgs.bCmd = 0x32;
		CmdArgs.nArgc = 0;
		Command(CmdArgs);
	}
	MYMSG((_T("[S1D13521] WAIT_DSPE_TRG\r\n")));
	CmdArgs.bCmd = 0x28;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);
	MYMSG((_T("[S1D13521] WAIT_DSPE_FREND\r\n")));
	CmdArgs.bCmd = 0x29;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);
}


#define	init_spi()	RegWrite(0x0204, (RegRead(0x0204) & 0xFF7F))
#define	exit_spi()	RegWrite(0x0204, (RegRead(0x0204) | 0x0080))
#define	start_spi()	RegWrite(0x0208, 0x01)
#define	end_spi()	RegWrite(0x0208, 0x00)
static BYTE read_spi_byte(void)
{
	RegWrite(0x0202, 0x0000); // dummy write
	while ((RegRead(0x0206) & 0x08))
	{
		//usleep(1);printk("busy..\n");
	}
	return (BYTE)RegRead(0x0200);
}
static void write_spi_byte(BYTE d)
{
	RegWrite(0x0202, (0x0100 | (d & 0xFF)));
	while ((RegRead(0x0206) & 0x08))
	{
		//usleep(1);printk("busy..\n");
	}
}
static void write_enable()
{
	start_spi();
	write_spi_byte(0x06);	// Write Enable
	end_spi();
}
static void write_disable()
{
	start_spi();
	write_spi_byte(0x04);	// Write Disable
	end_spi();
}
static void wait_wip(void)
{
	BYTE st_reg;

	do {
		start_spi();
		write_spi_byte(0x05);	// read status register
		st_reg = read_spi_byte();
		end_spi();
	} while (st_reg & 0x01);
}
static void read_flash(int page, unsigned char *rbuf)
{
	int i;

	start_spi();
	write_spi_byte(0x03);	// Read Data Bytes
	write_spi_byte((page>>8)&0xFF);	// address1
	write_spi_byte(page&0xFF);		// address2
	write_spi_byte(0x00);			// address3
	for (i=0; i<FLASH_PAGE_SIZE; i++)
		rbuf[i] = read_spi_byte();
	end_spi();
}
static void write_flash(int page, unsigned char *wbuf)
{
	int i;

	write_enable();
	start_spi();
	write_spi_byte(0x02);	// Page Program
	write_spi_byte((page>>8)&0xFF);	// address1
	write_spi_byte(page&0xFF);		// address2
	write_spi_byte(0x00);			// address3
	for (i=0; i<FLASH_PAGE_SIZE; i++)
		write_spi_byte(wbuf[i]);
	end_spi();
	wait_wip();
	write_disable();
}
static void erase_flash(int page)
{
	write_enable();
	start_spi();
	write_spi_byte(0xD8);	// Sector Erase
	write_spi_byte((page>>8)&0xFF);	// address1
	write_spi_byte(page&0xFF);		// address2
	write_spi_byte(0x00);			// address3
	end_spi();
	wait_wip();
	write_disable();
}
static void erase_all_flash(void)
{
	write_enable();
	start_spi();
	write_spi_byte(0xC7);	// Bulk Erase
	end_spi();
	wait_wip();
	write_disable();
}

static void rect2Area(PRECT pRect, PAREA pArea)
{
	if (0 > ((int)pRect->left))
		pRect->left = 0;
	pRect->left &= ~(0x3);
	if (0 > ((int)pRect->top))
		pRect->top = 0;
	if (S1D13521_ORI_WIDTH <= pRect->right)
		pRect->right = (S1D13521_ORI_WIDTH - 1);
	pRect->right |= (0x3);
	if (S1D13521_ORI_HEIGHT <= pRect->bottom)
		pRect->bottom = (S1D13521_ORI_HEIGHT - 1);

	pArea->x = (WORD)pRect->left;
	pArea->y = (WORD)pRect->top;
	pArea->w = (WORD)(pRect->right - pRect->left + 1);
	pArea->h = (WORD)(pRect->bottom - pRect->top + 1);
}



static BOOL WaitHrdy(int nDebug)
{
	volatile int i;

	for (i=0; i<S1D13521_HRDY_TIMEOUT; i++)
	{
		if ((g_pGPIOPReg->GPNDAT & (1<<8)))
			break;
		delay(5);
	}

	if (DRVESC_WAIT_HRDY == g_dwDebugLevel)
	{
		MYERR((_T(" WaitHrdy(%d, %d)\r\n"), nDebug, i));
	}

	if (S1D13521_HRDY_TIMEOUT == i)
	{
		MYERR((_T("WaitHrdy(%d)\r\n"), nDebug));
		return FALSE;
	}

	return TRUE;
}

static POWERSTATE SetPowerState(POWERSTATE ps)
{
	CMDARGS CmdArgs;

	if (POWER_RUN == ps)
	{
		RegWrite(0x0006, 0x0);
		//RegWrite(0x000A, (1<<12));
		delay(5);

		MYMSG((_T("[S1D13521] RUN_SYS\r\n")));
		CmdArgs.bCmd = 0x02;
	}
	else if (POWER_STANDBY == ps)
	{
		MYMSG((_T("[S1D13521] STBY\r\n")));
		CmdArgs.bCmd = 0x04;
	}
	else //if (POWER_SLEEP == ps)
	{
		MYMSG((_T("[S1D13521] SLP\r\n")));
		CmdArgs.bCmd = 0x05;
	}
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	if (DRVESC_SET_POWERSTATE == g_dwDebugLevel
		|| DRVESC_GET_POWERSTATE == g_dwDebugLevel)
	{
		MYERR((_T(" SetPowerState(%d)\r\n"), ps));
	}

	return ps;
}

static BOOL Command(CMDARGS CmdArgs)
{
	BOOL bRet;

	bRet = WaitHrdy(1);
	OUTREG16(&g_pS1D13521Reg->CMD, CmdArgs.bCmd);
	if (0 < CmdArgs.nArgc)
	{
		int i;
		//bRet = WaitHrdy(2);
		for (i=0; i<CmdArgs.nArgc; i++)
			OUTREG16(&g_pS1D13521Reg->DATA, CmdArgs.pArgv[i]);
	}

	if (DRVESC_COMMAND == g_dwDebugLevel)
	{
		MYERR((_T(" Command(%d, %d, %d, %d, %d, %d, %d)\r\n"),
			CmdArgs.bCmd, CmdArgs.nArgc, CmdArgs.pArgv[0], CmdArgs.pArgv[1],
			CmdArgs.pArgv[2], CmdArgs.pArgv[3], CmdArgs.pArgv[4]));
	}

	return bRet;
}

static WORD DataRead(void)
{
	WORD wData;

	WaitHrdy(3);
	wData = INREG16(&g_pS1D13521Reg->DATA);

	if (DRVESC_READ_DATA == g_dwDebugLevel)
	{
		MYERR((_T(" DataRead(%d)\r\n"), wData));
	}

	return wData;
}

static BOOL BurstWrite(BLOB *pBlob)
{
	int i;
	WORD wData;

	for (i=0; i<(int)pBlob->cbSize; i+=2)
	{
		wData = MAKEWORD(pBlob->pBlobData[i], pBlob->pBlobData[i+1]);
		OUTREG16(&g_pS1D13521Reg->DATA, wData);
	}

	if (DRVESC_WRITE_BURST == g_dwDebugLevel)
	{
		MYERR((_T(" BurstWrite(%d)\r\n"), pBlob->cbSize));
	}

	return TRUE;
}

static WORD RegRead(WORD wReg)
{
	WORD wData;

	WaitHrdy(4);
	OUTREG16(&g_pS1D13521Reg->CMD, 0x10);
	OUTREG16(&g_pS1D13521Reg->DATA, wReg);
	wData = INREG16(&g_pS1D13521Reg->DATA);

	if (DRVESC_READ_REG == g_dwDebugLevel)
	{
		MYERR((_T(" RegRead(%d, %d)\r\n"), wReg, wData));
	}

	return wData;
}
static WORD RegRead2(WORD wReg, BOOL bWaitHrdy)
{
	WORD wData;

	if (bWaitHrdy)
		WaitHrdy(4);
	OUTREG16(&g_pS1D13521Reg->CMD, 0x10);
	OUTREG16(&g_pS1D13521Reg->DATA, wReg);
	wData = INREG16(&g_pS1D13521Reg->DATA);

	if (DRVESC_READ_REG == g_dwDebugLevel)
	{
		MYERR((_T(" RegRead2(%d, %d)\r\n"), wReg, wData));
	}

	return wData;
}

static BOOL RegWrite(WORD wReg, WORD wData)
{
	BOOL bRet;

	bRet = WaitHrdy(5);
	OUTREG16(&g_pS1D13521Reg->CMD, 0x11);
	OUTREG16(&g_pS1D13521Reg->DATA, wReg);
	OUTREG16(&g_pS1D13521Reg->DATA, wData);

	if (DRVESC_WRITE_REG == g_dwDebugLevel)
	{
		MYERR((_T(" RegWrite(%d, %d)\r\n"), wReg, wData));
	}

	return bRet;
}

static int SfmRead(BLOB *pBlob)
{
	int i;
	PBYTE pData = pBlob->pBlobData;

	init_spi();
	for (i=0; i<(int)(pBlob->cbSize / FLASH_PAGE_SIZE); i++)
	{
		read_flash(i, g_sfmBuffer);
		memcpy(pData, g_sfmBuffer, FLASH_PAGE_SIZE);
		pData += FLASH_PAGE_SIZE;
	}
	if (pBlob->cbSize % FLASH_PAGE_SIZE)
	{
		read_flash(i, g_sfmBuffer);
		memcpy(pData, g_sfmBuffer, (pBlob->cbSize % FLASH_PAGE_SIZE));
	}
	exit_spi();

	if (DRVESC_READ_SFM == g_dwDebugLevel)
	{
		MYERR((_T(" SfmRead(%d)\r\n"), pBlob->cbSize));
	}

	return pBlob->cbSize;
}
static int SfmWrite(BLOB *pBlob)
{
	int i;
	PBYTE pData = pBlob->pBlobData;

	init_spi();
	erase_all_flash();
	for (i=0; i<(int)(pBlob->cbSize / FLASH_PAGE_SIZE); i++)
	{
		memcpy(g_sfmBuffer, pData, FLASH_PAGE_SIZE);
		pData += FLASH_PAGE_SIZE;
		write_flash(i, g_sfmBuffer);
	}
	if (pBlob->cbSize % FLASH_PAGE_SIZE)
	{
		memcpy(g_sfmBuffer, pData, (pBlob->cbSize % FLASH_PAGE_SIZE));
		write_flash(i, g_sfmBuffer);
	}
	exit_spi();

	if (DRVESC_WRITE_SFM == g_dwDebugLevel)
	{
		MYERR((_T(" SfmWrite(%d)\r\n"), pBlob->cbSize));
	}

	return pBlob->cbSize;
}

static BOOL ImageWrite(PIMAGEDATAS pid)
{
	PBYTE pBuffer;
	CMDARGS CmdArgs;
	int i, j, offset, pos, sy, ey, sx, ex;

	pBuffer = (NULL == pid->pBuffer) ? g_lpFrameBuffer : pid->pBuffer;
	if (NULL == pBuffer)
		return FALSE;

	if (NULL == pid->pRect)
	{
		MYMSG((_T("[S1D13521] LD_IMG\r\n")));
		CmdArgs.bCmd = 0x20;
		CmdArgs.pArgv[0] = (S1D13521_FB_BPP<<4);
		CmdArgs.nArgc = 1;
		Command(CmdArgs);

		sy = 0;
		ey = S1D13521_FB_HEIGHT;
		sx = 0;
		ex = S1D13521_FB_WIDTH;
	}
	else
	{
		AREA Area;
		rect2Area(pid->pRect, &Area);

		MYMSG((_T("[S1D13521] LD_IMG_AREA\r\n")));
		CmdArgs.bCmd = 0x22;
		CmdArgs.pArgv[0] = (S1D13521_FB_BPP<<4);
		CmdArgs.pArgv[1] = Area.x;
		CmdArgs.pArgv[2] = Area.y;
		CmdArgs.pArgv[3] = Area.w;
		CmdArgs.pArgv[4] = Area.h;
		CmdArgs.nArgc = 5;
		Command(CmdArgs);

		sy = Area.y;
		ey = (Area.y + Area.h);
		sx = Area.x;
		ex = (Area.x + Area.w);
	}

	MYMSG((_T("[S1D13521] WR_REG\r\n")));
	CmdArgs.bCmd = 0x11;
	CmdArgs.pArgv[0] = 0x0154;
	CmdArgs.nArgc = 1;
	Command(CmdArgs);
	// Param[2] : Register Write Data
	for (i=sy; i<ey; i++)
	{
		offset = (i * S1D13521_ORI_WIDTH);
#if	(LCD_BPP == 4)
		for (j=sx; j<ex; j+=4)
		{
			pos = (offset + j) >> 1;
#ifdef	USE_S1D13521_BIGENDIAN
			OUTREG16(&g_pS1D13521Reg->DATA,
				MAKEWORD(pBuffer[pos+1], pBuffer[pos]));
#else	USE_S1D13521_BIGENDIAN
			OUTREG16(&g_pS1D13521Reg->DATA,
				MAKEWORD(S1D13521_PIXEL_SWAP(pBuffer[pos]),
					S1D13521_PIXEL_SWAP(pBuffer[pos+1])));
#endif	USE_S1D13521_BIGENDIAN
		}
#elif (LCD_BPP == 8)
		for (j=sx; j<ex; j+=2)
		{
			pos = offset + j;
			OUTREG16(&g_pS1D13521Reg->DATA,
				MAKEWORD(pBuffer[pos+1], pBuffer[pos]));
		}
#endif
	}
	// (0, 0, 600, 800) Update Time
	// LCD_BPP == 8 : 110 mSec
	// LCD_BPP == 4 :  55 mSec
	// LCD_BPP == 4 :  54 mSec	// USE_S1D13521_BIGENDIAN

	MYMSG((_T("[S1D13521] LD_IMG_END\r\n")));
	CmdArgs.bCmd = 0x23;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	if (DRVESC_WRITE_IMAGE == g_dwDebugLevel)
	{
		MYERR((_T(" ImageWrite(0x%x, %d, %d, %d)\r\n"), pBuffer, sy, ey, sx, ex));
	}

	return TRUE;
}
static BOOL UpdateWrite(PRECT pRect)
{
	CMDARGS CmdArgs;
	DWORD i=0, loop=0;
	WORD wData=0;
#ifndef	_EBOOT_
	DWORD dwStart = GetTickCount();
#endif	_EBOOT_

	if (NULL == pRect)
	{
		MYMSG((_T("[S1D13521] UPD_FULL or UPD_PART\r\n")));
		// 0x33(UPD_FULL), 0x35(UPD_PART)
		CmdArgs.bCmd = (DSPUPD_FULL == g_DspUpdState) ? 0x33 : 0x35;
		CmdArgs.pArgv[0] = ((g_bBorder ? 1 : 0)<<14) | (g_WaveformMode<<8);
		CmdArgs.nArgc = 1;
		Command(CmdArgs);
	}
	else
	{
		AREA Area;
		rect2Area(pRect, &Area);

		MYMSG((_T("[S1D13521] UPD_FULL_AREA or UPD_PART_AREA\r\n")));
		// 0x34(UPD_FULL_AREA), 0x36(UPD_PART_AREA)
		CmdArgs.bCmd = (DSPUPD_FULL == g_DspUpdState) ? 0x34 : 0x36;
		CmdArgs.pArgv[0] = ((g_bBorder ? 1 : 0)<<14) | (g_WaveformMode<<8);
		CmdArgs.pArgv[1] = Area.x;
		CmdArgs.pArgv[2] = Area.y;
		CmdArgs.pArgv[3] = Area.w;
		CmdArgs.pArgv[4] = Area.h;
		CmdArgs.nArgc = 5;
		Command(CmdArgs);
	}

	MYMSG((_T("[S1D13521] WAIT_DSPE_TRG\r\n")));
	CmdArgs.bCmd = 0x28;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	MYMSG((_T("[S1D13521] WAIT_DSPE_FREND\r\n")));
	CmdArgs.bCmd = 0x29;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);
#ifndef	_EBOOT_
/*if (DSPUPD_FULL == g_DspUpdState)
{
	if (WAVEFORM_GU == g_WaveformMode || WAVEFORM_GC == g_WaveformMode)
		loop = 78 + 20;	//delay(780);
	else
		loop = 26 + 20;	//delay(260);
	for (i=0; i<loop; i++)
	{
		wData = RegRead2(0x0338, FALSE);
		if (0 == (wData & (1<<3)))	// [3] Display Frame Busy
			break;
		delay(10);
	}
}*/
#endif	_EBOOT_

	if (DRVESC_WRITE_UPDATE == g_dwDebugLevel)
	{
		MYERR((_T(" UpdateWrite(%d, %d, %d, %d)\r\n"),
			pRect->left, pRect->top, pRect->right, pRect->bottom));
	}

#ifndef	_EBOOT_
	MYMSG((_T("\t%d\r\n"), GetTickCount()-dwStart));
#endif	_EBOOT_
	return TRUE;
}
static BOOL ImageUpdate(PIMAGEDATAS pid)
{
	BOOL bRet;

	bRet = ImageWrite(pid);
	if (bRet)
		bRet = UpdateWrite(pid->pRect);

	if (DRVESC_IMAGE_UPDATE == g_dwDebugLevel)
	{
		MYERR((_T(" ImageUpdate(0x%x, %d, %d, %d, %d)\r\n"),
			(pid->pBuffer ? pid->pBuffer : 0),
			(pid->pRect ? pid->pRect->left, pid->pRect->top, pid->pRect->right, pid->pRect->bottom
			: 0, 0, 0, 0)));
	}

	return bRet;
}

static BOOL DisplayBitmap(PIMAGEFILES pif)
{
	PBYTE pBuffer = pif->pBuffer;
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	RGBQUAD rgbq[16];
	int i, j, offset, x, y;
	CMDARGS CmdArgs;
	RECT rect;

	if (DRVESC_DISPLAY_BITMAP == g_dwDebugLevel)
	{
		MYERR((_T(" DisplayBitmap(0x%x, %d, %d, %d)\r\n"),
			pif->pBuffer, pif->nCount, pif->x, pif->y));
	}

	memcpy(&bfh, pBuffer, sizeof(BITMAPFILEHEADER));
	MYMSG((_T("bfh.bfType      = (0x%x)\r\n"), bfh.bfType));
	MYMSG((_T("bfh.bfSize      = (0x%x)\r\n"), bfh.bfSize));
	MYMSG((_T("bfh.bfReserved1 = (0x%x)\r\n"), bfh.bfReserved1));
	MYMSG((_T("bfh.bfReserved2 = (0x%x)\r\n"), bfh.bfReserved2));
	MYMSG((_T("bfh.bfOffBits   = (0x%x)\r\n"), bfh.bfOffBits));
	if ('B' != LOBYTE(bfh.bfType) && 'M' != LOBYTE(bfh.bfType))
	{
		MYERR((_T("[ERR] \"BM\" != bfh.bfType(0x%x)\r\n"), bfh.bfType));
		return FALSE;
	}
	if (pif->nCount != bfh.bfSize)
	{
		MYERR((_T("[ERR] pif->nCount(0x%x) != bfh.bfSize(0x%x)\r\n"), pif->nCount, bfh.bfSize));
		return FALSE;
	}
	pBuffer += sizeof(BITMAPFILEHEADER);

	memcpy(&bih, pBuffer, sizeof(BITMAPINFOHEADER));
	MYMSG((_T("bih.biSize          = (0x%x)\r\n"), bih.biSize));
	MYMSG((_T("bih.biWidth         = (%d)\r\n"), bih.biWidth));
	MYMSG((_T("bih.biHeight        = (%d)\r\n"), bih.biHeight));
	MYMSG((_T("bih.biPlanes        = (0x%x)\r\n"), bih.biPlanes));
	MYMSG((_T("bih.biBitCount      = (0x%x)\r\n"), bih.biBitCount));
	MYMSG((_T("bih.biCompression   = (0x%x)\r\n"), bih.biCompression));
	MYMSG((_T("bih.biSizeImage     = (0x%x)\r\n"), bih.biSizeImage));
	MYMSG((_T("bih.biXPelsPerMeter = (0x%x)\r\n"), bih.biXPelsPerMeter));
	MYMSG((_T("bih.biYPelsPerMeter = (0x%x)\r\n"), bih.biYPelsPerMeter));
	MYMSG((_T("bih.biClrUsed       = (0x%x)\r\n"), bih.biClrUsed));
	MYMSG((_T("bih.biClrImportant  = (0x%x)\r\n"), bih.biClrImportant));
	if (sizeof(BITMAPINFOHEADER) != bih.biSize)
	{
		MYERR((_T("sizeof(BITMAPINFOHEADER)(0x%x) != bfh.bfSize(0x%x)\r\n"), sizeof(BITMAPINFOHEADER), bih.biSize));
		return FALSE;
	}
	if (4 != bih.biBitCount)
	{
		MYERR((_T("4 != bih.biBitCount = %d\r\n"), bih.biBitCount));
		return FALSE;
	}
	pBuffer += sizeof(BITMAPINFOHEADER);

	for (i=0; i<16; i++)
	{
		memcpy(&rgbq[i], pBuffer, sizeof(RGBQUAD));
		MYMSG((_T("(%d) = r(0x%x), g(0x%x), b(0x%x), (0x%x)\r\n"),
			i, rgbq[i].rgbRed, rgbq[i].rgbGreen, rgbq[i].rgbBlue, rgbq[i].rgbReserved));
		pBuffer += sizeof(RGBQUAD);
	}

	if (ALIGN_CENTER & pif->Align)
		pif->x = abs(S1D13521_ORI_WIDTH - bih.biWidth) / 2;
	else if (ALIGN_RIGHT & pif->Align)
		pif->x = abs(S1D13521_ORI_WIDTH - bih.biWidth);

	if (ALIGN_VCENTER & pif->Align)
		pif->y = abs(S1D13521_ORI_HEIGHT - bih.biHeight) / 2;
	else if (ALIGN_BOTTOM & pif->Align)
		pif->y = abs(S1D13521_ORI_HEIGHT - bih.biHeight);

	MYMSG((_T("[S1D13521] LD_IMG_AREA\r\n")));
	CmdArgs.bCmd = 0x22;
	CmdArgs.pArgv[0] = (S1D13521_FB_BPP<<4);
	CmdArgs.pArgv[1] = pif->x;
	CmdArgs.pArgv[2] = pif->y;
	CmdArgs.pArgv[3] = (WORD)bih.biWidth;
	CmdArgs.pArgv[4] = (WORD)bih.biHeight;
	CmdArgs.nArgc = 5;
	Command(CmdArgs);
	MYMSG((_T("[S1D13521] WR_REG\r\n")));
	CmdArgs.bCmd = 0x11;
	CmdArgs.pArgv[0] = 0x0154;
	CmdArgs.nArgc = 1;
	Command(CmdArgs);
	// Param[2] : Register Write Data
	for (y=bih.biHeight-1; y>=0; y--)
	{
		offset = (y * bih.biWidth);
		for (x=0; x<bih.biWidth; x+=4)
		{
			j = (offset + x) >> 1;
#ifdef	USE_S1D13521_BIGENDIAN
			OUTREG16(&g_pS1D13521Reg->DATA,
				MAKEWORD(pBuffer[j+1], pBuffer[j]));
#else	USE_S1D13521_BIGENDIAN
			OUTREG16(&g_pS1D13521Reg->DATA,
				MAKEWORD(S1D13521_PIXEL_SWAP(pBuffer[j]),
					S1D13521_PIXEL_SWAP(pBuffer[j+1])));
#endif	USE_S1D13521_BIGENDIAN
		}
	}
	MYMSG((_T("[S1D13521] LD_IMG_END\r\n")));
	CmdArgs.bCmd = 0x23;
	CmdArgs.nArgc = 0;
	Command(CmdArgs);

	rect.left = pif->x;
	rect.top = pif->y;
	rect.right = pif->x + bih.biWidth;
	rect.bottom = pif->y + bih.biHeight;
	return UpdateWrite(&rect);
}








void S1d13521Initialize(void *pS1d13521, void *pGPIOReg)
{
	g_pS1D13521Reg = (S1D13521_REG *)pS1d13521;
	g_pGPIOPReg = (S3C6410_GPIO_REG *)pGPIOReg;

#ifdef	_EBOOT_
	initChip();
	initDisplay(TRUE);
#else	_EBOOT_
	g_bBorder = FALSE;
	Sleep(1000);
#endif	_EBOOT_
}

void S1d13521SetDibBuffer(void *pv)
{
	g_lpFrameBuffer = (LPBYTE)pv;
}

void S1d13521PowerHandler(BOOL bOff)
{
	if (bOff && POWER_SLEEP != g_SleepPowerState)
		OUTREG16(&g_pS1D13521Reg->CMD, 0x05);	// SLP
}

ULONG S1d13521DrvEscape(ULONG iEsc,	ULONG cjIn, PVOID pvIn, ULONG cjOut, PVOID pvOut)
{
	int nRetVal = 0;
	POWERSTATE psOld = g_PowerState;

	MYMSG((_T("[S1D13521] DrvEscape(iEsc(0x%08X), cjIn(%d), cjOut(%d))\r\n"), iEsc, cjIn, cjOut));
	switch (iEsc)
	{
	case DRVESC_SET_DEBUGLEVEL:
		g_dwDebugLevel = (DWORD)cjIn;
	case DRVESC_GET_DEBUGLEVEL:
		return g_dwDebugLevel;

	case DRVESC_SET_DIRTYRECT:
		g_bDirtyRect = (BOOL)cjIn;
	case DRVESC_GET_DIRTYRECT:
		return g_bDirtyRect;

	case DRVESC_SET_DSPUPDSTATE:
		if (DSPUPD_LAST > (DSPUPDSTATE)cjIn)
			g_DspUpdState = (DSPUPDSTATE)cjIn;
	case DRVESC_GET_DSPUPDSTATE:
		return g_DspUpdState;

	case DRVESC_SET_BORDER:
		g_bBorder = (BOOL)cjIn;
	case DRVESC_GET_BORDER:
		return g_bBorder;

	case DRVESC_SET_WAVEFORMMODE:
		if (WAVEFORM_LAST > (WAVEFORMMODE)cjIn)
			g_WaveformMode = (WAVEFORMMODE)cjIn;
	case DRVESC_GET_WAVEFORMMODE:
		return g_WaveformMode;

	case DRVESC_SET_POWERSTATE:
		if (POWER_LAST > (POWERSTATE)cjIn && g_PowerState != (POWERSTATE)cjIn)
			g_PowerState = SetPowerState((POWERSTATE)cjIn);
	case DRVESC_GET_POWERSTATE:
		return g_PowerState;

	case DRVESC_SYSTEM_SLEEP:
		{
			g_bSleepDirtyRect = g_bDirtyRect;
			g_SleepPowerState = g_PowerState;

			g_bDirtyRect = FALSE;
			g_PowerState = POWER_SLEEP;
		}
		return 0;
	case DRVESC_SYSTEM_WAKEUP:
		{
			g_bDirtyRect = g_bSleepDirtyRect;
			g_PowerState = g_SleepPowerState;
		}
		return 0;

	case DRVESC_WAIT_HRDY:
		return WaitHrdy(9);
	}

	if (POWER_RUN != g_PowerState)
		psOld = SetPowerState(POWER_RUN);
	switch (iEsc)
	{
	case DRVESC_COMMAND:
		if ((sizeof(CMDARGS) == cjIn) && pvIn)
			nRetVal = Command(*(PCMDARGS)pvIn);
		break;
	case DRVESC_READ_DATA:
		nRetVal = (int)DataRead();
		break;
	case DRVESC_WRITE_BURST:
		if ((sizeof(BLOB) == cjIn) && pvIn)
			nRetVal = BurstWrite((BLOB *)pvIn);
		break;
	case DRVESC_WRITE_REG:
		nRetVal = RegWrite(HIWORD((DWORD)cjIn), LOWORD((DWORD)cjIn));
		break;
	case DRVESC_READ_REG:
		nRetVal = (int)RegRead((WORD)cjIn);
		break;
	case DRVESC_WRITE_SFM:
		if ((sizeof(BLOB) == cjIn) && pvIn)
			nRetVal = SfmWrite((BLOB *)pvIn);
		break;
	case DRVESC_READ_SFM:
		if ((sizeof(BLOB) == cjOut) && pvOut)
			nRetVal = SfmRead((BLOB *)pvOut);
		break;
	case DRVESC_GET_TEMPERATURE:
		nRetVal = RegRead(0x0216);
		break;
	case DRVESC_WRITE_IMAGE:
		if ((sizeof(IMAGEDATAS) == cjIn) && pvIn)
			nRetVal = ImageWrite((PIMAGEDATAS)pvIn);
		break;
	case DRVESC_WRITE_UPDATE:
		if ((sizeof(RECT) == cjIn) && pvIn)
			nRetVal = UpdateWrite((PRECT)pvIn);
		break;
	case DRVESC_IMAGE_UPDATE:
		if ((sizeof(IMAGEDATAS) == cjIn) && pvIn)
			nRetVal = ImageUpdate((PIMAGEDATAS)pvIn);
		break;
	case DRVESC_DISPLAY_BITMAP:
		if ((sizeof(IMAGEFILES) == cjIn) && pvIn)
			nRetVal = DisplayBitmap((PIMAGEFILES)pvIn);
		break;
	}
	if (psOld != g_PowerState)
		SetPowerState(g_PowerState);

	return nRetVal;
}

