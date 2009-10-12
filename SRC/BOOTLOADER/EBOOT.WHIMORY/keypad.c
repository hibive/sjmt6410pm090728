
#include <windows.h>
#include <bsp.h>
#include "keypad.h"

void InitializeKeypad(void)
{
	volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	// KeypadCol : GPL[3:0] - Output, Pull-up/down disable
	pGPIOReg->GPLCON0 = (pGPIOReg->GPLCON0 & ~(0xFFFF<<0)) | (0x1111<<0);
	pGPIOReg->GPLPUD  = (pGPIOReg->GPLPUD & ~(0xFF<<0)) | (0x00<<0);

	// KeypadRow : GPK[11:8] - Input, Pull-up/down disable
	pGPIOReg->GPKCON1 = (pGPIOReg->GPKCON1 & ~(0xFFFF<<0)) | (0x0000<<0);
	pGPIOReg->GPKPUD  = (pGPIOReg->GPKPUD & ~(0xFF<<16)) | (0x00<<16);

	// KeyHold : GPN[6] - Input, Pull-down enable
	pGPIOReg->GPNCON = (pGPIOReg->GPNCON & ~(0x3<<12)) | (0x0<<12);
	pGPIOReg->GPNPUD = (pGPIOReg->GPNPUD & ~(0x3<<12)) | (0x1<<12);
}

EKEY_DATA GetKeypad(void)
{
	volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
	EKEY_DATA KeyData = KEY_NONE;
	int row, col;
	volatile int delay;

	pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xF<<0)) | (0xF<<0);
	delay = 10000;	while (delay--);
	for (row=0; row<4; row++)
	{
		pGPIOReg->GPLDAT = (pGPIOReg->GPLDAT & ~(0xF<<0)) | ~(1<<row);
		delay = 10000;	while (delay--);
		for (col=0; col<4; col++)
		{
			if (0 == (pGPIOReg->GPKDAT & (1<<(8+col))))
			{
				KeyData = (row*4) + col + 1;
				break;
			}
			delay = 10000;	while (delay--);
		}
	}

	if (pGPIOReg->GPNDAT & (0x1<<6))
		KeyData |= KEY_HOLD;

	return KeyData;
}

