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

Module Name:    s5k4ba_module.cpp

Abstract:       S5K4BA Camera module control module

Functions:


Notes:


--*/

#include <bsp.h>
#include <pm.h>
#include "pmplatform.h"
#include "iic.h"
#include "setting.h"

#include "Module.h"

// Macros

// Definitions
#define MSG_ERROR        1

#define CAMERA_WRITE    (0x5a + 0)
#define CAMERA_READ     (0x5a + 0)

#define DEFAULT_MODULE_ITUXXX        CAM_ITU601
#define DEFAULT_MODULE_YUVORDER        CAM_ORDER_YCRYCB
/*#define DEFAULT_MODULE_HSIZE        640
#define DEFAULT_MODULE_VSIZE        480*/
#define DEFAULT_MODULE_HSIZE        800
#define DEFAULT_MODULE_VSIZE        600
#define DEFAULT_MODULE_HOFFSET        0
#define DEFAULT_MODULE_VOFFSET        0
#define DEFAULT_MODULE_UVOFFSET        CAM_UVOFFSET_0
#define DEFAULT_MODULE_CLOCK        27000000
#define DEFAULT_MODULE_CODEC        CAM_CODEC_422
#define DEFAULT_MODULE_HIGHRST        0
#define DEFAULT_MODULE_INVPCLK        1
#define    DEFAULT_MODULE_INVVSYNC        1
#define DEFAULT_MODULE_INVHREF         0

// Variables
static MODULE_DESCRIPTOR             gModuleDesc;
static HANDLE                        hI2C;   // I2C Bus Driver

// Functions 
int  ModuleWriteBlock();
DWORD HW_WriteRegisters(PUCHAR pBuff, DWORD nRegs);
DWORD HW_ReadRegisters(PUCHAR pBuff, UCHAR StartReg, DWORD nRegs);

/////////////////////////////////////////////////////////////////////////////////
int ModuleInit()
{
    DWORD dwErr = ERROR_SUCCESS, bytes;
    UINT32  IICClock = 10000;
    UINT32    uiIICDelay;
    RETAILMSG(0,(TEXT("+ModuleInit\n")));
    // Initialize I2C Driver
    hI2C = CreateFile( L"IIC0:",
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, 0, 0);
                   
    if ( INVALID_HANDLE_VALUE == hI2C ) {
        dwErr = GetLastError();
        RETAILMSG(MSG_ERROR, (TEXT("Error %d opening device '%s' \r\n"), dwErr, L"I2C0:" ));
        return FALSE;
    }

    gModuleDesc.ITUXXX = DEFAULT_MODULE_ITUXXX;
    gModuleDesc.UVOffset = DEFAULT_MODULE_UVOFFSET;
    gModuleDesc.SourceHSize = DEFAULT_MODULE_HSIZE;
    gModuleDesc.Order422 = DEFAULT_MODULE_YUVORDER;
    gModuleDesc.SourceVSize = DEFAULT_MODULE_VSIZE;
    gModuleDesc.Clock = DEFAULT_MODULE_CLOCK;
    gModuleDesc.Codec = DEFAULT_MODULE_CODEC;
    gModuleDesc.HighRst = DEFAULT_MODULE_HIGHRST;
    gModuleDesc.SourceHOffset = DEFAULT_MODULE_HOFFSET;
    gModuleDesc.SourceVOffset = DEFAULT_MODULE_VOFFSET;
    gModuleDesc.InvPCLK = DEFAULT_MODULE_INVPCLK;
    gModuleDesc.InvVSYNC = DEFAULT_MODULE_INVVSYNC;
    gModuleDesc.InvHREF = DEFAULT_MODULE_INVHREF;
    
    // use iocontrol to write
    if ( !DeviceIoControl(hI2C,
                          IOCTL_IIC_SET_CLOCK, 
                          &IICClock, sizeof(UINT32), 
                          NULL, 0,
                          &bytes, NULL) ) 
    {
        dwErr = GetLastError();
        RETAILMSG(MSG_ERROR,(TEXT("IOCTL_IIC_SET_CLOCK ERROR: %u \r\n"), dwErr));
        return FALSE;
    }       
    
    uiIICDelay = Clk_0;
    
    if ( !DeviceIoControl(hI2C,
                      IOCTL_IIC_SET_DELAY, 
                      &uiIICDelay, sizeof(UINT32), 
                      NULL, 0,
                      &bytes, NULL) )
    {
        dwErr = GetLastError();
        RETAILMSG(MSG_ERROR,(TEXT("IOCTL_IIC_SET_DELAY ERROR: %u \r\n"), dwErr));
        return FALSE;
    }
    
    
    RETAILMSG(0,(TEXT("-ModuleInit\n")));
    return TRUE;
}

void ModuleDeinit()
{    
    CloseHandle(hI2C);
}

int  ModuleWriteBlock()
{
    int i;
    UCHAR BUF=0;
    RETAILMSG(0,(TEXT("+ModuleWriteBlock\n")));
    for(i=0; i<(sizeof(S5K4BA_YCbCr8bit)/2); i++)
    {
        HW_WriteRegisters(&S5K4BA_YCbCr8bit[i][0], 2);
    }
    
    //ModuleSetImageSize(VGA);
    ModuleSetImageSize(SUB_SAMPLING2);
    
    
    RETAILMSG(0,(TEXT("-ModuleWriteBlock\n")));
/*    
    for(i=0; i<(sizeof(S5K3AB_YCbCr8bit)/2) && (res == 0); i++)
    {
        HW_ReadRegisters(&BUF, S5K3AB_YCbCr8bit[i][0], 1);
        RETAILMSG(1,(TEXT("0x%x\n"), BUF));
    }
    */
    return TRUE;    
}

DWORD
HW_WriteRegisters(
    PUCHAR pBuff,   // Optional buffer
    DWORD nRegs     // number of registers
    )
{
    DWORD dwErr=0;
    DWORD bytes;
    IIC_IO_DESC IIC_Data;
    
    RETAILMSG(0,(TEXT("+HW_WriteRegisters\n")));    
    
    IIC_Data.SlaveAddress = CAMERA_WRITE;
    IIC_Data.Count    = nRegs;
    IIC_Data.Data = pBuff;
    
    // use iocontrol to write
    if ( !DeviceIoControl(hI2C,
                          IOCTL_IIC_WRITE, 
                          &IIC_Data, sizeof(IIC_IO_DESC), 
                          NULL, 0,
                          &bytes, NULL) ) 
    {
        dwErr = GetLastError();
        RETAILMSG(MSG_ERROR,(TEXT("IOCTL_IIC_WRITE ERROR: %u \r\n"), dwErr));
    }   

    if ( dwErr ) {
        RETAILMSG(MSG_ERROR, (TEXT("I2CWrite ERROR: %u \r\n"), dwErr));
        //DEBUGMSG(ZONE_ERR, (TEXT("I2CWrite ERROR: %u \r\n"), dwErr));
    }            
    RETAILMSG(0,(TEXT("-HW_WriteRegisters\n")));    

    return dwErr;
}

DWORD
HW_ReadRegisters(
    PUCHAR pBuff,       // Optional buffer
    UCHAR StartReg,     // Start Register
    DWORD nRegs         // Number of Registers
    )
{
    DWORD dwErr=0;
    DWORD bytes;
    IIC_IO_DESC IIC_AddressData, IIC_Data;
    IIC_AddressData.SlaveAddress = CAMERA_WRITE;
    IIC_AddressData.Data = &StartReg;
    IIC_AddressData.Count = 1;
    
    IIC_Data.SlaveAddress = CAMERA_WRITE;
    IIC_Data.Data = pBuff;
    IIC_Data.Count = 1;
    
    // use iocontrol to read    
    if ( !DeviceIoControl(hI2C,
                          IOCTL_IIC_READ, 
                          &IIC_AddressData, sizeof(IIC_IO_DESC), 
                          &IIC_Data, sizeof(IIC_IO_DESC),
                          &bytes, NULL) ) 
    {
        dwErr = GetLastError();
        RETAILMSG(MSG_ERROR,(TEXT("IOCTL_IIC_WRITE ERROR: %u \r\n"), dwErr));
    }   

    
    if ( !dwErr ) {


    } else {        
        RETAILMSG(1,(TEXT("I2CRead ERROR: %u \r\n"), dwErr));
        //DEBUGMSG(ZONE_ERR,(TEXT("I2CRead ERROR: %u \r\n"), dwErr));
    }            
    
    return dwErr;
}


// copy module data to output buffer
void ModuleGetFormat(MODULE_DESCRIPTOR &outModuleDesc)
{
    memcpy(&outModuleDesc, &gModuleDesc, sizeof(MODULE_DESCRIPTOR));
}

int     ModuleSetImageSize(int imageSize)
{
    BYTE page[2];
    BYTE sizeValue[2];
    
    sizeValue[0] = 0x02;
    switch(imageSize)
    {
        case UXGA:
            sizeValue[1] = 0;
            break;    
        case SXGA:
            sizeValue[1] = 1;
            break;
        case VGA:
            sizeValue[1] = 2;
            break;
        case QVGA:
            sizeValue[1] = 3;
            break;
        case QQVGA:
            sizeValue[1] = 4;
            break;
        case CIF:
            sizeValue[1] = 5;
            break;
        case QCIF:
            sizeValue[1] = 6;
            break;
        case SUB_SAMPLING2:
            sizeValue[1] = 9;
            break;
        case SUB_SAMPLING4:
            sizeValue[1] = 0xB;
            break;
    }
    
    page[0] = 0xFC;
    page[1] = 0;
    HW_WriteRegisters(page, 2);
    HW_WriteRegisters(sizeValue, 2);
    return TRUE;
}