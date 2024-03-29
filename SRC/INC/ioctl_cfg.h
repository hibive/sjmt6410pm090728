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
//  File:  ioctl_cfg.h
//
//  Configuration file for the IOCTL component.
//
#ifndef __IOCTL_CFG_H
#define __IOCTL_CFG_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//  RESTRICTION
//
//  This file is a configuration file for the IOCTL component.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Define:  IOCTL_PLATFORM_TYPE/OEM
//
//  Defines the platform type and OEM string.
//
#ifdef	OMNIBOOK_VER
#define IOCTL_PLATFORM_TYPE	(L"OMNIBOOK S3C6410 WINCE600")
#define IOCTL_PLATFORM_OEM	(L"OMNIBOOK SEOJEON MEDIATECH")
#else	//!OMNIBOOK_VER
#define IOCTL_PLATFORM_TYPE        (L"SMDK6410")
#define IOCTL_PLATFORM_OEM        (L"Samsung SMDK6410")
#endif	OMNIBOOK_VER

//------------------------------------------------------------------------------
//  Define:  IOCTL_PROCESSOR_VENDOR/NAME/CORE
//
//  Defines the processor information
//

#define IOCTL_PROCESSOR_VENDOR    (L"Samsung Electronics")
#define IOCTL_PROCESSOR_NAME        (L"S3C6410")
#define IOCTL_PROCESSOR_CORE        (L"ARM1176JZF-S")

//------------------------------------------------------------------------------
//
//  Define:  IOCTL_PROCESSOR_INSTRUCTION_SET
//
//  Defines the processor instruction set information
//
#define IOCTL_PROCESSOR_INSTRUCTION_SET    (0)
#define IOCTL_PROCESSOR_CLOCK_SPEED        S3C6410_ACLK

//------------------------------------------------------------------------------
//
//  Define:  IOCTL_HAL_GET_CPUID
//
//  Defines the processor identification code request
//
#define IOCTL_HAL_GET_CPUID                 CTL_CODE(FILE_DEVICE_UNKNOWN, 2100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HAL_SET_SYSTEM_LEVEL          CTL_CODE(FILE_DEVICE_UNKNOWN, 2101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HAL_PROFILE_DVS               CTL_CODE(FILE_DEVICE_UNKNOWN, 2102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HAL_CLOCK_INFO                CTL_CODE(FILE_DEVICE_UNKNOWN, 2103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#ifdef	OMNIBOOK_VER
#define IOCTL_HAL_OMNIBOOK_SHUTDOWN				CTL_CODE(FILE_DEVICE_UNKNOWN, 2200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HAL_OMNIBOOK_GET_INFO				CTL_CODE(FILE_DEVICE_UNKNOWN, 2201, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HAL_OMNIBOOK_SET_INFO				CTL_CODE(FILE_DEVICE_UNKNOWN, 2202, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HAL_OMNIBOOK_UPDATE_BOOTLOADER	CTL_CODE(FILE_DEVICE_UNKNOWN, 2203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif	OMNIBOOK_VER

BOOL OALIoCtlHalGetCPUID(
    UINT32 code, void *pInBuffer, UINT32 inSize, void *pOutBuffer,
    UINT32 outSize, UINT32 *lpBytesReturned );

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
