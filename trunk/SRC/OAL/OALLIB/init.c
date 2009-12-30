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
//  File:  init.c
//
//  Samsung SMDK6410 board initialization code.
//
#include <bsp.h>

UINT32 g_oalIoCtlClockSpeed;

//------------------------------------------------------------------------------
//
//  Global:  g_oalRtcResetTime
//
//  RTC init time after a RTC reset has occured.
//
SYSTEMTIME g_oalRtcResetTime =
{
#ifdef	OMNIBOOK_VER
	2009,	// wYear
	12,		// wMonth
	2,		// wDayofWeek
	1,		// wDay
	12,		// wHour
#else	//!OMNIBOOK_VER
    2007,     // wYear
    2,        // wMonth
    3,        // wDayofWeek
    14,        // wDay
    12,        // wHour
#endif	OMNIBOOK_VER
    0,        // wMinute
    0,        // wSecond
    0        // wMilliseconds
};

static void InitializeGPIO(void);
static void InitializeCLKGating(void);
static void InitializeBlockPower(void);
static void InitializeCLKSource(void);
static void InitializeRTC(void);
extern void InitializeOTGCLK(void);
#ifdef	OMNIBOOK_VER
#define	PMIC_ADDR	0xCC
extern void IICWriteByte(unsigned long slvAddr, unsigned long addr, unsigned char data);
extern void IICReadByte(unsigned long slvAddr, unsigned long addr, unsigned char *data);
#endif	OMNIBOOK_VER

//------------------------------------------------------------------------------
//
//  OEMSetMemoryAttributes
//
//  OEM function to change memory attributes that isn't supported by kernel.
//  Current implementaion only supports PAGE_WRITECOMBINE.
//
//------------------------------------------------------------------------------
BOOL OEMSetMemoryAttributes (
    LPVOID pVirtAddr,       // Virtual address of region
    LPVOID pPhysAddrShifted,// PhysicalAddress >> 8 (to support up to 40 bit address)
    DWORD  cbSize,          // Size of the region
    DWORD  dwAttributes     // attributes to be set
    )
{
    if (PAGE_WRITECOMBINE != dwAttributes) {
        DEBUGMSG (OAL_ERROR, (L"OEMSetMemoryAttributes: Only PAGE_WRITECOMBINE is supported\r\n"));
        return FALSE;
    }

    return NKVirtualSetAttributes (pVirtAddr, cbSize,
                                  0x4,                  // on ARMv6, this value means Shared-Device. On ARMv4, Non-cacheable & bufferable
                                  0xC,                  // Mask of all cache related bits
                                  &dwAttributes);
}

//------------------------------------------------------------------------------
//
//  Function:  OEMInit
//
//  This is Windows CE OAL initialization function. It is called from kernel
//  after basic initialization is made.
//
void OEMInit()
{
    BOOL *bCleanBootFlag;

    OALMSG(OAL_FUNC, (L"[OAL] ++OEMInit()\r\n"));

    OALMSG(OAL_FUNC, (TEXT("[OAL] S3C6410_APLL_CLK   : %d\n\r"), System_GetAPLLCLK()));
    OALMSG(OAL_FUNC, (TEXT("[OAL] ARMCLK : %d\n\r"), System_GetARMCLK()));
    OALMSG(OAL_FUNC, (TEXT("[OAL] HCLK   : %d\n\r"), System_GetHCLK()));
    OALMSG(OAL_FUNC, (TEXT("[OAL] PCLK   : %d\n\r"), System_GetPCLK()));

    g_oalIoCtlClockSpeed = System_GetARMCLK();

    //CEProcessorType = PROCESSOR_STRONGARM;

    // Set memory size for DrWatson kernel support
    //
    dwNKDrWatsonSize = 128 * 1024;

    // Intialize optional kernel functions. (Processor Extended Feature)
    //
    g_pOemGlobal->pfnIsProcessorFeaturePresent = (PFN_IsProcessorFeaturePresent)OALIsProcessorFeaturePresent;

    // Set OEMSetMemoryAttributes function
    g_pOemGlobal->pfnSetMemoryAttributes = (PFN_SetMemoryAttributes)OEMSetMemoryAttributes;

    // Initialize GPIO
    //
    InitializeGPIO();

    // Turn Off all Debug LED
    //
    OEMWriteDebugLED(0, 0x0);

    // Initialize Clock Source
    //
    InitializeCLKSource();

    // Initialize Clock Gating
    //
    InitializeCLKGating();

    // Initialize Block Power
    //
    InitializeBlockPower();

#ifdef	OMNIBOOK_VER
	{
		unsigned char buf[2];
		IICReadByte(PMIC_ADDR, 0x00, buf);
		if (!(buf[0] & (1<<3)))	// [bit3] ELDO3 - VDD_OTGI(1.2V)
		{
			buf[0] |= (1<<3);	// on
			IICWriteByte(PMIC_ADDR, 0x00, buf[0]);
			OALMSG(1, (L"VDD_OTGI(1.2V) ON\r\n"));
		}
		IICReadByte(PMIC_ADDR, 0x01, buf);
		if (!(buf[0] & (1<<5)))	// [bit5] ELDO8 - VDD_OTG(3.3V)
		{
			buf[0] |= (1<<5);	// on
			IICWriteByte(PMIC_ADDR, 0x01, buf[0]);
			OALMSG(1, (L"VDD_OTG(3.3V) ON\r\n"));
		}
	}
#endif	OMNIBOOK_VER

    // Initialize OTG PHY Clock
    //
    InitializeOTGCLK();

    // Initialize BCD registers in RTC to known values
    //
    InitializeRTC();
    
    // Initilize cache globals
    //
    OALCacheGlobalsInit();

    OALLogSerial(L"DCache: %d sets, %d ways, %d line size, %d size\r\n",
                g_oalCacheInfo.L1DSetsPerWay, g_oalCacheInfo.L1DNumWays,
                g_oalCacheInfo.L1DLineSize, g_oalCacheInfo.L1DSize);

    OALLogSerial(L"ICache: %d sets, %d ways, %d line size, %d size\r\n",
                g_oalCacheInfo.L1ISetsPerWay, g_oalCacheInfo.L1INumWays,
                g_oalCacheInfo.L1ILineSize, g_oalCacheInfo.L1ISize);

    // Check and Initialize the BSP Args area
    //
    OALArgsInit((BSP_ARGS *)IMAGE_SHARE_ARGS_UA_START);

    // Check clean boot flag in BSP Args area
    //
    OALMSG(OAL_FUNC, (TEXT("[OAL] OEMInit() : BSP Args forces Clean Boot\r\n")));
    bCleanBootFlag = (BOOL *)OALArgsQuery(BSP_ARGS_QUERY_CLEANBOOT);
    if (*bCleanBootFlag)
    {
        // Notify to filesys.exe that we want a clean boot.
        NKForceCleanBoot();
    }
#ifdef	OMNIBOOK_VER
	strcpy(((BSP_ARGS *)IMAGE_SHARE_ARGS_UA_START)->szWinCEBuildDateTime, __TIMESTAMP__);
#endif	OMNIBOOK_VER

    // Initialize Interrupts
    //
    if (!OALIntrInit())
    {
        OALMSG(OAL_ERROR, (L"[OAL:ERR] OEMInit() : failed to initialize interrupts\r\n"));
    }

    // Initialize System Clock
    //
    OALTimerInit(RESCHED_PERIOD, (OEM_COUNT_1MS ), 0);

    // Make high-res Monte Carlo profiling available to the kernel
    //
    g_pOemGlobal->pfnProfileTimerEnable = OEMProfileTimerEnable;
    g_pOemGlobal->pfnProfileTimerDisable = OEMProfileTimerDisable;

    // Initialize the KITL connection if required
    //
    KITLIoctl(IOCTL_KITL_STARTUP, NULL, 0, NULL, 0, NULL);

    OALMSG(1 && OAL_FUNC, (L"[OAL] --OEMInit()\r\n"));
}

//------------------------------------------------------------------------------

static void InitializeGPIO()
{
    volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

    OALMSG(OAL_FUNC, (L"[OAL] InitializeGPIO()\r\n"));
    // OEM can add GPIO initialization code here
}

static void InitializeCLKGating(void)
{
    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    OALMSG(OAL_FUNC, (L"[OAL] InitializeCLKGating()\r\n"));

    // CAUTION !!!
    // HCLK_IROM, HCLK_MEM1, HCLK_MEM0, HCLK_MFC Should be Always On for power Mode
    // Because we can not expect when Warm Reset will be triggered..

#ifdef	OMNIBOOK_VER
	pSysConReg->HCLK_GATE = (0<<31)		// 3D
							|(1<<30)	// Reserved
#else	//!OMNIBOOK_VER
    pSysConReg->HCLK_GATE = (0x3<<30)    // Reserved
#endif	OMNIBOOK_VER
                            |(0<<29)    // USB Host
                            |(0<<28)    // Security Sub-system
                            |(0<<27)    // SDMA1
                            |(0<<26)    // SDMA0, USB Host (EVT0)
                            |(1<<25)    // Internal ROM                <--- Always On (for Power Mode)
                            |(1<<24)    // DDR1                    <--- Always On
                            |(0<<23)    // DDR0
                            |(1<<22)    // DMC1                    <--- Always On (for Power Mode)
                            |(1<<21)    // DMC0, SROM, OneNAND, NFCON, CFCON    <--- Always On (for Power Mode)
                            |(0<<20)    // USB OTG
                            |(0<<19)    // HSMMC2
                            |(0<<18)    // HSMMC1
                            |(0<<17)    // HSMMC0
                            |(0<<16)    // MDP Interface
                            |(0<<15)    // Direct Host Interface (MSM I/F)
                            |(0<<14)    // Indirect Host Interface
                            |(0<<13)    // DMA1
                            |(0<<12)    // DMA0
                            |(0<<11)    // Jpeg
                            |(0<<10)    // Cam Interface
                            |(0<<9)        // TV Scaler
                            |(0<<8)        // 2D
                            |(0<<7)        // TV Encoder
                            |(1<<6)        // Reserved
                            |(0<<5)        // Post Processor
                            |(0<<4)        // Rotator
#ifdef	DISPLAY_BROADSHEET
                            |(0<<3)        // Display Controller
#else	//!DISPLAY_BROADSHEET
                            |(1<<3)        // Display Controller            <--- Always On
#endif	DISPLAY_BROADSHEET
                            |(0<<2)        // Trust Interrupt Controller
                            |(1<<1)        // Interrupt Controller        <--- Always On
#ifdef	DISPLAY_BROADSHEET
							|(0<<0);		// MFC
#else	//!DISPLAY_BROADSHEET
                            |(1<<0);        // MFC                    <--- Always On (for Power Mode)
#endif	DISPLAY_BROADSHEET

#ifdef	OMNIBOOK_VER
	pSysConReg->PCLK_GATE = (0xF<<28)	// Reserved
							|(0<<27)	// IIC1
							|(0<<26)	// IIS2
							|(1<<25)	// Reserved
#else	//!OMNIBOOK_VER
    pSysConReg->PCLK_GATE = (0x7F<<25)    // Reserved
#endif	OMNIBOOK_VER
                            |(0<<24)    // Security Key
                            |(0<<23)    // CHIP ID
                            |(0<<22)    // SPI1
                            |(0<<21)    // SPI0
                            |(0<<20)    // HSI Receiver
                            |(0<<19)    // HSI Transmitter
                            |(1<<18)    // GPIO                    <--- Always On
                            |(0<<17)    // IIC
                            |(0<<16)    // IIS1
                            |(0<<15)    // IIS0
                            |(0<<14)    // AC97 Interface
                            |(0<<13)    // TZPC
                            |(1<<12)    // Touch Screen & ADC        <--- Always On
                            |(0<<11)    // Keypad
                            |(0<<10)    // IrDA
                            |(0<<9)        // PCM1
                            |(0<<8)        // PCM0
                            |(1<<7)        // PWM Timer                <--- Always On
                            |(1<<6)        // RTC                    <--- Always On
                            #if   (CPU_REVISION == EVT0)
                            |(0<<5)        // WatchDog Timer
                            #elif (CPU_REVISION == EVT1)
                            |(1<<5)        // WatchDog Timer for SW_RST
                            #endif
#if         (DEBUG_PORT == DEBUG_UART0)                // Be Careful to Serial KITL Clock
                            |(0<<4)        // UART3
                            |(0<<3)        // UART2
                            |(0<<2)        // UART1
                            |(1<<1)        // UART0                    <--- Always On
#elif    (DEBUG_PORT == DEBUG_UART1)                // Be Careful to Serial KITL Clock
                            |(0<<4)        // UART3
                            |(0<<3)        // UART2
                            |(1<<2)        // UART1                    <--- Always On
                            |(0<<1)        // UART0
#elif    (DEBUG_PORT == DEBUG_UART2)                // Be Careful to Serial KITL Clock
                            |(0<<4)        // UART3
                            |(1<<3)        // UART2                    <--- Always On
                            |(0<<2)        // UART1
                            |(0<<1)        // UART0
#elif    (DEBUG_PORT == DEBUG_UART3)                // Be Careful to Serial KITL Clock
                            |(1<<4)        // UART3                    <--- Always On
                            |(0<<3)        // UART2
                            |(0<<2)        // UART1
                            |(0<<1)        // UART0
#endif
                            |(0<<0);        // MFC

    pSysConReg->SCLK_GATE = (0x1<<31)    // Reserved
                            |(0<<30)    // USB Host
                            |(0<<29)    // MMC2 48
                            |(0<<28)    // MMC1 48
                            |(0<<27)    // MMC0 48
                            |(0<<26)    // MMC2
                            |(0<<25)    // MMC1
                            |(0<<24)    // MMC0
                            |(0<<23)    // SPI1 48
                            |(0<<22)    // SPI0 48
                            |(0<<21)    // SPI1
                            |(0<<20)    // SPI0
                            |(0<<19)    // DAC 27
                            |(0<<18)    // TV Encoder 27
                            |(0<<17)    // TV Scaler 27
                            |(0<<16)    // TV Scaler
                            |(0<<15)    // Display Controller 27
#ifdef	DISPLAY_BROADSHEET
							|(0<<14)	// Display Controller
#else	//!DISPLAY_BROADSHEET
                            |(1<<14)    // Display Controller            <--- Always On
#endif	DISPLAY_BROADSHEET
                            |(0<<13)    // Post Processor1 27
                            |(0<<12)    // Post Processor0 27
                            |(0<<11)    // Post Processor1
                            |(0<<10)    // Post Processor0
                            |(0<<9)        // Audio1
                            |(0<<8)        // Audio0
                            |(0<<7)        // Security Block
                            |(0<<6)        // IrDA
                            |(1<<5)        // UART0~3                <--- Always On
                            |(0<<4)        // OneNAND
                            |(0<<3)        // MFC
                            |(0<<2)        // Camera Interface
                            |(0<<1)        // Jpeg
                            |(1<<0);        // Reserved
}


static void InitializeBlockPower(void)
{
    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    OALMSG(OAL_FUNC, (L"[OAL] InitializeBlockPower()\r\n"));

    pSysConReg->NORMAL_CFG = (1<<31)        // Reserved
                            |(0<<30)        // IROM Block Off    (Internal 32KB Boot ROM)
                            |(0x1FFF<<17)    // Reserved
#ifdef	OMNIBOOK_VER
                            |(0<<16)		// DOMAIN_ETM On	(JTAG not connected when ETM off)
#else	//!OMNIBOOK_VER
                            |(1<<16)        // DOMAIN_ETM On    (JTAG not connected when ETM off)
#endif	OMNIBOOK_VER
                            |(0<<15)        // DOMAIN_S Off    (SDMA0, SDMA1, Security System)
#ifdef	DISPLAY_BROADSHEET
                            |(0<<14)		// DOMAIN_F On	  (LCD, Post, Rotator)
#else	//!DISPLAY_BROADSHEET
                            |(1<<14)        // DOMAIN_F On    (LCD, Post, Rotator)
#endif	DISPLAY_BROADSHEET
                            |(0<<13)        // DOMAIN_P Off    (TV Scaler, TV Encoder, 2D)
                            |(0<<12)        // DOMAIN_I Off    (Cam I/F, Jpeg)
#ifdef	OMNIBOOK_VER
                            |(1<<11)        // Reserved
                            |(0<<10)        // DOMAIN_G Off    (3D)
#else	//!OMNIBOOK_VER
                            |(0x3<<10)        // Reserved
#endif	OMNIBOOK_VER
                            |(0<<9)            // DOMAIN_V Off    (MFC)
                            |(0x100);        // Reserved
}

static void InitializeCLKSource(void)
{
    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    OALMSG(OAL_FUNC, (L"[OAL] InitializeCLKSource()\r\n"));

    pSysConReg->CLK_SRC = (pSysConReg->CLK_SRC & ~(0xFFFFFFF0))
                            |(0<<31)    // TV27_SEL    -> 27MHz
                            |(0<<30)    // DAC27        -> 27MHz
                            |(0<<28)    // SCALER_SEL    -> MOUT_EPLL
                            |(1<<26)    // LCD_SEL    -> Dout_MPLL
                            |(0<<24)    // IRDA_SEL    -> MOUT_EPLL
                            |(0<<22)    // MMC2_SEL    -> MOUT_EPLL
                            |(0<<20)    // MMC1_SEL    -> MOUT_EPLL
                            |(0<<18)    // MMC0_SEL    -> MOUT_EPLL
                            |(0<<16)    // SPI1_SEL    -> MOUT_EPLL
                            |(0<<14)    // SPI0_SEL    -> MOUT_EPLL
                            |(0<<13)    // UART_SEL    -> MOUT_EPLL
                            |(0<<10)    // AUDIO1_SEL    -> MOUT_EPLL
                            |(0<<7)        // AUDIO0_SEL    -> MOUT_EPLL
                            |(0<<5)        // UHOST_SEL    -> 48MHz
                            |(0<<4);        // MFCCLK_SEL    -> HCLKx2 (0:HCLKx2, 1:MoutEPLL)
}

static void InitializeRTC(void)
{
    volatile S3C6410_RTC_REG *pRTCReg = (S3C6410_RTC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_RTC, FALSE);

    // As per the S3C6410 User Manual, the RTC clock divider should be reset for exact RTC operation.

    // Enable RTC control first
    pRTCReg->RTCCON |= (1<<0);

    // Pulse the RTC clock divider reset
    pRTCReg->RTCCON |= (1<<3);
    pRTCReg->RTCCON &= ~(1<<3);

#ifndef	OMNIBOOK_VER
    // The value of BCD registers in the RTC are undefined at reset. Set them to a known value
    pRTCReg->BCDSEC  = 0;
    pRTCReg->BCDMIN  = 0;
    pRTCReg->BCDHOUR = 0;
    pRTCReg->BCDDATE = 1;
    pRTCReg->BCDDAY  = 1;
    pRTCReg->BCDMON  = 1;
    pRTCReg->BCDYEAR = 0;
#endif	//!OMNIBOOK_VER

    // Disable RTC control.
    pRTCReg->RTCCON &= ~(1<<0);
}


