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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2005. Samsung Electronics, co. ltd  All rights reserved.

Module Name:

Abstract:

    Platform dependent TOUCH initialization functions

rev:
    2004.4.27    : S3C2440 port 
    2005.05.23    : Magneto porting revision
    2006.06.29    : S3C2443 port
    2007.02.21    : S3C6410 port

Notes:
--*/

#include <bsp.h>
#include <tchddsi.h>
#include "s3c6410_adc_touch_macro.h"

#ifndef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("Touch"), { 
    TEXT("Samples"),TEXT("Calibrate"),TEXT("Stats"),TEXT("Thread"),
    TEXT("TipState"),TEXT("Init"),TEXT(""),TEXT(""),
    TEXT(""),TEXT("Misc"),TEXT("Delays"),TEXT("Timing"),
    TEXT("Alloc"),TEXT("Function"),TEXT("Warning"),TEXT("Error") },
    0x8000              // error
};
#endif


//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
DWORD gIntrTouch = SYSINTR_NOP;
DWORD gIntrTouchChanged = SYSINTR_NOP;

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------
static volatile S3C6410_GPIO_REG * g_pGPIOReg = NULL;
static volatile S3C6410_ADC_REG * g_pADCReg = NULL;
static volatile S3C6410_VIC_REG * g_pVIC0Reg = NULL;
static volatile S3C6410_VIC_REG * g_pVIC1Reg = NULL;
static volatile S3C6410_PWM_REG * g_pPWMReg = NULL;

static BOOL g_bTSP_Initialized = FALSE;
static BOOL g_bTSP_DownFlag = FALSE;
static int g_TSP_CurRate = 0;
static unsigned int g_SampleTick_High;
static unsigned int g_SampleTick_Low;
static CRITICAL_SECTION g_csTouchADC;    // Critical Section for ADC Done

//------------------------------------------------------------------------------
// External Variables
//------------------------------------------------------------------------------
extern "C" const int MIN_CAL_COUNT = 1;

static VOID TSP_VirtualFree(VOID);
static BOOL Touch_Pen_Filtering(int *px, int *py);
static VOID TSP_SampleStop(VOID);

static BOOL
TSP_VirtualAlloc(VOID)
{
    BOOL bRet = TRUE;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    DEBUGMSG(TSP_ZONE_FUNCTION,(_T("[TSP] ++TSP_VirtualAlloc()\r\n")));

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    g_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (g_pGPIOReg == NULL)
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_VirtualAlloc() : g_pGPIOReg Allocation Fail\r\n")));
        bRet = FALSE;
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_ADC;
    g_pADCReg = (S3C6410_ADC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_ADC_REG), FALSE);
    if (g_pADCReg == NULL)
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_VirtualAlloc() : g_pADCReg Allocation Fail\r\n")));
        bRet = FALSE;
        goto CleanUp;
    }
    
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_VIC0;
    g_pVIC0Reg = (S3C6410_VIC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_VIC_REG), FALSE);
    if (g_pVIC0Reg == NULL)
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_VirtualAlloc() : g_pVIC0Reg Allocation Fail\r\n")));
        bRet = FALSE;
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_VIC1;
    g_pVIC1Reg = (S3C6410_VIC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_VIC_REG), FALSE);
    if (g_pVIC1Reg == NULL)
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_VirtualAlloc() : g_pVIC1Reg Allocation Fail\r\n")));
        bRet = FALSE;
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_PWM;
    g_pPWMReg = (S3C6410_PWM_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_PWM_REG), FALSE);
    if (g_pPWMReg == NULL)
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_VirtualAlloc() : g_pPWMReg Allocation Fail\r\n")));
        bRet = FALSE;
        goto CleanUp;
    }

CleanUp:

    if (bRet == FALSE)
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_VirtualAlloc() : Failed\r\n")));

        TSP_VirtualFree();
    }

    TSPMSG((_T("[TSP] --TSP_VirtualAlloc() = %d\r\n"), bRet));

    return bRet;
}

static VOID
TSP_VirtualFree(VOID)
{
    TSPMSG((_T("[TSP] ++TSP_VirtualFree()\r\n")));

    if (g_pGPIOReg)
    {
        MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_SYSCON_REG));
        g_pGPIOReg = NULL;
    }

    if (g_pADCReg)
    {
        MmUnmapIoSpace((PVOID)g_pADCReg, sizeof(S3C6410_ADC_REG));
        g_pADCReg = NULL;
    }

    if (g_pVIC0Reg)
    {
        MmUnmapIoSpace((PVOID)g_pVIC0Reg, sizeof(S3C6410_VIC_REG));
        g_pVIC0Reg = NULL;
    }

    if (g_pVIC1Reg)
    {
        MmUnmapIoSpace((PVOID)g_pVIC1Reg, sizeof(S3C6410_VIC_REG));
        g_pVIC1Reg = NULL;
    }

    if (g_pPWMReg)
    {
        MmUnmapIoSpace((PVOID)g_pPWMReg, sizeof(S3C6410_PWM_REG));
        g_pPWMReg = NULL;
    }

    TSPMSG((_T("[TSP] --TSP_VirtualFree()\r\n")));
}


static VOID
TSP_PowerOn(VOID)
{
    TSPMSG((_T("[TSP] ++TSP_PowerOn()\r\n")));

    g_pADCReg->ADCDLY = ADC_DELAY(TSP_ADC_DELAY);

    g_pADCReg->ADCCON = RESSEL_12BIT | PRESCALER_EN | PRESCALER_VAL(TSP_ADC_PRESCALER) | STDBM_NORMAL;
    
    g_pADCReg->ADCTSC = ADCTSC_WAIT_PENDOWN;
    g_pADCReg->ADCCLRINT = CLEAR_ADC_INT;
    g_pADCReg->ADCCLRWK = CLEAR_ADCWK_INT;

    g_SampleTick_Low = TSP_TIMER_CNT_LOW;
    g_SampleTick_High = TSP_TIMER_CNT_HIGH;

    // Set Divider MUX for Timer3
    SET_TIMER3_DIVIDER_MUX(g_pPWMReg, TSP_TIMER_DIVIDER);    

    g_pPWMReg->TCNTB3  = g_SampleTick_Low;

    // timer3 interrupt disable
    g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) & ~TIMER3_INTERRUPT_ENABLE;

    // timer3 interrupt status clear
    g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) | TIMER3_PENDING_CLEAR;

    TSPMSG((_T("[TSP] --TSP_PowerOn()\r\n")));
}

static VOID
TSP_PowerOff(VOID)
{
    TSPMSG((_T("[TSP] ++TSP_PowerOff()\r\n")));

    TSP_SampleStop();

    // To prevent touch locked after wake up,
    // Wait for ADC Done
    // Do not turn off ADC before its A/D conversion finished
    EnterCriticalSection(&g_csTouchADC);
    // ADC Done in TSP_GETXy()..
    LeaveCriticalSection(&g_csTouchADC);

    g_pADCReg->ADCTSC = UD_SEN_DOWN | YM_SEN_EN | YP_SEN_DIS | XM_SEN_DIS | XP_SEN_DIS | PULL_UP_DIS | AUTO_PST_DIS | XY_PST_NOP;

    // ADC Standby Mode, conversion data will be preserved.
    g_pADCReg->ADCCON |= STDBM_STANDBY; 

    TSPMSG((_T("[TSP] --TSP_PowerOff()\r\n")));
}

static VOID
TSP_SampleStart(VOID)
{
    // timer3 interrupt status clear, Do not use OR/AND operation on TINTC_CSTAT directly
    g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) | TIMER3_PENDING_CLEAR;

    // timer3 interrupt enable, Do not use OR/AND operation on TINTC_CSTAT directly
    g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) | TIMER3_INTERRUPT_ENABLE;

    STOP_TIMER3(g_pPWMReg);

    UPDATE_TCNTB3(g_pPWMReg);
    NOUPDATE_TCNTB3(g_pPWMReg);

    SET_TIMER3_AUTORELOAD(g_pPWMReg);
    START_TIMER3(g_pPWMReg);
}

static VOID
TSP_SampleStop(VOID)
{
    STOP_TIMER3(g_pPWMReg);

    // timer3 interrupt disable, Do not use OR/AND operation on TINTC_CSTAT directly
    g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) & ~TIMER3_INTERRUPT_ENABLE;

    // timer3 interrupt status clear, Do not use OR/AND operation on TINTC_CSTAT directly
    g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) | TIMER3_PENDING_CLEAR;
}

static BOOL
TSP_CalibrationPointGet(TPDC_CALIBRATION_POINT *pTCP)
{
    int cDisplayWidth, cDisplayHeight;
    int CalibrationRadiusX, CalibrationRadiusY;

    TSPMSG((_T("[TSP] ++TSP_CalibrationPointGet()\r\n")));

    cDisplayWidth = pTCP->cDisplayWidth;
    cDisplayHeight = pTCP->cDisplayHeight;

    CalibrationRadiusX = cDisplayWidth  / 20;
    CalibrationRadiusY = cDisplayHeight / 20;

    switch (pTCP->PointNumber)
    {
    case    0:
        pTCP->CalibrationX = cDisplayWidth  / 2;
        pTCP->CalibrationY = cDisplayHeight / 2;
        break;

    case    1:
        pTCP->CalibrationX = CalibrationRadiusX * 2;
        pTCP->CalibrationY = CalibrationRadiusY * 2;
        break;

    case    2:
        pTCP->CalibrationX = CalibrationRadiusX * 2;
        pTCP->CalibrationY = cDisplayHeight - CalibrationRadiusY * 2;
        break;

    case    3:
        pTCP->CalibrationX = cDisplayWidth  - CalibrationRadiusX * 2;
        pTCP->CalibrationY = cDisplayHeight - CalibrationRadiusY * 2;
        break;

    case    4:
        pTCP->CalibrationX = cDisplayWidth - CalibrationRadiusX * 2;
        pTCP->CalibrationY = CalibrationRadiusY * 2;
        break;

    default:
        pTCP->CalibrationX = cDisplayWidth  / 2;
        pTCP->CalibrationY = cDisplayHeight / 2;
        SetLastError(ERROR_INVALID_PARAMETER);
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] TSP_CalibrationPointGet() : ERROR_INVALID_PARAMETER\r\n")));
        return FALSE;
    }

    TSPMSG((_T("[TSP] --TSP_CalibrationPointGet()\r\n")));

    return TRUE;
}

static BOOL
TSP_GetXY(int *px, int *py)
{
    int i,j,k;
    int temp;
    int x[TSP_SAMPLE_NUM], y[TSP_SAMPLE_NUM];
    int dx, dy;
    int TimeOut = 100;  // about 100ms

    EnterCriticalSection(&g_csTouchADC);

    for (i = 0; i < TSP_SAMPLE_NUM; i++)
    {
        g_pADCReg->ADCTSC = ADCTSC_AUTO_ADC;    // Auto Conversion
        g_pADCReg->ADCCON |= ENABLE_START_EN;    // ADC Conversion Start

        while (g_pADCReg->ADCCON & ENABLE_START_EN)
        {    // Wait for Start Bit Cleared
            if(TimeOut-- < 0)
            {
                RETAILMSG(ZONE_ERROR,(TEXT("ADC cannot start\n")));
                goto ADCfails;
            }        
            Sleep(1);
        }

        TimeOut = 100;  // about 100ms
        while (!(g_pADCReg->ADCCON & ECFLG_END))
        {    // Wait for ADC Conversion Ended
            if(TimeOut-- < 0)
            {
                RETAILMSG(ZONE_ERROR,(TEXT("ADC Conversion cannot be done\n")));
                goto ADCfails;
            }        
            Sleep(1);
        }

        x[i] = D_XPDATA_MASK(g_pADCReg->ADCDAT0);
        y[i] = D_YPDATA_MASK(g_pADCReg->ADCDAT1);
    }

ADCfails:
    LeaveCriticalSection(&g_csTouchADC);

    for (j = 0; j < TSP_SAMPLE_NUM -1; ++j)
    {
        for (k = j+1; k < TSP_SAMPLE_NUM; ++k)
        {
            if(x[j]>x[k])
            {
                temp = x[j];
                x[j]=x[k];
                x[k]=temp;
            }

            if(y[j]>y[k])
            {
                temp = y[j];
                y[j]=y[k];
                y[k]=temp;
            }
        }
    }

#ifdef    DETAIL_SAMPLING
    // 8 samples Interpolation (weighted 4 samples)
    *px = (x[2] + ((x[3]+x[4])<<1) + (x[3]+x[4]) + x[5]);
    *py = (y[2] + ((y[3]+y[4])<<1) + (y[3]+y[4]) + y[5]);

    if ((*px & 0x7) > 3) *px = (*px>>3) + 1;
    else *px = *px>>3;

    if ((*py & 0x7) > 3) *py = (*py>>3) + 1;
    else *py = *py>>3;

    dx = x[5] - x[2];
    dy = y[5] - y[2];
#else
    // 2 samples average
    *px = (x[1] + x[2] + 1)>>1;
    *py = (y[1] + y[2] + 1)>>1;

    dx = x[2] - x[1];
    dy = y[2] - y[1];
#endif


    if ((dx > TSP_INVALIDLIMIT) || (dy > TSP_INVALIDLIMIT))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


//---------------------------------------------------------------------------


BOOL
DdsiTouchPanelEnable(VOID)
{
    UINT32 Irq[3];

    TSPMSG((_T("[TSP] ++DdsiTouchPanelEnable()\r\n")));

    if (!g_bTSP_Initialized)    // Map Virtual address and Interrupt at First time Only
    {
        if (!TSP_VirtualAlloc())
        {
            RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] DdsiTouchPanelEnable() : TSP_VirtualAlloc() Failed\r\n")));
            return FALSE;
        }

        // Initialize Critical Section
        InitializeCriticalSection(&g_csTouchADC);

        // Obtain SysIntr values from the OAL for the touch and touch timer interrupts.
        Irq[0] = -1;
        Irq[1] = OAL_INTR_FORCE_STATIC;
        Irq[2] = IRQ_PENDN;
        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(Irq), &gIntrTouch, sizeof(UINT32), NULL))
        {
            RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] DdsiTouchPanelEnable() : IOCTL_HAL_REQUEST_SYSINTR Failed\r\n")));
            gIntrTouch = SYSINTR_UNDEFINED;
            return FALSE;
        }

        Irq[0] = -1;
        Irq[1] = OAL_INTR_FORCE_STATIC;
        Irq[2] = IRQ_TIMER3;
        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(Irq), &gIntrTouchChanged, sizeof(UINT32), NULL))
        {
            RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] DdsiTouchPanelEnable() : IOCTL_HAL_REQUEST_SYSINTR Failed\r\n")));
            gIntrTouchChanged = SYSINTR_UNDEFINED;
            return FALSE ;
        }

        TSPINF((_T("[TSP:INF] DdsiTouchPanelEnable() : gIntrTouch = %d\r\n"), gIntrTouch));
        TSPINF((_T("[TSP:INF] DdsiTouchPanelEnable() : gIntrTouchChanged = %d\r\n"), gIntrTouchChanged));

        g_bTSP_Initialized = TRUE;
    }

    TSP_PowerOn();

    TSPMSG((_T("[TSP] --DdsiTouchPanelEnable()\r\n")));

    return TRUE;
}

VOID
DdsiTouchPanelDisable(VOID)
{
    TSPMSG((_T("[TSP] ++DdsiTouchPanelDisable()\r\n")));

    if (g_bTSP_Initialized)
    {
        TSP_PowerOff();
        //TSP_VirtualFree();    // Do not release Virtual Address... Touch will be use all the time...
        //g_bTSP_Initialized = FALSE;
    }

    TSPMSG((_T("[TSP] --DdsiTouchPanelDisable()\r\n")));
}

BOOL
DdsiTouchPanelGetDeviceCaps(INT iIndex, LPVOID lpOutput)
{
    if ( lpOutput == NULL )
    {
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] DdsiTouchPanelGetDeviceCaps() : ERROR_INVALID_PARAMETER\r\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        ASSERT(0);
        return FALSE;
    }

    switch(iIndex)
    {
    case TPDC_SAMPLE_RATE_ID:
        {
            TPDC_SAMPLE_RATE *pTSR = (TPDC_SAMPLE_RATE*)lpOutput;

            TSPMSG((_T("[TSP] DdsiTouchPanelGetDeviceCaps() : TPDC_SAMPLE_RATE_ID\r\n")));

            pTSR->SamplesPerSecondLow = TSP_SAMPLE_RATE_LOW;
            pTSR->SamplesPerSecondHigh = TSP_SAMPLE_RATE_HIGH;
            pTSR->CurrentSampleRateSetting = g_TSP_CurRate;
        }
        break;
    case TPDC_CALIBRATION_POINT_COUNT_ID:
        {
            TPDC_CALIBRATION_POINT_COUNT *pTCPC = (TPDC_CALIBRATION_POINT_COUNT*)lpOutput;

            TSPMSG((_T("[TSP] DdsiTouchPanelGetDeviceCaps() : TPDC_CALIBRATION_POINT_COUNT_ID\r\n")));

            pTCPC->flags = 0;
            pTCPC->cCalibrationPoints = 5;      // calibrate touch point using 5points
        }
        break;
    case TPDC_CALIBRATION_POINT_ID:
        return(TSP_CalibrationPointGet((TPDC_CALIBRATION_POINT*)lpOutput));
    default:
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] DdsiTouchPanelGetDeviceCaps() : ERROR_INVALID_PARAMETER\r\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        ASSERT(0);
        return FALSE;
    }

    return TRUE;
}

BOOL
DdsiTouchPanelSetMode(INT iIndex, LPVOID  lpInput)
{
    BOOL  bRet = FALSE;

    TSPMSG((_T("[TSP] ++DdsiTouchPanelSetMode(%d)\r\n"), iIndex));

    switch ( iIndex )
    {
    case TPSM_SAMPLERATE_LOW_ID:
        g_TSP_CurRate = 0;
        g_pPWMReg->TCNTB3  = g_SampleTick_Low;
        SetLastError( ERROR_SUCCESS );
        bRet = TRUE;
        break;
    case TPSM_SAMPLERATE_HIGH_ID:
        g_TSP_CurRate = 1;
        g_pPWMReg->TCNTB3  = g_SampleTick_High;
        SetLastError( ERROR_SUCCESS );
        bRet = TRUE;
        break;
    default:
        RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP:ERR] DdsiTouchPanelSetMode() : ERROR_INVALID_PARAMETER\r\n")));
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    }

    TSPMSG((_T("[TSP] --DdsiTouchPanelSetMode() = %d\r\n"), bRet));

    return bRet;
}

VOID
DdsiTouchPanelGetPoint(TOUCH_PANEL_SAMPLE_FLAGS *pTipState, INT *pUncalX, INT *pUncalY)
{
    static int PrevX=0;
    static int PrevY=0;
    int TmpX = 0;
    int TmpY = 0;

    TSPMSG((_T("[TSP] ++DdsiTouchPanelGetPoint()\r\n")));

    if (g_pVIC1Reg->VICRAWINTR & (1<<(PHYIRQ_PENDN-VIC1_BIT_OFFSET)))        // gIntrTouch Interrupt Case
    {
        TSPMSG((_T("[TSP] gIntrTouch(PHYIRQ_PENDN) Case\r\n")));

        *pTipState = TouchSampleValidFlag;

        if ((g_pADCReg->ADCDAT0 & D_UPDOWN_UP)
            || (g_pADCReg->ADCDAT1 & D_UPDOWN_UP))
        {
            TSPMSG((_T("[TSP] Pen Up\r\n")));

            g_bTSP_DownFlag = FALSE;

            g_pADCReg->ADCTSC = ADCTSC_WAIT_PENDOWN;

            *pUncalX = PrevX;
            *pUncalY = PrevY;

            TSP_SampleStop();
        }
        else
        {
            TSPMSG((_T("[TSP] Pen Down\r\n")));

            g_bTSP_DownFlag = TRUE;

            g_pADCReg->ADCTSC = ADCTSC_WAIT_PENUP;

            *pTipState |= TouchSampleIgnore;

            *pUncalX = PrevX;
            *pUncalY = PrevY;

            *pTipState |= TouchSampleDownFlag;

            TSP_SampleStart();
        }

        g_pADCReg->ADCCLRWK = CLEAR_ADCWK_INT;

        InterruptDone(gIntrTouch);        // Not handled in MDD
    }
    else    // gIntrTouchTimer Interrupt Case
    {
        TSPMSG((_T("[TSP] gIntrTouchChanged(PHYIRQ_TIMER3) Case\r\n")));

        // Check for Pen-Up case on the event of timer3 interrupt
        if ((g_pADCReg->ADCDAT0 & D_UPDOWN_UP)
            || (g_pADCReg->ADCDAT1 & D_UPDOWN_UP))
        {
            TSPMSG((_T("[TSP] Pen Up +\r\n")));

            g_bTSP_DownFlag = FALSE;

            g_pADCReg->ADCTSC = ADCTSC_WAIT_PENDOWN;

            *pUncalX = PrevX;
            *pUncalY = PrevY;

            *pTipState = TouchSampleValidFlag;

            TSP_SampleStop();
        }
        else if (g_bTSP_DownFlag)
        {
            if (TSP_GetXY(&TmpX, &TmpY) == TRUE)
            {
#ifdef NEW_FILTER_SCHEME
                if(Touch_Pen_Filtering(&TmpX, &TmpY))
#else
                if(Touch_Pen_Filtering_Legacy(&TmpX, &TmpY))
#endif
                {
                    *pTipState = TouchSampleValidFlag | TouchSampleDownFlag;
                    *pTipState &= ~TouchSampleIgnore;
                }
                else        // Invalid touch pen
                {
//                    *pTipState = TouchSampleValidFlag;
//                    *pTipState |= TouchSampleIgnore;
                    *pTipState = TouchSampleIgnore;
                }

                *pUncalX = PrevX = TmpX;
                *pUncalY = PrevY = TmpY;

                g_pADCReg->ADCTSC = ADCTSC_WAIT_PENUP;
            }
            else
            {
                *pTipState = TouchSampleIgnore;
            }
        }
        else
        {
            RETAILMSG(TSP_ZONE_ERROR,(_T("[TSP] Unknown State\r\n")));

            *pTipState = TouchSampleIgnore;
            TSP_SampleStop();
        }

        // timer3 interrupt status clear, Do not use OR/AND operation on TINTC_CSTAT directly
        g_pPWMReg->TINT_CSTAT = TINT_CSTAT_INTMASK(g_pPWMReg->TINT_CSTAT) | TIMER3_PENDING_CLEAR;

        InterruptDone(gIntrTouchChanged);        // Not Handled in MDD
    }

    TSPMSG((_T("[TSP] --DdsiTouchPanelGetPoint()\r\n")));
}

LONG
DdsiTouchPanelAttach(VOID)
{
    return (0);
}

LONG
DdsiTouchPanelDetach(VOID)
{
    return (0);
}

VOID
DdsiTouchPanelPowerHandler(BOOL bOff)
{
    TSPMSG((_T("[TSP] ++DdsiTouchPanelPowerHandler(%d)\r\n"), bOff));

    if (bOff)
    {
        TSP_PowerOff();
    }
    else
    {
        TSP_PowerOn();

        // Detect proper touch state after Wake Up
        SetInterruptEvent(gIntrTouchChanged);
    }

    TSPMSG((_T("[TSP] --DdsiTouchPanelPowerHandler()\r\n")));
}

static BOOL
Touch_Pen_Filtering(INT *px, INT *py)
{
    BOOL RetVal = TRUE;
    // TRUE  : Valid pen sample
    // FALSE : Invalid pen sample
    INT Filter_Margin;
    static int count = 0;
    static INT x[2], y[2];
    INT TmpX, TmpY;
    INT dx, dy;

    if(*px <0 && *py <0)
    {
        count = 0;
        return FALSE;
    }
    else
    {
        count++;
    }

    if (count > 2)
    {
        // apply filtering rule
        count = 2;

        // average between x,y[0] and *px,y
        TmpX = (x[0] + *px)>>1;
        TmpY = (y[0] + *py)>>1;

        // difference between x,y[1] and TmpX,Y
        dx = (x[1] > TmpX) ? (x[1] - TmpX) : (TmpX - x[1]);
        dy = (y[1] > TmpY) ? (y[1] - TmpY) : (TmpY - y[1]);

        Filter_Margin = (x[1] > x[0]) ? (x[1]-x[0]) : (x[0]-x[1]);
        Filter_Margin += (y[1] > y[0]) ? (y[1]-y[0]) : (y[0]-y[1]);
        Filter_Margin += TSP_FILTER_LIMIT;

        if ((dx > Filter_Margin) || (dy > Filter_Margin)) {
            // Invalid pen sample
            *px = x[1];
            *py = y[1]; // previous valid sample
            RetVal = FALSE;
            count = 0;
        }
        else
        {
            // Valid pen sample
            x[0] = x[1]; y[0] = y[1];
            x[1] = *px; y[1] = *py; // reserve pen samples

            RetVal = TRUE;
        }
    }
    else // (count > 2)
    { // till 2 samples, no filtering rule
        x[0] = x[1]; y[0] = y[1];
        x[1] = *px; y[1] = *py; // reserve pen samples

        RetVal = FALSE;
    }

    return RetVal;
}
static BOOL
Touch_Pen_Filtering_Legacy(INT *px, INT *py)
{    
    BOOL RetVal = TRUE;
    // TRUE  : Valid pen sample
    // FALSE : Invalid pen sample
    static int count = 0;
    static INT x[2], y[2];
    INT TmpX, TmpY;
    INT dx, dy;
    count++;

    if (count > 2)
    {
        // apply filtering rule
        count = 2;

        // average between x,y[0] and *px,y
        TmpX = (x[0] + *px) / 2;
        TmpY = (y[0] + *py) / 2;

        // difference between x,y[1] and TmpX,Y
        dx = (x[1] > TmpX) ? (x[1] - TmpX) : (TmpX - x[1]);
        dy = (y[1] > TmpY) ? (y[1] - TmpY) : (TmpY - y[1]);

        if ((dx > TSP_FILTER_LIMIT) || (dy > TSP_FILTER_LIMIT))
        {
            // Invalid pen sample

            *px = x[1];
            *py = y[1]; // previous valid sample
            RetVal = FALSE;
            count = 0;
        }
        else
        {
            // Valid pen sample
            x[0] = x[1]; y[0] = y[1];
            x[1] = *px; y[1] = *py; // reserve pen samples

            RetVal = TRUE;
        }
    }
    else
    { // till 2 samples, no filtering rule
        x[0] = x[1]; y[0] = y[1];
        x[1] = *px; y[1] = *py; // reserve pen samples

        RetVal = FALSE;
    }

    return RetVal;
}

