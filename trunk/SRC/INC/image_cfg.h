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
//  File:  image_cfg.h
//
//  Defines configuration parameters used to create the NK and Bootloader
//  program images.
//
#ifndef __IMAGE_CFG_H
#define __IMAGE_CFG_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//  RESTRICTION
//
//  This file is a configuration file. It should ONLY contain simple #define
//  directives defining constants. This file is included by other files that
//  only support simple substitutions.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  NAMING CONVENTION
//
//  The IMAGE_ naming convention ...
//
//  IMAGE_<NAME>_<SECTION>_<MEMORY_DEVICE>_[OFFSET|SIZE|START|END]
//
//      <NAME>          - WINCE, BOOT, SHARE
//      <SECTION>       - section name: user defined
//      <MEMORY_DEVICE> - the memory device the block resides on
//      OFFSET          - number of bytes from memory device start address
//      SIZE            - maximum size of the block
//      START           - start address of block    (device address + offset)
//      END             - end address of block      (start address  + size - 1)
//
//------------------------------------------------------------------------------

// DRAM Base Address
#ifdef SMDK6410_X5D
#define DRAM_BASE_PA_START            (0x60000000)
#else
#define DRAM_BASE_PA_START            (0x50000000)
#endif
#define DRAM_BASE_CA_START            (0x80000000)
#define DRAM_BASE_UA_START            (0xA0000000)
#ifdef SMDK6410_X5D
#define DRAM_SIZE                     (0x04000000)
#else
#define DRAM_SIZE                     (0x08000000)
#endif

//------------------------------------------------------------------------------

// Steploader Area
#define IMAGE_STEPLOADER_PA_START       (0x00000000)
#define IMAGE_STEPLOADER_SIZE           (0x00002000)

//------------------------------------------------------------------------------

// Eboot Area
#define IMAGE_EBOOT_OFFSET              (0x00030000)
#define IMAGE_EBOOT_PA_START            (DRAM_BASE_PA_START+IMAGE_EBOOT_OFFSET)
#define IMAGE_EBOOT_CA_START            (DRAM_BASE_CA_START+IMAGE_EBOOT_OFFSET)
#define IMAGE_EBOOT_UA_START            (DRAM_BASE_UA_START+IMAGE_EBOOT_OFFSET)
#define IMAGE_EBOOT_SIZE                (0x00080000)

#define EBOOT_BINFS_BUFFER_OFFSET       (0x06C00000)
#define EBOOT_BINFS_BUFFER_PA_START     (DRAM_BASE_PA_START+EBOOT_BINFS_BUFFER_OFFSET)
#define EBOOT_BINFS_BUFFER_CA_START     (DRAM_BASE_CA_START+EBOOT_BINFS_BUFFER_OFFSET)
#define EBOOT_BINFS_BUFFER_UA_START     (DRAM_BASE_UA_START+EBOOT_BINFS_BUFFER_OFFSET)
#define EBOOT_BINFS_BUFFER_SIZE         (0x00480000)

#ifdef SMDK6410_X5D
#define EBOOT_USB_BUFFER_OFFSET         (0x01D00000)
#else
#define EBOOT_USB_BUFFER_OFFSET         (0x03000000)
#endif
#define EBOOT_USB_BUFFER_PA_START       (DRAM_BASE_PA_START+EBOOT_USB_BUFFER_OFFSET)
#define EBOOT_USB_BUFFER_CA_START       (DRAM_BASE_CA_START+EBOOT_USB_BUFFER_OFFSET)
#define EBOOT_USB_BUFFER_UA_START       (DRAM_BASE_UA_START+EBOOT_USB_BUFFER_OFFSET)

// Eboot Display Frame Buffer
// 1MB
#ifdef SMDK6410_X5D
#define EBOOT_FRAMEBUFFER_OFFSET        (0x03F00000)
#define EBOOT_FRAMEBUFFER_PA_START      (DRAM_BASE_PA_START+EBOOT_FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_UA_START      (DRAM_BASE_UA_START+EBOOT_FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_SIZE          (0x00100000)
#else
#define EBOOT_FRAMEBUFFER_OFFSET        (0x07F00000)
#define EBOOT_FRAMEBUFFER_PA_START      (DRAM_BASE_PA_START+EBOOT_FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_UA_START      (DRAM_BASE_UA_START+EBOOT_FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_SIZE          (0x00100000)
#endif

//------------------------------------------------------------------------------

// NK Area
#ifdef SMDK6410_X5D
#define IMAGE_NK_OFFSET                 (0x00100000)
#define IMAGE_NK_PA_START               (DRAM_BASE_PA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_CA_START               (DRAM_BASE_CA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_UA_START               (DRAM_BASE_UA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_SIZE                   (0x02100000)
#else
#define IMAGE_NK_OFFSET                 (0x00100000)
#define IMAGE_NK_PA_START               (DRAM_BASE_PA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_CA_START               (DRAM_BASE_CA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_UA_START               (DRAM_BASE_UA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_SIZE                   (0x04F00000)  // Set Max Size, This will be tailored automatically
#endif

//------------------------------------------------------------------------------

// BSP ARGs Area
#define IMAGE_SHARE_ARGS_OFFSET         (0x00020800)
#define IMAGE_SHARE_ARGS_PA_START       (DRAM_BASE_PA_START+IMAGE_SHARE_ARGS_OFFSET)
#define IMAGE_SHARE_ARGS_CA_START       (DRAM_BASE_CA_START+IMAGE_SHARE_ARGS_OFFSET)
#define IMAGE_SHARE_ARGS_UA_START       (DRAM_BASE_UA_START+IMAGE_SHARE_ARGS_OFFSET)
#define IMAGE_SHARE_ARGS_SIZE           (0x00000800)

//------------------------------------------------------------------------------

// Sleep Data Area
#define IMAGE_SLEEP_DATA_OFFSET         (0x00028000)
#define IMAGE_SLEEP_DATA_PA_START       (DRAM_BASE_PA_START+IMAGE_SLEEP_DATA_OFFSET)
#define IMAGE_SLEEP_DATA_CA_START       (DRAM_BASE_CA_START+IMAGE_SLEEP_DATA_OFFSET)
#define IMAGE_SLEEP_DATA_UA_START       (DRAM_BASE_UA_START+IMAGE_SLEEP_DATA_OFFSET)
#define IMAGE_SLEEP_DATA_SIZE           (0x00002000)

//------------------------------------------------------------------------------

// Display Frame Buffer
#ifdef SMDK6410_X5D
// 8MB
#define IMAGE_FRAMEBUFFER_OFFSET        (0x03800000)
#define IMAGE_FRAMEBUFFER_PA_START      (DRAM_BASE_PA_START+IMAGE_FRAMEBUFFER_OFFSET)
#define IMAGE_FRAMEBUFFER_UA_START      (DRAM_BASE_UA_START+IMAGE_FRAMEBUFFER_OFFSET)
#define IMAGE_FRAMEBUFFER_SIZE          (0x00400000)
#else
// 12MB
#define IMAGE_FRAMEBUFFER_OFFSET        (0x06800000)
#define IMAGE_FRAMEBUFFER_PA_START      (DRAM_BASE_PA_START+IMAGE_FRAMEBUFFER_OFFSET)
#define IMAGE_FRAMEBUFFER_UA_START      (DRAM_BASE_UA_START+IMAGE_FRAMEBUFFER_OFFSET)
#define IMAGE_FRAMEBUFFER_SIZE          (0x00C00000)
#endif

//------------------------------------------------------------------------------

// MFC Video Process Buffer
#ifdef SMDK6410_X5D
// 8MB
#define IMAGE_MFC_BUFFER_OFFSET         (0x03C00000)
#define IMAGE_MFC_BUFFER_PA_START       (DRAM_BASE_PA_START+IMAGE_MFC_BUFFER_OFFSET)
#define IMAGE_MFC_BUFFER_UA_START       (DRAM_BASE_UA_START+IMAGE_MFC_BUFFER_OFFSET)
#define IMAGE_MFC_BUFFER_SIZE           (0x00400000)
#else
// 12MB
#define IMAGE_MFC_BUFFER_OFFSET         (0x07400000)
#define IMAGE_MFC_BUFFER_PA_START       (DRAM_BASE_PA_START+IMAGE_MFC_BUFFER_OFFSET)
#define IMAGE_MFC_BUFFER_UA_START       (DRAM_BASE_UA_START+IMAGE_MFC_BUFFER_OFFSET)
#define IMAGE_MFC_BUFFER_SIZE           (0x00C00000)
#endif

//------------------------------------------------------------------------------

// CMM memory
#ifdef SMDK6410_X5D
#define IMAGE_CMM_BUFFER_OFFSET         (0x03500000)
#define IMAGE_CMM_BUFFER_PA_START       (DRAM_BASE_PA_START+IMAGE_CMM_BUFFER_OFFSET)
#define IMAGE_CMM_BUFFER_UA_START       (DRAM_BASE_UA_START+IMAGE_CMM_BUFFER_OFFSET)
#define IMAGE_CMM_BUFFER_SIZE           (0x00300000)
#else
#define IMAGE_CMM_BUFFER_OFFSET         (0x06500000)
#define IMAGE_CMM_BUFFER_PA_START       (DRAM_BASE_PA_START+IMAGE_CMM_BUFFER_OFFSET)
#define IMAGE_CMM_BUFFER_UA_START       (DRAM_BASE_UA_START+IMAGE_CMM_BUFFER_OFFSET)
#define IMAGE_CMM_BUFFER_SIZE           (0x00300000)
#endif

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
