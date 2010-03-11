
#include <windows.h>
#include <bsp.h>
#include "keypad.h"

#define	DELAY_LOOP	50

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
	int row, col, tmp;
	volatile int delay;

	pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xFF<<0)) | (0xFF<<0);
	for (col=0; col<8; col++)
	{
		pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xFF<<0)) | ~(1<<col);
		delay = DELAY_LOOP;	while (delay--);
		tmp = ((pGPIOReg->GPKDAT & (0xFF<<8))>>8);
		if (0xFF == tmp)
			continue;

		for (row=0; row<7; row++)
		{
			delay = DELAY_LOOP;	while (delay--);
			if (0 == (pGPIOReg->GPKDAT & (1<<(8+row))))
			{
				KeyData = (row*8) + col + 1;
				break;
			}
		}
	}

	if (KEY_NONE == KeyData)
		return KEY_NONE;

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
	case KEY_T:		return 't';
	case KEY_L:		return 'l';
	case KEY_A:		return 'a';
	case KEY_N:		return 'n';
	case KEY_KE:
	case KEY_VOLDOWN:
		break;
	case KEY_9:		return '9';
	case KEY_2:		return '2';
	case KEY_Y:		return 'y';
	case KEY_K:		return 'k';
	case KEY_S:		return 's';
	case KEY_M:		return 'm';
	case KEY_PP:
	case KEY_VOLUP:
		break;
	case KEY_8:		return '8';
	case KEY_1:		return '1';
	case KEY_U:		return 'u';
	case KEY_J:		return 'j';
	case KEY_Z:		return 'z';
	case KEY_DOT:	return '.';
	case KEY_NP:
	case KEY_RIGHT:
		break;
	case KEY_7:		return '7';
	case KEY_Q:		return 'q';
	case KEY_I:		return 'i';
	case KEY_H:		return 'h';
	case KEY_X:		return 'x';
	case KEY_SLASH:	return '/';
	case KEY_BACK:
	case KEY_UP:
		break;
	case KEY_6:		return '6';
	case KEY_W:		return 'w';
	case KEY_P:		return 'p';
	case KEY_G:		return 'g';
	case KEY_C:		return 'c';
	case KEY_ENTER:	return '\n';
	case KEY_MENU:
		break;
	case KEY_ENTER2:return '\n';
	case KEY_5:		return '5';
	case KEY_E:		return 'e';
	case KEY_O:		return 'o';
	case KEY_F:		return 'f';
	case KEY_V:		return 'v';
	case KEY_SYM:
	case KEY_NP2:
	case KEY_DOWN:
		break;
	case KEY_4:		return '4';
	case KEY_R:		return 'r';
	case KEY_BACKSPACE:	return 0x8;
	case KEY_D:		return 'd';
	case KEY_B:		return 'b';
	case KEY_FONT:
		break;
	case KEY_SPACE:	return ' ';
	case KEY_LEFT:
	case KEY_FN:
	case KEY_SHIFT:
	case KEY_ALT:
	case KEY_HOME:
		break;
	}

	return (BYTE)-1;
}

