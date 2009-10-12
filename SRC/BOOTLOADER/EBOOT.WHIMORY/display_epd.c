
#if	1

#include "display_epd.h"

#define	_EBOOT_	1
#include "..\\Drivers\\Display_Broadsheet\\S1d13521.c"
// const unsigned char Eng_Font_8x16[128][16];
#include "display_epd_eng_font_8x16.h"
// const unsigned char Instruction_Byte_Code[];
#include "display_epd_instructionbytecode.h"
// const unsigned char Rle_Image_BootUp[];
#include "display_epd_rle_image_bootup.h"
// const unsigned char Rle_Image_BootMenu[];
//#include "display_epd_rle_image_bootmenu.h"


#define S1D13521_BASE_PA	0x30000000


#define EOF	(-1)
static int rleDecode(const BLOB RleData, PBYTE pOutput)
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

void EPDDisplayImage(EIMAGE_TYPE eImageType)
{
	BLOB RleData;
	IMAGEFILES ifs;

	S1d13521DrvEscape(DRVESC_SET_DSPUPDSTATE, DSPUPD_PART, NULL, 0, NULL);
	switch (eImageType)
	{
	case IMAGE_BOOTUP:
		delay(100);
		RleData.cbSize = sizeof(Rle_Image_BootUp) / sizeof(Rle_Image_BootUp[0]);
		RleData.pBlobData = (PBYTE)Rle_Image_BootUp;
		S1d13521DrvEscape(DRVESC_SET_WAVEFORMMODE, WAVEFORM_DU, NULL, 0, NULL);
		break;
	case IMAGE_BOOTMENU:
		//delay(500);
		//RleData.cbSize = sizeof(Rle_Image_BootMenu) / sizeof(Rle_Image_BootMenu[0]);
		//RleData.pBlobData = (PBYTE)Rle_Image_BootMenu;
		//S1d13521DrvEscape(DRVESC_SET_WAVEFORMMODE, WAVEFORM_GU, NULL, 0, NULL);
		break;
	default:
		break;
	}

	ifs.nCount = rleDecode(RleData, (PBYTE)EBOOT_FRAMEBUFFER_UA_START);
	ifs.pBuffer = (PBYTE)EBOOT_FRAMEBUFFER_UA_START;
	ifs.Align = ALIGN_CENTER | ALIGN_VCENTER;
	ifs.x = ifs.y = 0;
	S1d13521DrvEscape(DRVESC_DISPLAY_BITMAP, sizeof(IMAGEFILES), (PVOID)&ifs, 0, NULL);
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

#define	FONT_WIDTH		8
#define	FONT_HEIGHT		16
#define	MAX_TEXT_WIDTH	(S1D13521_ORI_WIDTH / FONT_WIDTH)
#define	MAX_TEXT_LINE	(S1D13521_ORI_HEIGHT / FONT_HEIGHT)
static char g_szTextBuf[MAX_TEXT_LINE][MAX_TEXT_WIDTH] = {0,};
static BYTE g_bTextLine = 0;
static RECT g_rectText = {0, 0, S1D13521_ORI_WIDTH, S1D13521_ORI_HEIGHT};
static IMAGEDATAS g_idsText = {(PBYTE)EBOOT_FRAMEBUFFER_UA_START, &g_rectText};
void EPDWriteEngFont8x16(const char *fmt, ...)
{
	char szLine[256], *buf;
	char c, *p, tmp[11];	//int형의 최대크기의 길이는 10자리수 이므로
	unsigned char uc;
	int i, tmp_index, in;
	va_list sarg;

	buf = szLine;
	va_start(sarg, fmt);	//뒤쪽 인수를 가리키기위한 초기화
	for (i=0; fmt[i]; i++)
	{
		if (fmt[i] == '%')
		{
			i++;
			switch (fmt[i])
			{
			case 'B':	//
				uc = (unsigned char)va_arg(sarg, unsigned char);
				if (in = (uc / 16))
					*buf++ = (9 < in) ? (in - 10 + 'A') : (in + '0');
				else
					*buf++ = '0';
				if (in = (uc % 16))
					*buf++ = (9 < in) ? (in - 10 + 'A') : (in + '0');
				else
					*buf++ = '0';
				break;
			case 'c':	//문자
				c = (char)va_arg(sarg, char);
				*buf++ = (c);
				break;
			case 'd':	//정수형
				in = (int)va_arg(sarg, int);
				tmp_index = 0;
				while (in)	//ASCII to integer
				{
					tmp[tmp_index] = (in%10) + '0';
					in /= 10;   
					tmp_index++;
				}
				tmp_index--;	//마지막 배열을 가리키도록 인덱스 값의 1을 빼줌
				while (tmp_index >= 0)
				{
					*buf++ = (tmp[tmp_index]);
					tmp_index--;
				}
				break;
			case 's':	//문자열
				p = (char *)va_arg(sarg, char *);
				while (*p != '\0')
				{
					*buf++ = (*p);
					p++;
				}
				break;
			default:	// % 뒤에 엉뚱한 문자인 경우
				break;
			}
		}
		else
		{
			*buf++ = (fmt[i]);
		}
	}
	va_end(sarg);
	*buf = '\0';

	EdbgOutputDebugString(szLine);

	buf = szLine;
	i = 0;
	while (c = *buf++)
	{
		switch (c)
		{
		case '\r':
		case '\t':
			break;
		case '\n':
			g_szTextBuf[g_bTextLine++][i] = '\0';
			i = 0;
			break;
		default:
			g_szTextBuf[g_bTextLine][i++] = c;
			break;
		}

		if (MAX_TEXT_WIDTH <= i)
		{
			i = 0;
			g_bTextLine++;
		}
		if (MAX_TEXT_LINE <= g_bTextLine)
		{
			for (in=0; in<MAX_TEXT_LINE-1; in++)
				memcpy(g_szTextBuf[in], g_szTextBuf[in+1], MAX_TEXT_WIDTH);
			g_bTextLine = MAX_TEXT_LINE - 1;
		}
	}
}
void EPDFlushEngFont8x16(void)
{
	int y, x, h, w;
	unsigned char code, *ptr, font_data;

	if (WAVEFORM_DU != S1d13521DrvEscape(DRVESC_GET_WAVEFORMMODE, 0, NULL, 0, NULL))
		S1d13521DrvEscape(DRVESC_SET_WAVEFORMMODE, WAVEFORM_DU, NULL, 0, NULL);
	memset(g_idsText.pBuffer, 0xFF, S1D13521_FB_SIZE);	// white clear

	for (y=0; y<MAX_TEXT_LINE; y++)
	{
		for (x=0; x<MAX_TEXT_WIDTH; x++)
		{
			code = g_szTextBuf[y][x];
			if ('\0' == code)
				break;
			if (0x7F < code)
				continue;

			ptr = g_idsText.pBuffer + ((y*FONT_HEIGHT*S1D13521_ORI_WIDTH + x*FONT_WIDTH)>>1);
			for (h=0; h<FONT_HEIGHT; h++)
			{
				font_data = (unsigned char)(Eng_Font_8x16[(code*FONT_HEIGHT) + h] >> 8);
				for (w=0; w<FONT_WIDTH; w+=2)
				{
					switch ((font_data>>w) & 0x3)
					{
					case 0x0:
						*(ptr + 3 - (w>>1)) = 0xFF;
						break;
					case 0x1:
						*(ptr + 3 - (w>>1)) = 0xF0;
						break;
					case 0x2:
						*(ptr + 3 - (w>>1)) = 0x0F;
						break;
					case 0x3:
						*(ptr + 3 - (w>>1)) = 0x00;
						break;
					}
				}

				ptr += (S1D13521_ORI_WIDTH>>1);
			}
		}
	}
	S1d13521DrvEscape(DRVESC_IMAGE_UPDATE, sizeof(IMAGEDATAS), (PVOID)&g_idsText, 0, NULL);
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

#else

#include <windows.h>

void EPDInitialize(void)
{
}
void EPDDisplayImage(EIMAGETYPE eImageType)
{
}
void EPDShowProgress(DWORD dwCurrent, DWORD dwTotal)
{
}

int EPDSerialFlashWrite(void)
{
	return 0;
}

#endif

