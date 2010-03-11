//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    LTC3714.c

Abstract:       Low Control library of LTC3714 PM chip

Functions:


Notes:


--*/

#include <bsp.h>

#include "s3c6410_pm.h"
// This macro must be modified on other physical memory system
#define CHECK_IN_PA(t)      (((DWORD)(t)&(DRAM_BASE_PA_START))==(DRAM_BASE_PA_START))

static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;


void LTC3714_Init()
{
    // We assume these GPIOs is used for only changing votage.
    if(CHECK_IN_PA(LTC3714_Init))
    {
        // Assume call from startup.s in OAL
        g_pGPIOReg = (S3C6410_GPIO_REG *)(S3C6410_BASE_REG_PA_GPIO);
    }
    else
    {
        OALMSG(TRUE, (_T("%s\r\n"), _T(__FUNCTION__)));    
        g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    }

#ifdef	OMNIBOOK_VER
	// GPC[2] DVM_SET3, GPC[1] DVM_SET2, GPC[0] DVM_SET1
	g_pGPIOReg->GPCCON = (g_pGPIOReg->GPCCON & ~(0xFFF<<0)) | (0x111<<0);	// Output
	g_pGPIOReg->GPCPUD = (g_pGPIOReg->GPCPUD & ~(0x3F<<0)) | (0x00<<0);		// Pull-up/down disable
	g_pGPIOReg->GPCDAT = (g_pGPIOReg->GPCDAT & ~(0x7<<0)) | (0x0<<0);
#else	//!OMNIBOOK_VER
    //GPIO Setting - For LTC3714 VID
    g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3ff<<22)) | (0x155<<22);

    // Pull-up/dn disable
    g_pGPIOReg->GPNPUD = (g_pGPIOReg->GPNPUD & ~(0x3ff<<22));

    // Latch Control Signal
    // CORE_REG_OE: XhiA9(GPL9),  ARM_REG_LE: XhiA8(GPL8), INT_REG_LE: XhiA10(GPL10)
    g_pGPIOReg->GPLCON1 = (g_pGPIOReg->GPLCON1 & ~(0xfff)) | (0x111);

    g_pGPIOReg->GPLPUD = (g_pGPIOReg->GPLPUD & ~(0x3f<<16));
#endif	OMNIBOOK_VER
}

//////////
// Function Name : LTC3714_VoltageSet
// Function Description : CLKGate_Test in the Normal Mode
// Input :                     uPwr : 1:  ARM Voltage Control,  2: Internal Voltage Control, 3: Both Voltage Control
//                            uVoltage :  1mV
// Output :    None
// Version : v0.1
void LTC3714_VoltageSet(UINT32 uPwr, UINT32 uVoltage, UINT32 uDelay)
{
     int uvtg, uRegValue;

    if(!CHECK_IN_PA(LTC3714_VoltageSet))
    {
        OALMSG(TRUE, (_T("%s(%d,%d,%d)\r\n"), _T(__FUNCTION__), uPwr, uVoltage, uDelay));
    }

#ifdef	OMNIBOOK_VER
	uvtg = uVoltage;

	if (uPwr & SETVOLTAGE_ARM)
	{
		uRegValue = g_pGPIOReg->GPCDAT;	// GPCDAT Register
		switch (uvtg)
		{
		case 1300:
			uRegValue = (uRegValue & ~(0x3<<0)) | ((1<<1)|(1<<0));
			break;
		case 1200:
			uRegValue = (uRegValue & ~(0x3<<0)) | ((0<<1)|(0<<0));
			break;
		case 1100:
			uRegValue = (uRegValue & ~(0x3<<0)) | ((0<<1)|(1<<0));
			break;
		case 1050:
			uRegValue = (uRegValue & ~(0x3<<0)) | ((1<<1)|(0<<0));
			break;
		default:	// 1.20V
			uRegValue = (uRegValue & ~(0x3<<0)) | ((0<<1)|(0<<0));
			break;
		}
		g_pGPIOReg->GPCDAT = uRegValue;	// GPCDAT Register
	}
	if (uPwr & SETVOLTAGE_INTERNAL)
	{
		uRegValue = g_pGPIOReg->GPCDAT;	// GPCDAT Register
		switch (uvtg)
		{
		case 1300:
			uRegValue = (uRegValue & ~(0x1<<2)) | (0<<2);
			break;
		case 1200:
			uRegValue = (uRegValue & ~(0x1<<2)) | (1<<2);
			break;
		default:	// 1.30V
			uRegValue = (uRegValue & ~(0x1<<2)) | (0<<2);
			break;
		}
		g_pGPIOReg->GPCDAT = uRegValue;	// GPCDAT Register
	}
#else	//!OMNIBOOK_VER
     //DWORD    oldGPNDAT;
     //DWORD    oldGPLDAT;
    //////////////////////////////////////////////
    // GPN15  GPN14 GNP13 GPN12 GPN11
    //  VID4    VID3   VID2    VID1   VID0        // Voltage

    //    0     0      0     0     0        // 1.75V
    //    0     0      0     0     1        // 1.70V
    //    0     0      0     1     0        // 1.65V
    //    0     0      0     1     1        // 1.60V
    //    0     0      1     0     0        // 1.55V
    //    0     0      1     0     1        // 1.50V
    //    0     0      1     1     0        // 1.45V
    //    0     0      1     1     1        // 1.40V
    //    0     1      0     0     0        // 1.35V
    //    0     1      0     0     1        // 1.30V
    //    0     1      0     1     0        // 1.25V
    //    0     1      0     1     1        // 1.20V
    //    0     1      1     0     0        // 1.15V
    //    0     1      1     0     1        // 1.10V
    //    0     1      1     1     0        // 1.05V
    //    0     1      1     1     1        // 1.00V
    //   1     0      0   0       0        // 0.975V
    //    1     0      0     0     1        // 0.950V
    //    1      0      0   1       0           // 0.925V
    //    1     0      0     1     1        // 0.900V
    //    1      0      1   0       0           // 0.875V
    //    1     0      1     0     1        // 0.850V
    //    1      0      1   1       0          //  0.825V
    //    1     0      1     1     1        // 0.800V
    //    1      1      0   0       0          //  0.775V
    //    1      1      0   0       1          //  0.750V
    //    1      1      0   1       0          //  0.725V
    //    1      1      0    1      1          //  0.700V
    //    1      1      1    0      0          //  0.675V
    //    1      1      1    0      1          //  0.650V
    //    1      1      1    1      0          //  0.625V
    //    1      1      1    1      1          //  0.600V

       uvtg=uVoltage;

    uRegValue = g_pGPIOReg->GPNDAT;                    // GPNDAT Register

    switch (uvtg)
    {
    case 1750:
          uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(0<<13)|(0<<12)|(0<<11));    //D4~0
          break;

    case 1700:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(0<<13)|(0<<12)|(1<<11));    //D4~0
          break;

    case 1650:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(0<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 1600:
        uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(0<<13)|(1<<12)|(1<<11));    //D4~0
        break;

    case 1550:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(1<<13)|(0<<12)|(0<<11));    //D4~0
         break;

    case 1500:
        uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(1<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 1450:
        uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(1<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 1400:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(0<<14)|(1<<13)|(1<<12)|(1<<11));    //D4~0
         break;

    case 1350:
          uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(0<<13)|(0<<12)|(0<<11));    //D4~0
          break;

    case 1300:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(0<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 1250:
        uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(0<<13)|(1<<12)|(0<<11));    //D4~0
        break;

    case 1200:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(0<<13)|(1<<12)|(1<<11));    //D4~0
         break;

    case 1150:
        uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(1<<13)|(0<<12)|(0<<11));    //D4~0
         break;

    case 1100:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(1<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 1050:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(1<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 1000:
         uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11));    //D4~0
        break;

    case 975:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(0<<13)|(0<<12)|(0<<11));    //D4~0
         break;

    case 950:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(0<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 925:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(0<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 900:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(0<<13)|(1<<12)|(1<<11));    //D4~0
         break;

    case 875:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(1<<13)|(0<<12)|(0<<11));    //D4~0
         break;

    case 850:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(1<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 825:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(1<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 800:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(0<<14)|(1<<13)|(1<<12)|(1<<11));    //D4~0
         break;

    case 775:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(0<<13)|(0<<12)|(0<<11));    //D4~0
         break;

    case 750:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(0<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 725:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(0<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 700:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(0<<13)|(1<<12)|(1<<11));    //D4~0
         break;

    case 675:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(1<<13)|(0<<12)|(0<<11));    //D4~0
         break;

    case 650:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(1<<13)|(0<<12)|(1<<11));    //D4~0
         break;

    case 625:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(1<<13)|(1<<12)|(0<<11));    //D4~0
         break;

    case 600:
        uRegValue=(uRegValue&~(0x1f<<11))|((1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11));    //D4~0
         break;

    default:    // 1.00V
        uRegValue=(uRegValue&~(0x1f<<11))|((0<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11));    //D4~0
         break;
    }

    g_pGPIOReg->GPNDAT = uRegValue;                    // GPNDAT Register

    if(uPwr&SETVOLTAGE_ARM)        //ARM Voltage Control => ARM_REG_LE => Output H => Data Changed
    {
        g_pGPIOReg->GPLDAT = (g_pGPIOReg->GPLDAT & ~(0x1<<8)) | (0x1<<8);

    }

    if(uPwr&SETVOLTAGE_INTERNAL)    // INT Voltage Control
    {
        g_pGPIOReg->GPLDAT = (g_pGPIOReg->GPLDAT & ~(0x1<<10)) | (0x1<<10);

    }

    // Output Enable
    g_pGPIOReg->GPLDAT = (g_pGPIOReg->GPLDAT & ~(0x1<<9)) | (0x1<<9);

    OALStall_us(uDelay);


    g_pGPIOReg->GPLDAT = (g_pGPIOReg->GPLDAT & ~(0x1<<8));
    g_pGPIOReg->GPLDAT = (g_pGPIOReg->GPLDAT & ~(0x1<<10));
#endif	OMNIBOOK_VER
}

