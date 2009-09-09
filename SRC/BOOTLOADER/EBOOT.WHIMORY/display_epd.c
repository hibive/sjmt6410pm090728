
#if	1
#define	_EBOOT_	1
#include "..\\Drivers\\Display_Broadsheet\\S1d13521.c"

// const unsigned char Instruction_Byte_Code[];
#include "display_epd_instructionbytecode.h"
// const unsigned char Rle_Image_BootUp[];
#include "display_epd_rle_image_bootup.h"
// const unsigned char Rle_Image_BootMenu[];
#include "display_epd_rle_image_bootmenu.h"


#define S1D13521_BASE_PA	0x30000000

void EPDInitialize(void)
{
	volatile S1D13521_REG *pS1D13521Reg = (S1D13521_REG *)OALPAtoVA(S1D13521_BASE_PA, FALSE);
	volatile S3C6410_SROMCON_REG *pSROMReg = (S3C6410_SROMCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SROMCON, FALSE);
	volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	pSROMReg->SROM_BW = (pSROMReg->SROM_BW & ~(0xF<<16)) |
						(0<<19) |	// nWBE/nBE(for UB/LB) control for Memory Bank1(0=Not using UB/LB, 1=Using UB/LB)
						(0<<18) |	// Wait enable control for Memory Bank1 (0=WAIT disable, 1=WAIT enable)
						(1<<16);	// Data bus width control for Memory Bank1 (0=8-bit, 1=16-bit)
	pSROMReg->SROM_BC4 = (0x7<<28) |	// Tacs
						(0x7<<24) | // Tcos
						(0xF<<16) | // Tacc
						(0x7<<12) | // Tcoh
						(0x7<< 8) | // Tah
						(0x7<< 4) | // Tacp
						(0x0<< 0);	// PMC

	// GPN[8] : EPD_HRDY(8)
	pGPIOReg->GPNCON = (pGPIOReg->GPNCON & ~(0x3<<16)) | (0x0<<16);	// input mode
	pGPIOReg->GPNPUD = (pGPIOReg->GPNPUD & ~(0x3<<16)) | (0x0<<16);	// pull-up/down disable

	S1d13521Initialize((void *)pS1D13521Reg, (void *)pGPIOReg);
	//S1d13521SetDibBuffer((void *)EBOOT_FRAMEBUFFER_UA_START);
}

#define EOF	(-1)
int RleDecode(const BLOB RleData, PBYTE pOutput)
{
	DWORD i;
	int currChar, prevChar;
	BYTE count, *pTmp;

	prevChar = EOF;	// force next char to be different
	pTmp = pOutput;
	for (i=0; i<RleData.cbSize; )
	{
		currChar = RleData.pBlobData[i++];
		*pTmp++ = currChar;

		if (currChar == prevChar)
		{
			count = RleData.pBlobData[i++];
			while (count > 0)
			{
				*pTmp++ = currChar;
				count--;
			}

			prevChar = EOF;	// force next char to be different
		}
		else
		{
			// no run
			prevChar = currChar;
		}
	}

	return (pTmp - pOutput);
}

void EPDDisplayImage(int nType)
{
	BLOB RleData;
	IMAGEFILES ifs;

	S1d13521DrvEscape(DRVESC_SET_DSPUPDSTATE, DSPUPD_PART, NULL, 0, NULL);
	if (1 == nType)	// BootMenu
	{
		RleData.cbSize = sizeof(Rle_Image_BootMenu) / sizeof(Rle_Image_BootMenu[0]);
		RleData.pBlobData = (PBYTE)Rle_Image_BootMenu;

		S1d13521DrvEscape(DRVESC_SET_WAVEFORMMODE, WAVEFORM_GU, NULL, 0, NULL);
	}
	else	// BootUp
	{
		RleData.cbSize = sizeof(Rle_Image_BootUp) / sizeof(Rle_Image_BootUp[0]);
		RleData.pBlobData = (PBYTE)Rle_Image_BootUp;

		S1d13521DrvEscape(DRVESC_SET_WAVEFORMMODE, WAVEFORM_DU, NULL, 0, NULL);
	}

	ifs.nCount = RleDecode(RleData, (PBYTE)EBOOT_FRAMEBUFFER_UA_START);
	ifs.pBuffer = (PBYTE)EBOOT_FRAMEBUFFER_UA_START;
	ifs.Align = ALIGN_CENTER | ALIGN_VCENTER;
	ifs.x = ifs.y = 0;
	S1d13521DrvEscape(DRVESC_DISPLAY_BITMAP, sizeof(IMAGEFILES), (PVOID)&ifs, 0, NULL);
}

int EPDSerialFlashWrite(void)
{
	BLOB sfmd;
	int nRet;

	sfmd.cbSize = sizeof(Instruction_Byte_Code) / sizeof(Instruction_Byte_Code[0]);
	sfmd.pBlobData = (PBYTE)Instruction_Byte_Code;
	nRet = (int)S1d13521DrvEscape(DRVESC_WRITE_SFM, sizeof(BLOB), (PVOID)&sfmd, 0, NULL);
	return nRet;
}

static BYTE g_bOldPercent = 0xFF;
static RECT g_rect = {100+20, 320+138, 100+20, 320+138+11};
static IMAGEDATAS g_ids = {(PBYTE)EBOOT_FRAMEBUFFER_UA_START, &g_rect};
void EPDShowProgress(DWORD dwCurrent, DWORD dwTotal)
{
	BYTE bPercent;

	bPercent = (BYTE)((dwCurrent * 100.) / dwTotal + 0.5);
	if (g_bOldPercent != bPercent)
	{
		volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

		//EdbgOutputDebugString("%d, ", bPercent);
		if (0 == bPercent)
		{
			S1d13521DrvEscape(DRVESC_SET_WAVEFORMMODE, WAVEFORM_DU, NULL, 0, NULL);
		}
		else if (!(bPercent%10))
		{
			g_ids.pRect->right = g_ids.pRect->left + 34;
			S1d13521DrvEscape(DRVESC_IMAGE_UPDATE, sizeof(IMAGEDATAS), (PVOID)&g_ids, 0, NULL);
			g_ids.pRect->left = g_ids.pRect->right;
		}
		OEMWriteDebugLED( 0, (bPercent%4));

		g_bOldPercent = bPercent;
	}
}
#else
#include <windows.h>

void EPDInitialize(void)
{
}
void EPDDisplayImage(int nType)
{
}
int EPDSerialFlashWrite(void)
{
	return 0;
}
void EPDShowProgress(DWORD dwCurrent, DWORD dwTotal)
{
}
#endif
