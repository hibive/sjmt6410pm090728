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

Module Name:    s3c6410_ldi.h

Abstract:       Function prototypes and enumrate value of LDI control library

Functions:


Notes:


--*/

#ifndef __S3C6410_LDI_H__
#define __S3C6410_LDI_H__

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
    LDI_LTS222QV_RGB,                // 2.2" QVGA 240x320 in SMDK6410
    LDI_LTV350QV_RGB,                // 3.5" QVGA 320x240 in SMDK6410
    LDI_LTE480WV_RGB,                // 4.8" WVGA 800*480 in SMDK6410
    LDI_LTP700WV_RGB,                // 7" WVGA 800x480 External Module
    LDI_SMRP_TD043MTEA1_RGB,        // 4.3" WVGA 800x480 in SMRP6410
    LDI_SMRP_LTE480WV_RGB,        // 4.8" WVGA 800*480 4.8" in SMRP6410
    LDI_LTM030DK_RGB,
    LDI_MODULE_TYPE_MAX
} LDI_LCD_MODULE_TYPE;

typedef enum
{
    LDI_SUCCESS,
    LDI_ERROR_NULL_PARAMETER,
    LDI_ERROR_ILLEGAL_PARAMETER,
    LDI_ERROR_NOT_INITIALIZED,
    LDI_ERROR_NOT_IMPLEMENTED,
    LDI_ERROR_XXX
} LDI_ERROR;

LDI_ERROR LDI_initialize_register_address(void *pSPIReg, void *pDispConReg, void *pGPIOReg);
LDI_ERROR LDI_set_LCD_module_type(LDI_LCD_MODULE_TYPE ModuleType);
LDI_ERROR LDI_initialize_LCD_module(void);
LDI_ERROR LDI_deinitialize_LCD_module(void);
LDI_ERROR LDI_fill_output_device_information(void *pDevInfo);

void LDI_LTM030DK_port_initialize(void);
static void LDI_LTM030DK_spi_port_enable(void);
LDI_ERROR LDI_LTM030DK_RGB_initialize(void);


static void LDI_LTS222QV_port_initialize(void);
static void LDI_LTS222QV_spi_port_enable(void);
static void LDI_LTS222QV_spi_port_disable(void);
static void LDI_LTS222QV_reset(void);
static LDI_ERROR LDI_LTS222QV_RGB_initialize(void);

static void LDI_LTV350QV_port_initialize(void);
static void LDI_LTV350QV_reset(void);
static LDI_ERROR LDI_LTV350QV_RGB_initialize(void);

static void LDI_LTE480WV_RGB_port_initialize(void);
static LDI_ERROR LDI_LTE480WV_RGB_power_on(void);
static LDI_ERROR LDI_LTE480WV_RGB_power_off(void);
static LDI_ERROR LDI_LTE480WV_RGB_initialize(void);

static void LDI_LTP700WV_port_initialize(void);
static void LDI_LTP700WV_reset(void);
static LDI_ERROR LDI_LTP700WV_RGB_initialize(void);

static void LDI_TD043MTEA1_port_initialize(void);
static void LDI_TD043MTEA1_power_on(void);
static void LDI_TD043MTEA1_power_off(void);
static LDI_ERROR LDI_TD043MTEA1_RGB_initialize(void);

static void LDI_SMRP_LTE480WV_RGB_port_initialize(void);
static LDI_ERROR LDI_SMRP_LTE480WV_RGB_power_on(void);
static LDI_ERROR LDI_SMRP_LTE480WV_RGB_power_off(void);
static LDI_ERROR LDI_SMRP_LTE480WV_RGB_initialize(void);

static void LDI_LTS222QV_write(unsigned int address, unsigned int data);
static void LDI_LTV350QV_write(unsigned int address, unsigned int data);
static void LDI_LTM030DK_write(unsigned int address, unsigned int data);
static void DelayLoop_1ms(int msec);
static void DelayLoop(int delay);

// Macros to handle GPIOs

#define TFT_LCD_nRESET_BIT  (5)

#define SET_TFT_LCD_nRESET(s)  SET_GPIO(s, GPNDAT, TFT_LCD_nRESET_BIT, 1)
#define CLEAR_TFT_LCD_nRESET(s)  CLEAR_GPIO(s, GPNDAT, TFT_LCD_nRESET_BIT)

#if __cplusplus
}
#endif

#endif    // __S3C6410_LDI_H__
