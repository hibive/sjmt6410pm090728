
#include <windows.h>
#include <bsp.h>
#include "keypad.h"

#define	DELAY_LOOP	1000

void InitializeKeypad(void)
{
	volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	// KeypadCol : GPL[7:0] - Output, Pull-up/down disable
	pGPIOReg->GPLCON0 = (pGPIOReg->GPLCON0 & ~(0xFFFFFFFF<<0)) | (0x11111111<<0);
	pGPIOReg->GPLPUD  = (pGPIOReg->GPLPUD & ~(0xFFFF<<0)) | (0x0000<<0);

	// KeypadRow : GPK[15:8] - Input, Pull-up/down disable
	pGPIOReg->GPKCON1 = (pGPIOReg->GPKCON1 & ~(0xFFFFFFFF<<0)) | (0x00000000<<0);
	pGPIOReg->GPKPUD  = (pGPIOReg->GPKPUD & ~(0xFFFF<<16)) | (0x0000<<16);
}

EKEY_DATA GetKeypad(void)
{
	volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
	EKEY_DATA KeyData = KEY_NONE;
	int row, col;
	volatile int delay;

	pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xFF<<0)) | (0xFF<<0);
	delay = DELAY_LOOP;	while (delay--);
	for (col=0; col<8; col++)
	{
		pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xFF<<0)) | ~(1<<col);
		delay = DELAY_LOOP;	while (delay--);
		for (row=0; row<7; row++)
		{
			if (0 == (pGPIOReg->GPKDAT & (1<<(8+row))))
			{
				KeyData = (row*8) + col + 1;
				break;
			}
			delay = DELAY_LOOP;	while (delay--);
		}
	}

	row = 7;	// KEY_FN, KEY_SHIFT, KEY_ALT
	col = 0;	// KEY_FN
	pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xFF<<0)) | ~(1<<col);
	delay = DELAY_LOOP;	while (delay--);
	if (0 == (pGPIOReg->GPKDAT & (1<<(8+row))))
		KeyData |= KEY_HOLD;

	return KeyData;
}

BYTE GetKeypad2(void)
{
	switch (GetKeypad())
	{
	case KEY_0:		return '0';
	case KEY_3:		return '3';
	case KEY_T:		return 'T';
	case KEY_L:		return 'L';
	case KEY_A:		return 'A';
	case KEY_N:		return 'N';
	case KEY_KE:
	case KEY_VOLDOWN:
		break;
	case KEY_9:		return '9';
	case KEY_2:		return '2';
	case KEY_Y:		return 'Y';
	case KEY_K:		return 'K';
	case KEY_S:		return 'S';
	case KEY_M:		return 'M';
	case KEY_PP:
	case KEY_VOLUP:
		break;
	case KEY_8:		return '8';
	case KEY_1:		return '1';
	case KEY_U:		return 'U';
	case KEY_J:		return 'J';
	case KEY_Z:		return 'Z';
	case KEY_DOT:	return '.';
	case KEY_NP:
	case KEY_RIGHT:
		break;
	case KEY_7:		return '7';
	case KEY_Q:		return 'Q';
	case KEY_I:		return 'I';
	case KEY_H:		return 'H';
	case KEY_X:		return 'X';
	case KEY_SLASH:	return '/';
	case KEY_BACK:
	case KEY_UP:
		break;
	case KEY_6:		return '6';
	case KEY_W:		return 'W';
	case KEY_P:		return 'P';
	case KEY_G:		return 'G';
	case KEY_C:		return 'C';
	case KEY_ENTER:	return '\n';
	case KEY_MENU:
		break;
	case KEY_ENTER2:return '\n';
	case KEY_5:		return '5';
	case KEY_E:		return 'E';
	case KEY_O:		return 'O';
	case KEY_F:		return 'F';
	case KEY_V:		return 'V';
	case KEY_SYM:
	case KEY_HOME:
	case KEY_DOWN:
		break;
	case KEY_4:		return '4';
	case KEY_R:		return 'R';
	case KEY_BACKSPACE:	return 0x8;
	case KEY_D:		return 'D';
	case KEY_B:		return 'B';
	case KEY_FONT:
		break;
	case KEY_SPACE:	return ' ';
	case KEY_LEFT:
	case KEY_FN:
	case KEY_SHIFT:
	case KEY_ALT:
		break;
	}

	return (BYTE)-1;
}

