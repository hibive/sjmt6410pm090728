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

Module Name:    s3c6410_ldi.c

Abstract:       Libray to control LDI

Functions:


Notes:


--*/

#include <bsp.h>
//#include <windows.h>
//#include <bsp_cfg.h>
//#include <s3c6410.h>
#include "s3c6410_ldi.h"
#include "s3c6410_display_con.h"
#include "s3c6410_display_con_macro.h"
#include "LTS222QV_RGB_dataset.h"
#include "LTV350QV_RGB_dataset.h"
#include "LTM030DK_RGB_dataset.h"

#define LDI_MSG(x)  
#define LDI_INF(x)  
#define LDI_ERR(x)  

#define LCDP_CLK            (1<<1)  // GPC[1]
#define LCDP_MOSI           (1<<2)  // GPC[2]
#define LCDP_nSS            (1<<3)  // GPC[3]

#define LCD_CLK         (1<<5)  // GPC[5]
#define LCD_MOSI        (1<<6)  // GPC[6]
#define LCD_nSS         (1<<7)  // GPC[7]
#define LCDP_CLK_Lo        (g_pGPIOReg->GPCDAT &= ~LCDP_CLK)
#define LCDP_CLK_Hi        (g_pGPIOReg->GPCDAT |= LCDP_CLK)
#define LCDP_MOSI_Lo        (g_pGPIOReg->GPCDAT &= ~LCDP_MOSI)
#define LCDP_MOSI_Hi        (g_pGPIOReg->GPCDAT |= LCDP_MOSI)
#define LCDP_nSS_Lo        (g_pGPIOReg->GPCDAT &= ~LCDP_nSS)
#define LCDP_nSS_Hi        (g_pGPIOReg->GPCDAT |= LCDP_nSS)

#define LCD_CLK_Lo      (g_pGPIOReg->GPCDAT &= ~LCD_CLK)
#define LCD_CLK_Hi      (g_pGPIOReg->GPCDAT |= LCD_CLK)
#define LCD_MOSI_Lo     (g_pGPIOReg->GPCDAT &= ~LCD_MOSI)
#define LCD_MOSI_Hi     (g_pGPIOReg->GPCDAT |= LCD_MOSI)
#define LCD_nSS_Lo      (g_pGPIOReg->GPCDAT &= ~LCD_nSS)
#define LCD_nSS_Hi      (g_pGPIOReg->GPCDAT |= LCD_nSS)

// SYS I/F CSn Main
#define LCD_nCSMain_Lo  (g_pGPIOReg->GPJDAT &= ~(1<<8))
#define LCD_nCSMain_Hi  (g_pGPIOReg->GPJDAT |= (1<<8))
#define LCD_RD_Lo       (g_pGPIOReg->GPJDAT &= ~(1<<7))
#define LCD_RD_Hi       (g_pGPIOReg->GPJDAT |= (1<<7))
#define LCD_RS_Lo       (g_pGPIOReg->GPJDAT &= ~(1<<10))
#define LCD_RS_Hi       (g_pGPIOReg->GPJDAT |= (1<<10))
#define LCD_nWR_Lo      (g_pGPIOReg->GPJDAT &= ~(1<<11))
#define LCD_nWR_Hi      (g_pGPIOReg->GPJDAT |= (1<<11))

#define LCD_DELAY_1MS    30000          // Sufficient under 1Ghz
#define SPI_DELAY        100            // Sufficient under 1Ghz


static volatile S3C6410_SPI_REG *g_pSPIReg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_DISPLAY_REG *g_pDispConReg = NULL;
static LDI_LCD_MODULE_TYPE g_ModuleType;

LDI_ERROR LDI_initialize_register_address(void *pSPIReg, void *pDispConReg, void *pGPIOReg)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_initialize_register_address(0x%08x, 0x%08x, 0x%08x)\n\r"), pSPIReg, pDispConReg, pGPIOReg));

    if (pSPIReg == NULL || pDispConReg == NULL || pGPIOReg == NULL)
    {
        LDI_ERR((_T("[LDI:ERR] LDI_initialize_register_address() : NULL pointer parameter\n\r")));
        error = LDI_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pSPIReg = (S3C6410_SPI_REG *)pSPIReg;
        g_pDispConReg = (S3C6410_DISPLAY_REG *)pDispConReg;
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        LDI_INF((_T("[LDI:INF] g_pSPIReg = 0x%08x\n\r"), g_pSPIReg));
        LDI_INF((_T("[LDI:INF] g_pDispConReg = 0x%08x\n\r"), g_pDispConReg));
        LDI_INF((_T("[LDI:INF] g_pGPIOReg    = 0x%08x\n\r"), g_pGPIOReg));
    }

    LDI_MSG((_T("[LDI]--LDI_initialize_register_address() : %d\n\r"), error));

    return error;
}

LDI_ERROR LDI_set_LCD_module_type(LDI_LCD_MODULE_TYPE ModuleType)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_set_LCD_module_type(%d)\n\r"), ModuleType));

    g_ModuleType = ModuleType;

    LDI_MSG((_T("[LDI]--LDI_set_LCD_module_type() : %d\n\r"), error));

    return error;
}

LDI_ERROR LDI_initialize_LCD_module(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_initialize_LCD_module()\n\r")));

    switch(g_ModuleType)
    {
    case LDI_LTS222QV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_LTS222QV_RGB\n\r"), g_ModuleType));
        LDI_LTS222QV_port_initialize();
        LDI_LTS222QV_reset();
        LDI_LTS222QV_RGB_initialize();
        break;
    case LDI_LTV350QV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_LTV350QV_RGB\n\r"), g_ModuleType));
        LDI_LTV350QV_port_initialize();
        LDI_LTV350QV_reset();
        LDI_LTV350QV_RGB_initialize();
        break;
    case LDI_LTE480WV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_LTE480WV_RGB\n\r"), g_ModuleType));
        LDI_LTE480WV_RGB_port_initialize();
        LDI_LTE480WV_RGB_power_on();
        LDI_LTE480WV_RGB_initialize();
        break;
    case LDI_LTP700WV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_LTP700WV_RGB\n\r"), g_ModuleType));
        LDI_LTP700WV_port_initialize();
        LDI_LTP700WV_reset();
        LDI_LTP700WV_RGB_initialize();
        break;
    case LDI_SMRP_TD043MTEA1_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_SMRP_TD043MTEA1_RGB\n\r"), g_ModuleType));
        LDI_TD043MTEA1_port_initialize();
        LDI_TD043MTEA1_power_on();
        LDI_TD043MTEA1_RGB_initialize();
        break;
    case LDI_SMRP_LTE480WV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_SMRP_LTE480WV_RGB\n\r"), g_ModuleType));
        LDI_SMRP_LTE480WV_RGB_port_initialize();
        LDI_SMRP_LTE480WV_RGB_power_on();
        LDI_SMRP_LTE480WV_RGB_initialize();
        break;
    case LDI_LTM030DK_RGB:
        LDI_INF((_T("[LDI:INF] LDI_initialize_LCD_module() : Type [%d] LDI_LTM030DK_RGB\n\r"), g_ModuleType));
        LDI_LTM030DK_port_initialize();
//        LDI_LTM030DK_SetALC();
        LDI_LTM030DK_RGB_initialize();
        break;
    default:
        LDI_ERR((_T("[LDI:ERR] LDI_initialize_LCD_module() : Unknown LCD Module Type [%d]\n\r"), g_ModuleType));
        error = LDI_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    LDI_MSG((_T("[LDI]--LDI_initialize_LCD_module() : %d\n\r"), error));

    return error;
}

LDI_ERROR LDI_deinitialize_LCD_module(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_deinitialize_LCD_module()\n\r")));

    switch(g_ModuleType)
    {
    case LDI_LTS222QV_RGB:
    case LDI_LTV350QV_RGB:
    case LDI_LTP700WV_RGB:
    case LDI_LTM030DK_RGB:        
        // TODO: Nothing to do with Current Hardware
        break;
    case LDI_LTE480WV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_deinitialize_LCD_module() : Type [%d] LDI_LTE480WV_RGB\n\r"), g_ModuleType));
        LDI_LTE480WV_RGB_power_off();
        break;
    case LDI_SMRP_TD043MTEA1_RGB:
        LDI_INF((_T("[LDI:INF] LDI_deinitialize_LCD_module() : Type [%d] LDI_SMRP_TD043MTEA1_RGB\n\r"), g_ModuleType));
        LDI_TD043MTEA1_power_off();
        break;
    case LDI_SMRP_LTE480WV_RGB:
        LDI_INF((_T("[LDI:INF] LDI_deinitialize_LCD_module() : Type [%d] LDI_SMRP_LTE480WV_RGB\n\r"), g_ModuleType));
        LDI_SMRP_LTE480WV_RGB_power_off();
        break;
    default:
        LDI_ERR((_T("[LDI:ERR] LDI_initialize_LCD_module() : Unknown LCD Module Type [%d]\n\r"), g_ModuleType));
        error = LDI_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    LDI_MSG((_T("[LDI]--LDI_deinitialize_LCD_module() : %d\n\r"), error));

    return error;
}

LDI_ERROR LDI_fill_output_device_information(void *pDevInfo)
{
    LDI_ERROR error = LDI_SUCCESS;
    tDevInfo *pDeviceInfo;

    LDI_MSG((_T("[LDI]++LDI_fill_output_device_information()\n\r")));

    if (pDevInfo == NULL)
    {
        LDI_ERR((_T("[LDI:ERR] LDI_fill_output_device_information() : Null Parameter\n\r")));
        error = DISP_ERROR_NULL_PARAMETER;
        goto CleanUp;
    }

    pDeviceInfo = (tDevInfo *)pDevInfo;

    switch(g_ModuleType)
    {
    case LDI_LTS222QV_RGB:
        LDI_INF((_T("[LDI:INF] Output Devce Type [%d] = LDI_LTS222QV_RGB\n\r"), g_ModuleType));
        pDeviceInfo->RGBOutMode = DISP_18BIT_RGB666_S;
        pDeviceInfo->uiWidth = 240;
        pDeviceInfo->uiHeight = 320;
        pDeviceInfo->VBPD_Value = 7;
        pDeviceInfo->VFPD_Value = 10;
        pDeviceInfo->VSPW_Value = 3;
        pDeviceInfo->HBPD_Value = 2;
        pDeviceInfo->HFPD_Value = 2;
        pDeviceInfo->HSPW_Value = 1;
        pDeviceInfo->VCLK_Polarity = IVCLK_FALL_EDGE;
        pDeviceInfo->HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        pDeviceInfo->VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        pDeviceInfo->VDEN_Polarity = IVDEN_HIGH_ACTIVE;
        pDeviceInfo->PNR_Mode = PNRMODE_RGB_S;
        pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
        pDeviceInfo->VCLK_Direction = CLKDIR_DIVIDED;
        pDeviceInfo->Frame_Rate = 60;
        break;

    case LDI_LTV350QV_RGB:
        LDI_INF((_T("[LDI:INF] Output Devce Type [%d] = LDI_LTV350QV_RGB\n\r"), g_ModuleType));
        pDeviceInfo->RGBOutMode = DISP_16BIT_RGB565_P;
        pDeviceInfo->uiWidth = 320;
        pDeviceInfo->uiHeight = 240;
        pDeviceInfo->VBPD_Value = 5;
        pDeviceInfo->VFPD_Value = 3;
        pDeviceInfo->VSPW_Value = 4;
        pDeviceInfo->HBPD_Value = 5;
        pDeviceInfo->HFPD_Value = 3;
        pDeviceInfo->HSPW_Value = 10;
        pDeviceInfo->VCLK_Polarity = IVCLK_RISE_EDGE;
        pDeviceInfo->HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        pDeviceInfo->VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        pDeviceInfo->VDEN_Polarity = IVDEN_LOW_ACTIVE;
        pDeviceInfo->PNR_Mode = PNRMODE_RGB_P;
        pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
        pDeviceInfo->VCLK_Direction = CLKDIR_DIVIDED;
        pDeviceInfo->Frame_Rate = 60;    // VCLK (Max 10 MHz)
        break;

    case LDI_LTE480WV_RGB:
    case LDI_SMRP_LTE480WV_RGB:
        LDI_INF((_T("[LDI:INF] Output Devce Type [%d] = LDI_(SMRP)LTE480WV_RGB\n\r"), g_ModuleType));
        pDeviceInfo->RGBOutMode = DISP_16BIT_RGB565_P;
        pDeviceInfo->uiWidth = 800;
        pDeviceInfo->uiHeight = 480;
        pDeviceInfo->VBPD_Value = 3;
        pDeviceInfo->VFPD_Value = 5;
        pDeviceInfo->VSPW_Value = 5;
        pDeviceInfo->HBPD_Value = 13;
        pDeviceInfo->HFPD_Value = 8;
        pDeviceInfo->HSPW_Value = 3;
        pDeviceInfo->VCLK_Polarity = IVCLK_FALL_EDGE;
        pDeviceInfo->HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        pDeviceInfo->VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        pDeviceInfo->VDEN_Polarity = IVDEN_HIGH_ACTIVE;
        pDeviceInfo->PNR_Mode = PNRMODE_RGB_P;
        pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
        pDeviceInfo->VCLK_Direction = CLKDIR_DIVIDED;
        pDeviceInfo->Frame_Rate = 60;    // VCLK > 24.5 MHz (Max 35.7 MHz)
        break;

    case LDI_LTP700WV_RGB:
        LDI_INF((_T("[LDI:INF] Output Devce Type [%d] = LDI_LTP700WV_RGB\n\r"), g_ModuleType));
        pDeviceInfo->RGBOutMode = DISP_24BIT_RGB888_P;
        pDeviceInfo->uiWidth = 800;
        pDeviceInfo->uiHeight = 480;
        pDeviceInfo->VBPD_Value = 7;
        pDeviceInfo->VFPD_Value = 5;
        pDeviceInfo->VSPW_Value = 1;
        pDeviceInfo->HBPD_Value = 13;
        pDeviceInfo->HFPD_Value = 8;
        pDeviceInfo->HSPW_Value = 3;
        pDeviceInfo->VCLK_Polarity = IVCLK_FALL_EDGE;
        pDeviceInfo->HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        pDeviceInfo->VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        pDeviceInfo->VDEN_Polarity = IVDEN_HIGH_ACTIVE;
        pDeviceInfo->PNR_Mode = PNRMODE_RGB_P;
        pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
        pDeviceInfo->VCLK_Direction = CLKDIR_DIVIDED;
        pDeviceInfo->Frame_Rate = 60;
        break;

    case LDI_SMRP_TD043MTEA1_RGB:
        LDI_INF((_T("[LDI:INF] Output Devce Type [%d] = LDI_SMRP_TD043MTEA1_RGB\n\r"), g_ModuleType));
        pDeviceInfo->RGBOutMode = DISP_16BIT_RGB565_P;
        pDeviceInfo->uiWidth = 800;
        pDeviceInfo->uiHeight = 480;
        pDeviceInfo->VBPD_Value = 30;
        pDeviceInfo->VFPD_Value = 10;
        pDeviceInfo->VSPW_Value = 5;
        pDeviceInfo->HBPD_Value = 200;
        pDeviceInfo->HFPD_Value = 40;
        pDeviceInfo->HSPW_Value = 16;
        pDeviceInfo->VCLK_Polarity = IVCLK_FALL_EDGE;
        pDeviceInfo->HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        pDeviceInfo->VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        pDeviceInfo->VDEN_Polarity = IVDEN_HIGH_ACTIVE;
        pDeviceInfo->PNR_Mode = PNRMODE_RGB_P;
        pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
        pDeviceInfo->VCLK_Direction = CLKDIR_DIVIDED;
        pDeviceInfo->Frame_Rate = 60;
        break;
        
    case LDI_LTM030DK_RGB:
        LDI_INF((_T("[LDI:INF] Output Devce Type [%d] = LDI_LTM030DK_RGB\n\r"), g_ModuleType));
        pDeviceInfo->RGBOutMode = DISP_16BIT_RGB565_P;
        pDeviceInfo->uiWidth = 480;
        pDeviceInfo->uiHeight = 800;
        pDeviceInfo->VBPD_Value = 3;
        pDeviceInfo->VFPD_Value = 6;
        pDeviceInfo->VSPW_Value = 2;
        pDeviceInfo->HBPD_Value = 4;
        pDeviceInfo->HFPD_Value = 6;
        pDeviceInfo->HSPW_Value = 6;
        pDeviceInfo->VCLK_Polarity = IVCLK_RISE_EDGE;
        pDeviceInfo->HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        pDeviceInfo->VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        pDeviceInfo->VDEN_Polarity = IVDEN_HIGH_ACTIVE;
        pDeviceInfo->PNR_Mode = PNRMODE_RGB_P;
        pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
        pDeviceInfo->VCLK_Direction = CLKDIR_DIVIDED;
        pDeviceInfo->Frame_Rate = 60;
        break;

    default:
        LDI_INF((_T("[LDI:ERR] LDI_fill_output_device_information() : Unknown device type [%d]\n\r"), g_ModuleType));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

CleanUp:

    LDI_MSG((_T("[LDI]--LDI_fill_output_device_information()\n\r")));

    return error;
}


// Port_Init -> SPI Init -> LDI REset LDI Init ->
void LDI_LTM030DK_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++%s()\n\r"), _T(__FUNCTION__)));
    
//    (*(volatile unsigned *)0x7410800c)=0;    //Must be '0' for Normal-path instead of By-pass
    // nReset    : GPN[5]


    // set GPIO Initial Value to High
    g_pGPIOReg->GPNDAT |= (1<<5);            // nReset

    // Pull Up/Down Disable
    g_pGPIOReg->GPNPUD &= ~(0x3<<10);                // nReset

    // Set GPIO direction to output
    g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<10)) | (1<<10);        // nReset

    // nReset    : GPN[5]
    g_pGPIOReg->GPNDAT |= (1<<5);        // nReset High
    DelayLoop_1ms(50);                    // 10 ms

    g_pGPIOReg->GPNDAT &= ~(1<<5);    // nReset Low
    DelayLoop_1ms(10);                    // 10 ms

    g_pGPIOReg->GPNDAT |= (1<<5);        // nReset High
    DelayLoop_1ms(10);                    // 10 ms

    LDI_MSG((_T("[LDI]--%s()\n\r"), _T(__FUNCTION__)));
}

static void LDI_LTM030DK_spi_port_enable(void)
{
    LDI_MSG((_T("[LDI]++%s()\n\r"), _T(__FUNCTION__)));

    // Clk        : GPC[5]
    // MOSI        : GPC[6]
    g_pGPIOReg->SPCON &= ~(0x3<<0);    // Host I/F
//    g_pGPIOReg->GPICON = 0xaaaaaaaa;        // VD Signal
//    g_pGPIOReg->GPJCON = 0xaaaaaaaa;        // VD Signal

    //Set PWM GPIO to control Back-light  Regulator  Shotdown Pin
    g_pGPIOReg->GPFCON = (g_pGPIOReg->GPFCON & ~(0x3<<30)) | (0x1<<30); //GPFCON[31:30] -> Output
    g_pGPIOReg->GPFDAT &= ~(0x1<<15); //GPFDAT[15] -> Low
    g_pGPIOReg->GPFPUD &= ~(0x3<<30);   // GPFPUD[31:30] -> PullUp Down Disable

    // set GPIO Initial Value for SPI
    g_pGPIOReg->GPCDAT |= ((1<<1)|(1<<2)|(1<<3));        // Clk, MOSI, CS -> High

    // Set GPIO direction to output
    g_pGPIOReg->GPCCON = (g_pGPIOReg->GPCCON & ~(0xfff<<4)) | (0x111<<4) ;    // Clk, MOSI, CS

    // Pull Up/Down Disable
    g_pGPIOReg->GPCPUD &= ~((0x3<<2)|(0x3<<4)|(0x3<<6));    // Clk, MOSI, CS
    
    DelayLoop_1ms(5);                    // 5 ms
//    g_pDispConReg->SIFCCON0 = 0x01;    // RS:LO nCS:HI nOE:HI nWE:HI, Manual


    LDI_MSG((_T("[LDI]--%s()\n\r"), _T(__FUNCTION__)));

}

LDI_ERROR LDI_LTM030DK_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;
    int i=0;

    LDI_MSG((_T("[LDI]++%s()\n\r"), _T(__FUNCTION__)));

    LDI_LTM030DK_spi_port_enable();
   
    while(1)
    {
        LDI_LTM030DK_write(LTM030DK_RGB_initialize[i][0], LTM030DK_RGB_initialize[i][1]);
        if (LTM030DK_RGB_initialize[i][2]) DelayLoop_1ms(LTM030DK_RGB_initialize[i][2]);

        i++;

        if (LTM030DK_RGB_initialize[i][0] == 0 && LTM030DK_RGB_initialize[i][1] == 0) break;
    }

    g_pGPIOReg->SPCON |= 0x1;        // RGB I/F   

    LDI_MSG((_T("[LDI]--%s() : %d\n\r"), _T(__FUNCTION__), error));

    return error;
}

static void LDI_LTS222QV_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTS222QV_port_initialize()\n\r")));

    // nReset    : GPN[5]

    // set GPIO Initial Value to High
    g_pGPIOReg->GPNDAT |= (1<<5);            // nReset

    // Pull Up/Down Disable
    g_pGPIOReg->GPNPUD &= ~(0x3<<10);                // nReset

    // Set GPIO direction to output
    g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<10)) | (1<<10);        // nReset

    LDI_MSG((_T("[LDI]--LDI_LTS222QV_port_initialize()\n\r")));
}

static void LDI_LTS222QV_spi_port_enable(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTS222QV_spi_port_enable()\n\r")));

    // Clk        : GPC[5]
    // MOSI        : GPC[6]

    g_pGPIOReg->SPCON &= ~(0x3<<0);    // Host I/F
    g_pGPIOReg->GPICON = 0xaaaaaaaa;
    g_pGPIOReg->GPJCON = 0xaaaaaaaa;

    // set GPIO Initial Value
    g_pGPIOReg->GPCDAT |= ((1<<5)|(1<<6));        // Clk, MOSI -> High

    // Pull Up/Down Disable
    g_pGPIOReg->GPCPUD &= ~((0x3<<10)|(0x3<<12));    // Clk, MOSI

    // Set GPIO direction to output
    g_pGPIOReg->GPCCON = (g_pGPIOReg->GPCCON & ~(0xff<<20)) | (0x11<<20);    // Clk, MOSI

    g_pDispConReg->SIFCCON0 = 0x01;    // RS:LO nCS:HI nOE:HI nWE:HI, Manual

    LDI_MSG((_T("[LDI]--LDI_LTS222QV_spi_port_enable()\n\r")));

}

static void LDI_LTS222QV_spi_port_disable(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTS222QV_spi_port_disable()\n\r")));

    g_pGPIOReg->SPCON |= 0x1;        // RGB I/F
    g_pGPIOReg->GPCDAT |= ((1<<5)|(1<<6));        // Clk, MOSI -> High

    g_pGPIOReg->GPICON = 0x0;
    g_pGPIOReg->GPJCON = 0xaaaaaaa0;

    LDI_MSG((_T("[LDI]--LDI_LTS222QV_spi_port_disable()\n\r")));
}

static void LDI_LTS222QV_reset(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTS222QV_reset()\n\r")));

    // nReset    : GPN[5]
    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High
    DelayLoop_1ms(10);                    // 10 ms

    CLEAR_TFT_LCD_nRESET(g_pGPIOReg);   // nReset Low
    DelayLoop_1ms(10);                    // 10 ms

    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High
    DelayLoop_1ms(10);                    // 10 ms

    LDI_MSG((_T("[LDI]--LDI_LTS222QV_reset()\n\r")));
}

static LDI_ERROR LDI_LTS222QV_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;
    int i=0;

    LDI_MSG((_T("[LDI]++LDI_LTS222QV_RGB_initialize()\n\r")));

    LDI_LTS222QV_spi_port_enable();

    while(1)
    {
        LDI_LTS222QV_write(LTS222QV_RGB_initialize[i][0], LTS222QV_RGB_initialize[i][1]);
        if (LTS222QV_RGB_initialize[i][2]) DelayLoop_1ms(LTS222QV_RGB_initialize[i][2]);

        i++;

        if (LTS222QV_RGB_initialize[i][0] == 0 && LTS222QV_RGB_initialize[i][1] == 0) break;
    }

    LDI_LTS222QV_spi_port_disable();

    LDI_MSG((_T("[LDI]--LDI_LTS222QV_RGB_initialize() : %d\n\r"), error));

    return error;
}



static void LDI_LTV350QV_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTV350QV_port_initialize()\n\r")));

    // nReset    : GPN[5]
    // Clk       : GPC[5]
    // MOSI      : GPC[6]
    // nSS       : GPC[7]

    // set GPIO Initial Value to High

    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High    
    g_pGPIOReg->GPCDAT |= ((1<<5)|(1<<6)|(1<<7));    // Clk, MOSI, nSS

    // Pull Up/Down Disable
    g_pGPIOReg->GPNPUD &= ~(0x3<<10);    // nReset
    g_pGPIOReg->GPCPUD &= ~((0x3<<10)|(0x3<<12)|(0x3<<14));    // Clk, MOSI, nSS

    // Set GPIO direction to output
    g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<10)) | (1<<10);    // nReset
    g_pGPIOReg->GPCCON = (g_pGPIOReg->GPCCON & ~(0xfff<<20)) | (0x111<<20);    // Clk, MOSI, nSS

    LDI_MSG((_T("[LDI]--LDI_LTV350QV_port_initialize()\n\r")));
}

static void LDI_LTV350QV_reset(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTV350QV_reset()\n\r")));

    // nReset    : GPN[5]

    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High    
    DelayLoop_1ms(10);                    // 10 ms

    CLEAR_TFT_LCD_nRESET(g_pGPIOReg);         // nReset Low
    DelayLoop_1ms(5);                    // 5 ms

    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High    
    DelayLoop_1ms(5);                    // 5 ms

    LDI_MSG((_T("[LDI]--LDI_LTV350QV_reset()\n\r")));
}

static LDI_ERROR LDI_LTV350QV_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;
    int i=0;

    LDI_MSG((_T("[LDI]++LDI_LTV350QV_RGB_initialize()\n\r")));

    while(1)
    {
        LDI_LTV350QV_write(LTV350QV_RGB_initialize[i][0], LTV350QV_RGB_initialize[i][1]);
        if (LTV350QV_RGB_initialize[i][2]) DelayLoop_1ms(LTV350QV_RGB_initialize[i][2]);

        i++;

        if (LTV350QV_RGB_initialize[i][0] == 0 && LTV350QV_RGB_initialize[i][1] == 0) break;
    }

    LDI_MSG((_T("[LDI]--LDI_LTV350QV_RGB_initialize() : %d\n\r"), error));

    return error;
}



static void LDI_LTE480WV_RGB_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTE480WV_RGB_port_initialize()\n\r")));

    // PCI                 : GPN[5]
    // LCD_PANNEL_ON    : N/A in SMDK6410

    // set GPIO Initial Value to Low
    CLEAR_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High        

    // Pull Up/Down Disable
    g_pGPIOReg->GPNPUD &= ~(0x3<<10);        // PCI

    // Set GPIO direction to output
    g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<10)) | (1<<10);    // PCI

    LDI_MSG((_T("[LDI]--LDI_LTE480WV_RGB_port_initialize()\n\r")));
}

static LDI_ERROR LDI_LTE480WV_RGB_power_on(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_LTE480WV_RGB_power_on()\n\r")));

    // PCI                 : GPN[5]
    // LCD_PANNEL_ON    : N/A in SMDK6410

    // Envid Disable
    g_pDispConReg->VIDCON0 &= ~0x3;         //~(ENVID_ENABLE | ENVID_F_ENABLE);    // Direct Off

    // LCD Pannel Power On
    CLEAR_TFT_LCD_nRESET(g_pGPIOReg);       // PCI set to Low
    // TODO: LCD Power On Here
    DelayLoop_1ms(10);                    // tp-sig > 10 ms

    // Envid Enable (Start output through RGB I/F)
    g_pDispConReg->VIDCON0 |= 0x3;        //(ENVID_ENABLE | ENVID_F_ENABLE);
    DelayLoop_1ms(20);                    // tvsync-don > 1 frame (16.7 ms)

    // Set PCI to High
    SET_TFT_LCD_nRESET(g_pGPIOReg);         // PCI set to High

    LDI_MSG((_T("[LDI]--LDI_LTE480WV_RGB_power_on() : %d\n\r"), error));

    return error;
}

static LDI_ERROR LDI_LTE480WV_RGB_power_off(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_LTE480WV_RGB_power_off()\n\r")));

    // PCI                 : GPN[5]
    // LCD_PANNEL_ON    : N/A in SMDK6410

    // Set PCI to Low
    CLEAR_TFT_LCD_nRESET(g_pGPIOReg);   // PCI set to Low
    DelayLoop_1ms(40);                  // twht1 > 2 frame (33.3 ms)
    DelayLoop_1ms(20);                  // tpoff > 1 frame (16.7 ms)

    // Envid Disable
    g_pDispConReg->VIDCON0 &= ~(0x1);    //~(ENVID_F_ENABLE);    // Per Frame Off
    DelayLoop_1ms(20);                    // Wait for frame finished (16.7 ms)
    DelayLoop_1ms(10);                    // tsig0ff-vdd > 10 ms

    // LCD Pannel Power Off
    // TODO: LCD Power Off Here

    LDI_MSG((_T("[LDI]--LDI_LTE480WV_RGB_power_off() : %d\n\r"), error));

    return error;
}

static LDI_ERROR LDI_LTE480WV_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_LTE480WV_RGB_initialize()\n\r")));

    // There is No Power Sequence for LTP480WV

    LDI_MSG((_T("[LDI]--LDI_LTE480WV_RGB_initialize() : %d\n\r"), error));

    return error;
}



static void LDI_LTP700WV_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTP700WV_port_initialize()\n\r")));

    // nReset    : GPN[5]

    // set GPIO Initial Value to High
    g_pGPIOReg->GPNDAT |= (1<<5);    // nReset

    // Pull Up/Down Disable
    g_pGPIOReg->GPNPUD &= ~(0x3<<10);    // nReset

    // Set GPIO direction to output
    g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<10)) | (1<<10);    // nReset

    LDI_MSG((_T("[LDI]--LDI_LTP700WV_port_initialize()\n\r")));
}

static void LDI_LTP700WV_reset(void)
{
    LDI_MSG((_T("[LDI]++LDI_LTP700WV_reset()\n\r")));

    // nReset    : GPN[5]

    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High    
    DelayLoop_1ms(10);                    // 10 ms

    CLEAR_TFT_LCD_nRESET(g_pGPIOReg);       // nReset High   
    DelayLoop_1ms(10);                    // 10 ms

    g_pDispConReg->VIDCON0 |= 0x3;        // VCLK Output enable
    DelayLoop_1ms(100);                    // More than 4 frames..

    SET_TFT_LCD_nRESET(g_pGPIOReg);         // nReset High    
    DelayLoop_1ms(10);                    // 10 ms

    g_pDispConReg->VIDCON0 &= ~0x3;    // VCLK Output disable

    LDI_MSG((_T("[LDI]--LDI_LTP700WV_reset()\n\r")));
}

static LDI_ERROR LDI_LTP700WV_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_LTP700WV_RGB_initialize()\n\r")));

    // There is No Power Sequence for LTP700WV

    LDI_MSG((_T("[LDI]--LDI_LTP700WV_RGB_initialize() : %d\n\r"), error));

    return error;
}



static void LDI_TD043MTEA1_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++LDI_TD043MTEA1_port_initialize()\n\r")));

    // nReset(GREST, STBY)    : GPF[14]
    // LCD_PANNEL_ON        : GPF[13]

    // set GPIO Initial Value to Low
    g_pGPIOReg->GPFDAT &= ~(0x3<<13);    // nReset, LCD_PANNEL_ON

    // Pull Up/Down Disable
    g_pGPIOReg->GPFPUD &= ~(0xf<<26);    // nReset, LCD_PANNEL_ON

    // Set GPIO direction to output
    g_pGPIOReg->GPFCON = (g_pGPIOReg->GPFCON & ~(0xf<<26)) | (5<<26);    // nReset, LCD_PANNEL_ON

    LDI_MSG((_T("[LDI]--LDI_TD043MTEA1_port_initialize()\n\r")));
}

static void LDI_TD043MTEA1_power_on(void)
{
    LDI_MSG((_T("[LDI]++LDI_TD043MTEA1_power_on()\n\r")));

    // nReset(GREST, STBY)    : GPF[14]
    // LCD_PANNEL_ON        : GPF[13]

    // LCD Pannel Power On and nReset
    g_pGPIOReg->GPFDAT &= ~(1<<14);    // nReset Low
    g_pGPIOReg->GPFDAT |= (1<<13);        // LCD_PANNEL_ON High
    DelayLoop_1ms(10);                    // 10 ms

    // Release nReset
    g_pGPIOReg->GPFDAT |= (1<<14);        // nReset High
    DelayLoop_1ms(5);                    // 5 ms

    LDI_MSG((_T("[LDI]--LDI_TD043MTEA1_power_on()\n\r")));
}

static void LDI_TD043MTEA1_power_off(void)
{
    LDI_MSG((_T("[LDI]++LDI_TD043MTEA1_power_off()\n\r")));

    // nReset(GREST, STBY)    : GPF[14]
    // LCD_PANNEL_ON        : GPF[13]

    // LCD Pannel Power Off
    g_pGPIOReg->GPFDAT &= ~(1<<13);    // LCD_PANNEL_ON Low
    g_pGPIOReg->GPFDAT &= ~(1<<14);    // nReset Low
    DelayLoop_1ms(5);                    // 5 ms

    LDI_MSG((_T("[LDI]--LDI_TD043MTEA1_power_off()\n\r")));
}

static LDI_ERROR LDI_TD043MTEA1_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_TD043MTEA1_RGB_initialize()\n\r")));

    // TODO: Initialize Pannel with Serial Interface

    LDI_MSG((_T("[LDI]--LDI_TD043MTEA1_RGB_initialize() : %d\n\r"), error));

    return error;
}



static void LDI_SMRP_LTE480WV_RGB_port_initialize(void)
{
    LDI_MSG((_T("[LDI]++LDI_SMRP_LTE480WV_RGB_port_initialize()\n\r")));

    // PCI                    : GPF[14]
    // LCD_PANNEL_ON        : GPF[13]

    // set GPIO Initial Value to Low
    g_pGPIOReg->GPFDAT &= ~(0x3<<13);    // PCI, LCD_PANNEL_ON

    // Pull Up/Down Disable
    g_pGPIOReg->GPFPUD &= ~(0xf<<26);    // PCI, LCD_PANNEL_ON

    // Set GPIO direction to output
    g_pGPIOReg->GPFCON = (g_pGPIOReg->GPFCON & ~(0xf<<26)) | (5<<26);    // PCI, LCD_PANNEL_ON

    LDI_MSG((_T("[LDI]--LDI_SMRP_LTE480WV_RGB_port_initialize()\n\r")));
}

static LDI_ERROR LDI_SMRP_LTE480WV_RGB_power_on(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_SMRP_LTE480WV_RGB_power_on()\n\r")));

    // PCI                    : GPF[14]
    // LCD_PANNEL_ON        : GPF[13]

    // Envid Disable
    g_pDispConReg->VIDCON0 &= ~0x3;    //~(ENVID_ENABLE | ENVID_F_ENABLE);    // Direct Off

    // LCD Pannel Power On
    g_pGPIOReg->GPFDAT &= ~(1<<14);    // PCI Low
    g_pGPIOReg->GPFDAT |= (1<<13);        // LCD_PANNEL_ON High
    DelayLoop_1ms(10);                    // tp-sig > 10 ms

    // Envid Enable (Start output through RGB I/F)
    g_pDispConReg->VIDCON0 |= 0x3;        //(ENVID_ENABLE | ENVID_F_ENABLE);
    DelayLoop_1ms(20);                    // tvsync-don > 1 frame (16.7 ms)

    // Set PCI to High
    g_pGPIOReg->GPFDAT |= (0x1<<14);    // PCI

    LDI_MSG((_T("[LDI]--LDI_SMRP_LTE480WV_RGB_power_on() : %d\n\r"), error));

    return error;
}

static LDI_ERROR LDI_SMRP_LTE480WV_RGB_power_off(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_SMRP_LTE480WV_RGB_power_off()\n\r")));

    // PCI                    : GPF[14]
    // LCD_PANNEL_ON        : GPF[13]

    // Set PCI to Low
    g_pGPIOReg->GPFDAT &= ~(0x1<<14);    // PCI
    DelayLoop_1ms(40);                    // twht1 > 2 frame (33.3 ms)
    DelayLoop_1ms(20);                    // tpoff > 1 frame (16.7 ms)

    // Envid Disable
    g_pDispConReg->VIDCON0 &= ~(0x1);    //~(ENVID_F_ENABLE);    // Per Frame Off
    DelayLoop_1ms(20);                    // Wait for frame finished (16.7 ms)
    DelayLoop_1ms(10);                    // tsig0ff-vdd > 10 ms

    // LCD Pannel Power Off
    g_pGPIOReg->GPFDAT &= ~(1<<13);    // LCD_PANNEL_ON High

    LDI_MSG((_T("[LDI]--LDI_SMRP_LTE480WV_RGB_power_off() : %d\n\r"), error));

    return error;
}

static LDI_ERROR LDI_SMRP_LTE480WV_RGB_initialize(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_SMRP_LTE480WV_RGB_initialize()\n\r")));

    // There is No Power Sequence for LTP480WV

    LDI_MSG((_T("[LDI]--LDI_SMRP_LTE480WV_RGB_initialize() : %d\n\r"), error));

    return error;
}



static void LDI_LTS222QV_write(unsigned int address, unsigned int data)
{
    int j;

    //LDI_MSG((_T("[LDI]++LDI_LTS222QV_write(0x%08x, 0x%08x)\n\r"), address, data));

    LCD_CLK_Hi;
    LCD_MOSI_Lo;
    DelayLoop(SPI_DELAY);

    g_pDispConReg->SIFCCON0 = 0x11;    // RS:LO nCS:LO nOE:HI nWE:HI, Manual
    g_pDispConReg->SIFCCON0 = 0x13;    // RS:LO nCS:LO nOE:HI nWE:LO, Manual
    DelayLoop(SPI_DELAY);

    for (j = 7; j >= 0; j--)
    {
        LCD_CLK_Lo;

        if ((address >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCD_MOSI_Hi;
        }
        else
        {
            LCD_MOSI_Lo;
        }

        DelayLoop(SPI_DELAY);

        LCD_CLK_Hi;            // CLOCK = High
        DelayLoop(SPI_DELAY);
    }

    LCD_MOSI_Lo;
    DelayLoop(SPI_DELAY);

    g_pDispConReg->SIFCCON0 = 0x11;    // RS:LO nCS:LO nOE:HI nWE:HI, Manual
    g_pDispConReg->SIFCCON0 = 0x01;    // RS:LO nCS:HI nOE:HI nWE:HI, Manual
    DelayLoop(SPI_DELAY);

    g_pDispConReg->SIFCCON0 = 0x11;    // RS:LO nCS:LO nOE:HI nWE:HI, Manual
    g_pDispConReg->SIFCCON0 = 0x13;    // RS:LO nCS:LO nOE:HI nWE:LO, Manual
    DelayLoop(SPI_DELAY);

    for (j = 7; j >= 0; j--)
    {
        LCD_CLK_Lo;                            //    SCL Low

        if ((data >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCD_MOSI_Hi;
        }
        else
        {
            LCD_MOSI_Lo;
        }

        DelayLoop(SPI_DELAY);

        LCD_CLK_Hi;            // CLOCK = High
        DelayLoop(SPI_DELAY);
    }

    g_pDispConReg->SIFCCON0 = 0x11;    // RS:LO nCS:LO nOE:HI nWE:HI, Manual
    g_pDispConReg->SIFCCON0 = 0x01;    // RS:LO nCS:HI nOE:HI nWE:HI, Manual
    DelayLoop(SPI_DELAY);

    LCD_MOSI_Lo;
    DelayLoop(SPI_DELAY);

    //LDI_MSG((_T("[LDI]--LDI_LTS222QV_write()\n\r")));
}

static void LDI_LTV350QV_write(unsigned int address, unsigned int data)
{
    unsigned char dev_id_code = 0x1D;
    int j;

    //LDI_MSG((_T("[LDI]++LDI_LTV350QV_write(0x%08x, 0x%08x)\n\r"), address, data));

    LCD_nSS_Hi;         //    EN = High                    CS high
    LCD_CLK_Hi;                            //    SCL High
    LCD_MOSI_Hi;                            //    Data Low

    DelayLoop(SPI_DELAY);

    LCD_nSS_Lo;         //    EN = Low                CS Low
    DelayLoop(SPI_DELAY);

    for (j = 5; j >= 0; j--)
    {
        LCD_CLK_Lo;                            //    SCL Low

        if ((dev_id_code >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCD_MOSI_Hi;
        }
        else
        {
            LCD_MOSI_Lo;
        }

        DelayLoop(SPI_DELAY);

        LCD_CLK_Hi;            // CLOCK = High
        DelayLoop(SPI_DELAY);

    }

    // RS = "0" : index data
    LCD_CLK_Lo;            // CLOCK = Low
    LCD_MOSI_Lo;
    DelayLoop(SPI_DELAY);
    LCD_CLK_Hi;            // CLOCK = High
    DelayLoop(SPI_DELAY);

    // Write
    LCD_CLK_Lo;            // CLOCK = Low
    LCD_MOSI_Lo;
    DelayLoop(SPI_DELAY);
    LCD_CLK_Hi;            // CLOCK = High
    DelayLoop(SPI_DELAY);

    for (j = 15; j >= 0; j--)
    {
        LCD_CLK_Lo;                            //    SCL Low

        if ((address >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCD_MOSI_Hi;
        }
        else
        {
            LCD_MOSI_Lo;
        }

        DelayLoop(SPI_DELAY);

        LCD_CLK_Hi;            // CLOCK = High
        DelayLoop(SPI_DELAY);

    }

    LCD_MOSI_Hi;
    DelayLoop(SPI_DELAY);

    LCD_nSS_Hi;                 // EN = High
    DelayLoop(SPI_DELAY*10);

    LCD_nSS_Lo;         //    EN = Low                CS Low
    DelayLoop(SPI_DELAY);

    for (j = 5; j >= 0; j--)
    {
        LCD_CLK_Lo;                            //    SCL Low

        if ((dev_id_code >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCD_MOSI_Hi;
        }
        else
        {
            LCD_MOSI_Lo;
        }

        DelayLoop(SPI_DELAY);

        LCD_CLK_Hi;            // CLOCK = High
        DelayLoop(SPI_DELAY);

    }

    // RS = "1" instruction data
    LCD_CLK_Lo;            // CLOCK = Low
    LCD_MOSI_Hi;
    DelayLoop(SPI_DELAY);
    LCD_CLK_Hi;            // CLOCK = High
    DelayLoop(SPI_DELAY);

    // Write
    LCD_CLK_Lo;            // CLOCK = Low
    LCD_MOSI_Lo;
    DelayLoop(SPI_DELAY);
    LCD_CLK_Hi;            // CLOCK = High
    DelayLoop(SPI_DELAY);

    for (j = 15; j >= 0; j--)
    {
        LCD_CLK_Lo;                            //    SCL Low

        if ((data >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCD_MOSI_Hi;
        }
        else
        {
            LCD_MOSI_Lo;
        }

        DelayLoop(SPI_DELAY);

        LCD_CLK_Hi;            // CLOCK = High
        DelayLoop(SPI_DELAY);

    }

    LCD_nSS_Hi;                 // EN = High
    DelayLoop(SPI_DELAY);

    //LDI_MSG((_T("[LDI]--LDI_LTV350QV_write()\n\r")));
}

static void LDI_LTM030DK_write(unsigned int address, unsigned int data)
{
    int j;

//    LDI_MSG((_T("[LDI]++LDI_LTV350QV_write(0x%08x, 0x%08x)\n\r"), address, data));

    LCDP_nSS_Hi;                         //    EN = High       CS high
    LCDP_CLK_Lo;                         //    SCL Low

    DelayLoop(10);

    LCDP_nSS_Lo;         //    EN = Low                CS Low

    DelayLoop(10);        
    
    for (j = 7; j >= 0; j--)
    {
        LCDP_MOSI_Lo;
        LCDP_CLK_Lo;                            //    SCL Low
        DelayLoop(10);
        LCDP_CLK_Hi;
        DelayLoop(10);
    }

    LCDP_CLK_Lo;            // CLOCK = Low
    DelayLoop(100);
    
    // addr write
    for (j = 7; j >= 0; j--)
    {
        LCDP_CLK_Lo;                            //    SCL Low

        if ((address >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCDP_MOSI_Hi;
        }
        else
        {
            LCDP_MOSI_Lo;
        }

        DelayLoop(10);

        LCDP_CLK_Hi;            // CLOCK = High
        DelayLoop(10);

    }

 //   LCD_nSS_Lo;         //    EN = Low                CS Low
    LCDP_CLK_Lo;
    DelayLoop(100);

    // write operation and data selection
    for (j = 6; j >= 0; j--)
    {
        LCDP_MOSI_Lo;
        LCDP_CLK_Lo;                            //    SCL Low
        DelayLoop(10);
        LCDP_CLK_Hi;            // CLOCK = High
        DelayLoop(10);

    }
    
    // RS = "1" instruction data
    LCDP_MOSI_Hi;    
    LCDP_CLK_Lo;            // CLOCK = Low
    DelayLoop(10);
    LCDP_CLK_Hi;            // CLOCK = High
    DelayLoop(10);

    // Write
    LCDP_CLK_Lo;            // CLOCK = Low
    DelayLoop(100);

    // data write
    for (j = 7; j >= 0; j--)
    {
        LCDP_CLK_Lo;                            //    SCL Low

        if ((data >> j) & 0x0001)    // DATA HIGH or LOW
        {
            LCDP_MOSI_Hi;
        }
        else
        {
            LCDP_MOSI_Lo;
        }

        DelayLoop(100);

        LCDP_CLK_Hi;            // CLOCK = High
        DelayLoop(10);

    }

    LCDP_CLK_Lo;
    DelayLoop(100);
    
    LCDP_nSS_Hi;                 // EN = High
    DelayLoop(10);
    
}

static void DelayLoop_1ms(int msec)
{
    volatile int j;
    for(j = 0; j < LCD_DELAY_1MS*msec; j++)  ;
}

static void DelayLoop(int delay)
{
    volatile int j;
    for(j = 0; j < delay; j++)  ;
}

