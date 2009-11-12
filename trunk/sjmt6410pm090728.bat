@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM
@REM Use of this source code is subject to the terms of the Microsoft end-user
@REM license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
@REM If you did not accept the terms of the EULA, you are not authorized to use
@REM this source code. For a copy of the EULA, please see the LICENSE.RTF on your
@REM install media.
@REM


@REM EBOOK2_VER 2 or 3
set EBOOK2_VER=3
@REM EPD(1) or LCD()
set EBOOK2_DISP_EPD=1


@REM To support for SMDK6410X5D
@REM X5D MCP has 2Gb NAND + 512Mb M-DDR + 512Mb OneDRAM
@REM set SMDK6410_X5D=

@REM To support iROM NANDFlash boot
set BSP_IROMBOOT=1

@REM To support iROM SDMMC boot
set BSP_IROM_SDMMC_CH0_BOOT=
set BSP_IROM_SDMMC_CH1_BOOT=

set WINCEREL=1

set BSP_NODISPLAY=
set BSP_NOVIDEO=
set BSP_NOBACKLIGHT=
set BSP_NOTOUCH=

if "%EBOOK2_DISP_EPD%"=="1" (
	set BSP_NODISPLAY=1
	set BSP_NOVIDEO=1
	set BSP_NOBACKLIGHT=1
	set BSP_NOTOUCH=1

	set BSP_DISPLAY_BROADSHEET=1
	set BSP_TOUCH_UART=1
	set BSP_ACCELERATION=1
)

set BSP_NOKEYBD=
set BSP_NOPWRBTN=

set BSP_NONANDFS=

set BSP_NOHSMMC_CH0=
if /i "%BSP_IROM_SDMMC_CH0_BOOT%"=="1" set BSP_NOHSMMC_CH0=1
set BSP_NOHSMMC_CH1=
if /i "%BSP_IROM_SDMMC_CH1_BOOT%"=="1" set BSP_NOHSMMC_CH1=1
set BSP_HSMMC_CH1_8BIT=
set BSP_NOHSMMC_CH2=1
if "%EBOOK2_VER%"=="3" (
	set BSP_NOHSMMC_CH1=1
	set BSP_NOHSMMC_CH2=
)

set BSP_NOCFATAPI=1

set BSP_NOAUDIO=
set BSP_AUDIO_AC97=

set BSP_NOCAMERA=1

set BSP_NOBATTERY=
set BSP_NOETC=
set BSP_NOWIFI=

set BSP_NOD3DM=
set BSP_NOMFC=
set BSP_NOJPEG=
set BSP_NOOES=
if /i "%SMDK6410_X5D%"=="1" set BSP_NOOES=1
set BSP_NOCMM=
set BSP_NOUAO=

if "%EBOOK2_DISP_EPD%"=="1" (
	set BSP_NOD3DM=1
	set BSP_NOMFC=1
	set BSP_NOJPEG=1
	set BSP_NOOES=1
	set BSP_NOCMM=1
	set BSP_NOUAO=1
)

set BSP_NOSERIAL=1
if /i "%BSP_TOUCH_UART%"=="1" set BSP_NOSERIAL=
set BSP_NOUART0=1
set BSP_NOUART1=1
if /i "%BSP_TOUCH_UART%"=="1" set BSP_NOUART1=
set BSP_NOUART2=1
set BSP_NOUART3=1
set BSP_NOIRDA2=1
set BSP_NOIRDA3=1

set BSP_NOI2C=
set BSP_NOSPI=1

set BSP_NOUSBHCD=

@REM If you want to exclude USB Function driver in BSP. Set this variable
set BSP_NOUSBFN=
@REM This select default function driver
set BSP_USBFNCLASS=SERIAL
@REM set BSP_USBFNCLASS=MASS_STORAGE

@REM DVFS is not yet implemented.
set BSP_USEDVS=

set BSP_DEBUGPORT=SERIAL_UART0
@REM set BSP_DEBUGPORT=SERIAL_UART1
@REM set BSP_DEBUGPORT=SERIAL_UART2
@REM set BSP_DEBUGPORT=SERIAL_UART3


@REM set BSP_KITL=NONE
@REM set BSP_KITL=SERIAL_UART0
@REM set BSP_KITL=SERIAL_UART1
@REM set BSP_KITL=SERIAL_UART2
@REM set BSP_KITL=SERIAL_UART3
@REM set BSP_KITL=USBSERIAL

@REM For Hive Based Registry
set IMGHIVEREG=1
@REM if /i "%BSP_IROM_SDMMC_CH1_BOOT%"=="1" set IMGHIVEREG=

if /i "%IMGHIVEREG%"=="1" set PRJ_ENABLE_FSREGHIVE=1
if /i "%IMGHIVEREG%"=="1" set PRJ_ENABLE_REGFLUSH_THREAD=1

@REM Multipl-XIP using demand paging, BINFS must be turned on
set IMGMULTIXIP=

@REM Does not support
set IMGMULTIBIN=

@REM for using GDI Performance Utility, DispPerf
@REM there are some compatibility issue between CE6.0 R2 and previous version
@REM if you want to use DISPPERF in CE6.0 R2,
@REM build public components(GPE) also.
set DO_DISPPERF=


@REM Multimedia Samples
@REM DSHOWFILTERS must be on if use MFC Codec
set SAMPLES_DSHOWFILTERS=1
set SAMPLES_MFC=1
set SAMPLES_MFC_API=1
set SAMPLES_JPEG=1
set SAMPLES_HYBRIDDIVX=1
set SAMPLES_CMM=1
set SAMPLES_VFP_TEST=1

if "%EBOOK2_DISP_EPD%"=="1" (
	set SAMPLES_DSHOWFILTERS=
	set SAMPLES_MFC=
	set SAMPLES_MFC_API=
	set SAMPLES_JPEG=
	set SAMPLES_HYBRIDDIVX=
	set SAMPLES_CMM=
	set SAMPLES_VFP_TEST=
)


@REM - To support PocketMory
call %_TARGETPLATROOT%\src\Whimory\wmrenv.bat


set APPS_EBOOK2_STARTUP=1
set APPS_EBOOK2_COMMAND=
if /i "%EBOOK2_DISP_EPD%"=="1" set APPS_EBOOK2_COMMAND=1

if not "%EBOOK2_DISP_EPD%"=="1" (
	set SYSGEN_D3DM=1
	set SYSGEN_DDRAW=1
	set SYSGEN_DSHOW_WMV=1
)

if "%EBOOK2_VER%"=="2" (
	set SYSGEN_LARGEKB=1
)

if "%EBOOK2_VER%"=="3" (
	set SYSGEN_K_IME97=1
)

