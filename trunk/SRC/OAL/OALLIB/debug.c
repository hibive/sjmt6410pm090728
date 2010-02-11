//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//------------------------------------------------------------------------------
//
//  File:  debug.c
//
//  This module is provides the interface to the serial port.
//
#include <bsp.h>
#include <nkintr.h>

//------------------------------------------------------------------------------
// Defines

//------------------------------------------------------------------------------
// Externs

//------------------------------------------------------------------------------
// Global Variables

//------------------------------------------------------------------------------
// Local Variables

static volatile S3C6410_UART_REG *g_pUARTReg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;
#ifdef	OMNIBOOK_VER
static volatile BSP_ARGS *g_pBSPArgs = NULL;
#endif	OMNIBOOK_VER

static const UINT32 aSlotTable[16] =
{
    0x0000, 0x0080, 0x0808, 0x0888,
    0x2222, 0x4924, 0x4a52, 0x54aa,
    0x5555, 0xd555, 0xd5d5, 0xddd5,
    0xdddd, 0xdfdd, 0xdfdf, 0xffdf
};

//------------------------------------------------------------------------------
// Local Functions

//------------------------------------------------------------------------------
//
//  Function: OEMInitDebugSerial
//
//  Initializes the debug serial port
//
VOID OEMInitDebugSerial()
{
    UINT32 DivSlot;
    UINT32 uPCLK;
    float Div;

    // Map SFR Address
    //
    if (g_pUARTReg == NULL)
    {
#if    (DEBUG_PORT == DEBUG_UART0)
        // UART0
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART0, FALSE);
#elif (DEBUG_PORT == DEBUG_UART1)
        // UART1
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART1, FALSE);
#elif (DEBUG_PORT == DEBUG_UART2)
        // UART2
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART2, FALSE);
#elif (DEBUG_PORT == DEBUG_UART3)
        // UART3
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART3, FALSE);
#else
        INVALID_DEBUG_PORT        // Error
#endif
    }

    if (g_pGPIOReg == NULL)
    {
        g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
#ifdef	OMNIBOOK_VER
		g_pGPIOReg->GPAPUD &= ~(0xf<<4);	// Pull Up/Down Disable
		g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<8)) | (0x11<<8); 	// GPA[3:2] set to output
		g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x3<<2)) | (0x0<<2);		// Off : High Active

		g_pGPIOReg->GPAPUD &= ~(0xf<<12);	// Pull Up/Down Disable
		g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<24)) | (0x11<<24);	// GPA[7:6] set to output
		g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x3<<6)) | (0x2<<6);		// Off : R#[7], B[6]
#endif	OMNIBOOK_VER
    }

    if (g_pSysConReg == NULL)
    {
        g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
    }

    // UART I/O port initialize
#if    (DEBUG_PORT == DEBUG_UART0)
    // UART0 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<1);        // UART0
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART0 Port Initialize (RXD0 : GPA0, TXD0: GPA1)
    g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<0)) | (0x22<<0);        // GPA0->RXD0, GPA1->TXD0
    g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<0)) | (0x1<<0);            // RXD0: Pull-down, TXD0: pull up/down disable
#elif (DEBUG_PORT == DEBUG_UART1)
    // UART1 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<2);        // UART1
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART1 Port Initialize (RXD1 : GPA4, TXD1: GPA5)
    g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<16)) | (0x22<<16);    // GPA4->RXD1, GPA5->TXD1
    g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<8)) | (0x1<<8);            // RXD1: Pull-down, TXD1: pull up/down disable
#elif (DEBUG_PORT == DEBUG_UART2)
    // UART2 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<3);        // UART2
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART2 Port Initialize (RXD2 : GPAB0, TXD2: GPB1)
    g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<0)) | (0x22<<0);        // GPB0->RXD2, GPB1->TXD2
    g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<0)) | (0x1<<0);            // RXD2: Pull-down, TXD2: pull up/down disable
#elif (DEBUG_PORT == DEBUG_UART3)
    // UART3 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<4);        // UART3
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART3 Port Initialize (RXD3 : GPB2, TXD3: GPB3)
    g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<8)) | (0x22<<8);        // GPB2->RXD3, GPB3->TXD3
    g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<4)) | (0x1<<4);            // RXD3: Pull-down, TXD3: pull up/down disable
#endif

    // Initialize UART
    //
    g_pUARTReg->ULCON = (0<<6)|(0<<3)|(0<<2)|(3<<0);                    // Normal Mode, No Parity, 1 Stop Bit, 8 Bit Data
    g_pUARTReg->UCON = (0<<10)|(1<<9)|(1<<8)|(0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0);    // PCLK divide, Polling Mode
    g_pUARTReg->UFCON = (0<<6)|(0<<4)|(0<<2)|(0<<1)|(0<<0);            // Disable FIFO
    g_pUARTReg->UMCON = (0<<5)|(0<<4)|(0<<0);                        // Disable Auto Flow Control

    uPCLK = System_GetPCLK();

    Div = (float)((float)uPCLK/(16.0*(float)DEBUG_BAUDRATE)) - 1;        //< S3C6410_PCLK is macro code defined in soc_cfg.h
    DivSlot = (UINT32)((Div-(int)Div)*16);

    g_pUARTReg->UBRDIV = (UINT32)Div;                                    // Baud rate
    g_pUARTReg->UDIVSLOT = aSlotTable[DivSlot];
}

//------------------------------------------------------------------------------
//
//  Function: OEMWriteDebugByte
//
//  Transmits a character out the debug serial port.
//
VOID OEMWriteDebugByte(UINT8 ch)
{
    // Wait for TX Buffer Empty
    //
    while (!(g_pUARTReg->UTRSTAT & 0x2));

    // TX Character
    //
    g_pUARTReg->UTXH = ch;
}


//------------------------------------------------------------------------------
//
//  Function: OEMReadDebugByte
//
//  Reads a byte from the debug serial port. Does not wait for a character.
//  If a character is not available function returns "OEM_DEBUG_READ_NODATA".
//

int OEMReadDebugByte()
{
    int ch;

    if (g_pUARTReg->UTRSTAT & 0x1)        // There is received data
    {
        ch = (int)(g_pUARTReg->URXH);
    }
    else        // There no data in RX Buffer;
    {
        ch = OEM_DEBUG_READ_NODATA;
    }

    return ch;
}

#define MAX_OEM_LEDINDEX    ((1<<4) - 1)
// The SMDK6410 Evaluation Platform supports 4 LEDs
void OEMWriteDebugLED(UINT16 Index, DWORD Pattern)
{
#ifdef	OMNIBOOK_VER
	if (g_pBSPArgs == NULL)
		g_pBSPArgs = (BSP_ARGS *)OALPAtoVA(IMAGE_SHARE_ARGS_PA_START, FALSE);
	if (g_pBSPArgs->dwLEDCheck)
		return;
#endif	OMNIBOOK_VER
    if (g_pGPIOReg == NULL)
    {
        // It is first time. Initialize SFR and GPIO set to output
        g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    }
    if(Index == (UINT16)-1)   // Control Descrete LEDs(Masked)
    {
        // Pattern can contain Mask Value and Value;
#ifdef	OMNIBOOK_VER
		g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~((HIWORD(Pattern)&0x1)<<6)) | ((LOWORD(Pattern)&0x1)<<6);
#else	OMNIBOOK_VER
        g_pGPIOReg->GPNDAT = (g_pGPIOReg->GPNDAT & ~((HIWORD(Pattern)&0xf)<<12)) | ((LOWORD(Pattern)&0xf)<<(12));
#endif	OMNIBOOK_VER
    }
    else            
    {
#ifdef	OMNIBOOK_VER
		// GPA7(LED_R#), GPA6(LED_B) : 0->2, 1->3, 2->0, 3->1
		Pattern = (Pattern + 2) % 4;
		g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x3<<6)) | (Pattern<<6);
#else	OMNIBOOK_VER
        g_pGPIOReg->GPNDAT = (g_pGPIOReg->GPNDAT & ~(0xf<<12)) | ((Pattern&0xf)<<12);    
#endif	OMNIBOOK_VER
    }
}


//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugString
//
//  Output unicode string to debug serial port
//
VOID OEMWriteDebugString(LPWSTR string)
{
    while (*string != L'\0') OEMWriteDebugByte((UINT8)*string++);
}


//------------------------------------------------------------------------------

#ifdef	OMNIBOOK_VER
static int ascii2digit(char ch)
{
	if (0x30<=ch && 0x39>=ch)
		return(ch-'0');
	return 0;
}
void GetReleaseDate(LPCSTR lpTimeStamp, LPSYSTEMTIME lpst)
{
	char *szDayOfWeek[] =	{
		"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
	};
	char *szMonth[] =	{
		"Jan","Feb","Mar","Apr","May","Jun",
		"Jul","Aug","Sep","Oct","Nov","Dec"
	};
	char *pTmp;

	pTmp = (char *)lpTimeStamp;	// "Wed Dec 30 15:51:20 2009"
	for (lpst->wDayOfWeek=0; lpst->wDayOfWeek<sizeof(szDayOfWeek)/sizeof(szDayOfWeek[0]); lpst->wDayOfWeek++)
	{
		if (!strncmp(pTmp, szDayOfWeek[lpst->wDayOfWeek], 3))
			break;
	}
	pTmp += 4;	// "Dec 30 15:51:20 2009"
	for (lpst->wMonth=0; lpst->wMonth<sizeof(szMonth)/sizeof(szMonth[0]); lpst->wMonth++)
	{
		if (!strncmp(pTmp, szMonth[lpst->wMonth], 3))
		{
			lpst->wMonth++;
			break;
		}
	}
	pTmp += 4;	// "30 15:51:20 2009"
	lpst->wDay = ascii2digit(*pTmp)*10 + ascii2digit(*(pTmp+1));
	pTmp += 3;	// "15:51:20 2009"
	lpst->wHour = ascii2digit(*pTmp)*10 + ascii2digit(*(pTmp+1));
	pTmp += 3;	// "51:20 2009"
	lpst->wMinute = ascii2digit(*pTmp)*10 + ascii2digit(*(pTmp+1));
	pTmp += 3;	// "20 2009"
	lpst->wSecond = ascii2digit(*pTmp)*10 + ascii2digit(*(pTmp+1));
	pTmp += 3;	// "2009"
	lpst->wYear= ascii2digit(*pTmp)*1000 + ascii2digit(*(pTmp+1))*100
		 + ascii2digit(*(pTmp+2))*10 + ascii2digit(*(pTmp+3));
}
#endif	OMNIBOOK_VER

