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


Module Name:    HWCTXT.CPP

Abstract:        Platform dependent code for the mixing audio driver.

Notes:            The following file contains all the hardware specific code
                for the mixing audio driver.  This code's primary responsibilities
                are:

                    * Initialize audio hardware (including codec chip)
                    * Schedule DMA operations (move data from/to buffers)
                    * Handle audio interrupts

                All other tasks (mixing, volume control, etc.) are handled by the "upper"
                layers of this driver.

                ****** IMPORTANT ******
                For the S3C6410 CPU, DMA channel 2 can be used for both input and output.  In this,
                configuration, however, only one type operation (input or output) can execute.  In
                order to implement simultaneous playback and recording, two things must be done:

                    1) Input DMA should be moved to DMA Channel 1; Output DMA still uses DMA Channel 2.
                    2) Step #3 in InterruptThread() needs to be implemented so that the DMA interrupt
                       source (input DMA or output DMA?) can be determined.  The interrupt source needs
                       to be determined so that the appropriate buffers can be copied (Steps #4,#5...etc.).

                Lastly, the m_OutputDMAStatus and m_InputDMAStatus variables shouldn't need to be modified.
                The logic surrounding these drivers is simply used to determine which buffer (A or B) needs
                processing.

-*/

#include "wavemain.h"
#include <ceddk.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "WM8580.h"

#include <iic.h>
#include "s3c6410_iis_interface.h"
#include "s3c6410_dma_controller.h"
#include "hwctxt.h"

typedef enum
{
    DMA_CH_OUT    = 0x1,
    DMA_CH_IN        = 0x2
} DMA_CH_SELECT;


#define WAV_MSG(x)
#define WAV_INF(x)    DEBUGMSG(ZONE_FUNCTION, x)
#define WAV_ERR(x)    DEBUGMSG(ZONE_ERROR, x)


#define INTERRUPT_THREAD_PRIORITY_DEFAULT    (150)

#ifdef	EBOOK2_VER
#define CHIP_ID                         (0x34)        // WM8976 Chip ID
#else	EBOOK2_VER
#define CHIP_ID                         (0x36)        // WM8580 Chip ID
#endif	EBOOK2_VER
#define WM8580_READ                (CHIP_ID + 1)
#define WM8580_WRITE            (CHIP_ID + 0)

HardwareContext *g_pHWContext        = NULL;

static volatile S3C6410_IIS_REG        *g_pIISReg = NULL;
static volatile S3C6410_GPIO_REG    *g_pGPIOReg = NULL;
static volatile S3C6410_DMAC_REG    *g_pDMAC0Reg = NULL;
static volatile S3C6410_DMAC_REG    *g_pDMAC1Reg = NULL;
static volatile S3C6410_SYSCON_REG    *g_pSysConReg = NULL;

static DMA_CH_CONTEXT    g_OutputDMA;
static DMA_CH_CONTEXT    g_InputDMA;
static PHYSICAL_ADDRESS    g_PhyDMABufferAddr;


BOOL
HardwareContext::CreateHWContext(DWORD Index)
{
    if (g_pHWContext)
    {
        return(TRUE);
    }

    g_pHWContext = new HardwareContext;
    if (g_pHWContext == NULL)
    {
        return(FALSE);
    }

    return(g_pHWContext->Initialize(Index));
}


HardwareContext::HardwareContext()
: m_InputDeviceContext(), m_OutputDeviceContext()
{
    InitializeCriticalSection(&m_csLock);
    m_bInitialized = FALSE;
}


HardwareContext::~HardwareContext()
{
    DeleteCriticalSection(&m_csLock);
}


BOOL
HardwareContext::Initialize(DWORD Index)
{
    BOOL bRet;
    DWORD dwErr, bytes;
    UINT32 uiIICDelay;

    if (m_bInitialized)
    {
        return(FALSE);
    }

    m_DriverIndex = Index;
    m_InPowerHandler = FALSE;

    m_bOutputDMARunning = FALSE;
    m_bInputDMARunning = FALSE;
    m_bSavedInputDMARunning = FALSE;
    m_bSavedOutputDMARunning = FALSE;
    m_InputDMAStatus    = DMA_CLEAR;
    m_OutputDMAStatus = DMA_CLEAR;
    m_nOutByte[OUTPUT_DMA_BUFFER0] = 0;
    m_nOutByte[OUTPUT_DMA_BUFFER1] = 0;
    m_nInByte[INPUT_DMA_BUFFER0] = 0;
    m_nInByte[INPUT_DMA_BUFFER1] = 0;

    m_dwSysintrOutput = NULL;
    m_dwSysintrInput = NULL;
    m_hOutputDMAInterrupt = NULL;
    m_hInputDMAInterrupt = NULL;
    m_hOutputDMAInterruptThread = NULL;
    m_hInputDMAInterruptThread = NULL;

    m_dwOutputGain = 0xFFFF;
    m_dwInputGain = 0xFFFF;
    m_bOutputMute = FALSE;
    m_bInputMute = FALSE;

    m_NumForcedSpeaker = 0;

    // Map Virtual Address for SFR
    bRet = MapRegisters();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : MapRegisters() Failed\n\r")));
        goto CleanUp;
    }

    // Allocation and Map DMA Buffer
    bRet = MapDMABuffers();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : MapDMABuffers() Failed\n\r")));
        goto CleanUp;
    }

    //Open handle to the I2C bus
    m_hI2C = CreateFile( L"IIC0:",
                        GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, 0);
    if ( m_hI2C == INVALID_HANDLE_VALUE )
    {
        dwErr = GetLastError();
        WAV_ERR((_T("[WAV:ERR] Initialize() : I2C0: Device Open Failed = 0x%08x\n\r"), dwErr));
        goto CleanUp;
    }
    
    uiIICDelay = Clk_0;
    
    bRet = DeviceIoControl(m_hI2C,
                      IOCTL_IIC_SET_DELAY, 
                      &uiIICDelay, sizeof(UINT32), 
                      NULL, 0,
                      &bytes, NULL);
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : I2C0: Device Set Delay Failed.\n\r")));
        goto CleanUp;
    }

    // Enable Clock for IIS CH0
    // PCLK Case
    //g_pSysConReg->CLKSRC = (g_pSysConReg->CLKSRC & ~(0x7<<7)) | (0x /// ????? No config data for CLKSRC on F/W code.. WHY???
#ifdef	EBOOK2_VER
	g_pSysConReg->PCLK_GATE |= (1<<15);
	g_pSysConReg->SCLK_GATE |= (1<<8);
#else	EBOOK2_VER
    g_pSysConReg->PCLK_GATE |= (1<<26);
    g_pSysConReg->SCLK_GATE |= (1<<11);
#endif	EBOOK2_VER

    // Initialize SFR address for PDD Library
    IIS_initialize_register_address((void *)g_pIISReg, (void *)g_pGPIOReg);
    DMA_initialize_register_address((void *)g_pDMAC0Reg, (void *)g_pDMAC1Reg, (void *)g_pSysConReg);

    // Initialize IIS interface
    IIS_initialize_interface();

    // Initialize Audio Codec
    bRet = InitIISCodec();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitIISCodec(() Failed\n\r")));
        goto CleanUp;
    }

    // Request DMA Channel and Initialize
    // DMA context have Virtual IRQ Number of Allocated DMA Channel
    // You Should initialize Interrupt after DMA initialization
    bRet = InitOutputDMA();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitOutputDMA() Failed\n\r")));
        goto CleanUp;
    }

    bRet = InitInputDMA();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitInputDMA() Failed\n\r")));
        goto CleanUp;
    }

    // Initialize Interrupt
    bRet = InitInterruptThread();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitInterruptThread() Failed\n\r")));
        goto CleanUp;
    }

    // Set HwCtxt Initialize Flag
    m_bInitialized = TRUE;

    //-----------------------------------------------
    // Power Manager expects us to init in D0.
    // We are normally in D4 unless we are opened for play.
    // Inform the PM.
    //-----------------------------------------------
    m_Dx = D0;
    DevicePowerNotify(_T("WAV1:"),(_CEDEVICE_POWER_STATE)D4, POWER_NAME);

CleanUp:

    return bRet;
}


BOOL
HardwareContext::Deinitialize()
{
    if (m_bInitialized)
    {
        DeinitInterruptThread();

        StopOutputDMA();
        StopInputDMA();
        DMA_release_channel(&g_OutputDMA);
        DMA_release_channel(&g_InputDMA);

        CodecMuteControl(DMA_CH_OUT|DMA_CH_IN, TRUE);

        UnMapDMABuffers();
        UnMapRegisters();
    }

    return TRUE;
}


void
HardwareContext::PowerUp()
{
    WAV_MSG((_T("[WAV] ++PowerUp()\n\r")));

    // Enable Clock for IIS CH0
#ifdef	EBOOK2_VER
	g_pSysConReg->PCLK_GATE |= (1<<15);
	g_pSysConReg->SCLK_GATE |= (1<<8);
#else	EBOOK2_VER
    g_pSysConReg->PCLK_GATE |= (1<<26);
    g_pSysConReg->SCLK_GATE |= (1<<11);
#endif	EBOOK2_VER

    IIS_initialize_interface();

    InitIISCodec();

    CodecMuteControl(DMA_CH_OUT|DMA_CH_IN, TRUE);
#ifdef	EBOOK2_VER
	CodecPowerControl();
#endif	EBOOK2_VER

    WAV_MSG((_T("[WAV] --PowerUp()\n\r")));
}


void HardwareContext::PowerDown()
{
    WAV_MSG((_T("[WAV] ++PowerDown()\n\r")));

    CodecMuteControl(DMA_CH_OUT|DMA_CH_IN, TRUE);
    CodecPowerControl();

    // Disable Clock for IIS CH0
#ifdef	EBOOK2_VER
	g_pSysConReg->PCLK_GATE &= ~(1<<15);
	g_pSysConReg->SCLK_GATE &= ~(1<<8);
#else	EBOOK2_VER
    g_pSysConReg->PCLK_GATE &= ~(1<<26);
    g_pSysConReg->SCLK_GATE &= ~(1<<11);
#endif	EBOOK2_VER

    WAV_MSG((_T("[WAV] --PowerDown()\n\r")));
}


DWORD
HardwareContext::Open(void)
{
    DWORD mmErr = MMSYSERR_NOERROR;
    DWORD dwErr;

    // Don't allow play when not on, if there is a power constraint upon us.
    if ( D0 != m_Dx )
    {
        // Tell the Power Manager we need to power up.
        // If there is a power constraint then fail.
        dwErr = DevicePowerNotify(_T("WAV1:"), D0, POWER_NAME);
        if ( ERROR_SUCCESS !=  dwErr )
        {
            WAV_ERR((_T("[WAV:ERR] Open() : DevicePowerNotify Error : %u\r\n"), dwErr));
            mmErr = MMSYSERR_ERROR;
        }
    }

    return mmErr;
}


DWORD
HardwareContext::Close(void)
{
    DWORD mmErr = MMSYSERR_NOERROR;
    DWORD dwErr;

    // we are done so inform Power Manager to power us down, 030711
    dwErr = DevicePowerNotify(_T("WAV1:"), (_CEDEVICE_POWER_STATE)D4, POWER_NAME);
    if ( ERROR_SUCCESS !=  dwErr )
    {
        WAV_ERR((_T("[WAV:ERR] Close() : DevicePowerNotify Error : %u\r\n"), dwErr));
        mmErr = MMSYSERR_ERROR;
    }

    return mmErr;
}


BOOL
HardwareContext::IOControl(
            DWORD  dwOpenData,
            DWORD  dwCode,
            PBYTE  pBufIn,
            DWORD  dwLenIn,
            PBYTE  pBufOut,
            DWORD  dwLenOut,
            PDWORD pdwActualOut)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  bRc = TRUE;

    UNREFERENCED_PARAMETER(dwOpenData);

    switch (dwCode)
    {
    //-----------------
    // Power Management
    //-----------------
    case IOCTL_POWER_CAPABILITIES:
        {
            PPOWER_CAPABILITIES ppc;

            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(POWER_CAPABILITIES)) )
            {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                WAV_ERR((_T("[WAV:ERR] IOCTL_POWER_CAPABILITIES : Invalid Parameter\n\r")));
                break;
            }

            ppc = (PPOWER_CAPABILITIES)pBufOut;

            memset(ppc, 0, sizeof(POWER_CAPABILITIES));

            ppc->DeviceDx = 0x11;    // support D0, D4
            ppc->WakeFromDx = 0x0;    // no wake
            ppc->InrushDx = 0x0;        // no inrush

            // REVIEW: Do we enable all these for normal playback?
            // D0: SPI + I2S + CODEC (Playback) + Headphone=
            //     0.5 mA + 0.5 mA + (23 mW, into BUGBUG ohms ) + (30 mW, into 32 ohms)
            //     500 uA + 500 uA + 23000 uA + 32000 uA
            ppc->Power[D0] = 56000;

            // Report our nominal power consumption in uAmps rather than mWatts.
            ppc->Flags = POWER_CAP_PREFIX_MICRO | POWER_CAP_UNIT_AMPS;

            *pdwActualOut = sizeof(POWER_CAPABILITIES);
        }
        break;

    case IOCTL_POWER_SET:
        {
            CEDEVICE_POWER_STATE NewDx;

            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(CEDEVICE_POWER_STATE)) )
            {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                WAV_ERR((_T("[WAV:ERR] CEDEVICE_POWER_STATE : Invalid Parameter\n\r")));
                break;
            }

            NewDx = *(PCEDEVICE_POWER_STATE)pBufOut;

            if ( VALID_DX(NewDx) )
            {
                // grab the CS since the normal Xxx_PowerXxx can not.
                switch (NewDx)
                {
                case D0:
                    if (m_Dx != D0)
                    {
                        m_Dx = D0;

                        PowerUp();

                        Lock();

                        if (m_bSavedOutputDMARunning)
                        {
                            m_bSavedOutputDMARunning = FALSE;
                            SetInterruptEvent(m_dwSysintrOutput);
                            //StartOutputDMA();
                        }

                        Unlock();
                    }
                    break;
                default:
                    if (m_Dx != (_CEDEVICE_POWER_STATE)D4)
                    {
                        // Save last DMA state before Power Down
                        m_bSavedInputDMARunning = m_bInputDMARunning;
                        m_bSavedOutputDMARunning = m_bOutputDMARunning;

                        m_Dx = (_CEDEVICE_POWER_STATE)D4;

                        Lock();

                        StopOutputDMA();
                        StopInputDMA();

                        Unlock();

                        PowerDown();
                    }
                    break;
                }

                // return our state
                *(PCEDEVICE_POWER_STATE)pBufOut = m_Dx;

                *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);

                WAV_INF((_T("[WAV:INF] IOCTL_POWER_SET -> [D%d]\n\r"), m_Dx));
            }
            else
            {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                WAV_ERR((_T("[WAV:ERR] CEDEVICE_POWER_STATE : Invalid Parameter Dx\n\r")));
            }
        }
        break;

    case IOCTL_POWER_GET:
        if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(CEDEVICE_POWER_STATE)) )
        {
            bRc = FALSE;
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        *(PCEDEVICE_POWER_STATE)pBufOut = m_Dx;

        WAV_INF((_T("WAVEDEV: IOCTL_POWER_GET: D%u \r\n"), m_Dx));

        *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
        break;

    default:
        bRc = FALSE;
        dwErr = ERROR_INVALID_FUNCTION;
        WAV_INF((_T(" Unsupported ioctl 0x%X\r\n"), dwCode));
        break;
    }

    if (!bRc)
    {
        SetLastError(dwErr);
    }

    return(bRc);
}


BOOL
HardwareContext::StartOutputDMA()
{
    ULONG OutputTransferred;

    WAV_MSG((_T("[WAV] StartOutputDMA()\r\n")));

    if((m_bOutputDMARunning == FALSE) && (m_Dx == D0))
    {
        m_bOutputDMARunning = TRUE;
        m_nOutByte[OUTPUT_DMA_BUFFER0] = 0;
        m_nOutByte[OUTPUT_DMA_BUFFER1] = 0;

        m_nOutputBufferInUse = OUTPUT_DMA_BUFFER0;    // Start DMA with Buffer 0
        m_OutputDMAStatus = (DMA_DONEA | DMA_DONEB) & ~DMA_BIU;
        OutputTransferred = TransferOutputBuffer(m_OutputDMAStatus);

        if(OutputTransferred)
        {
            CodecPowerControl();                    // Turn Output Channel
            CodecMuteControl(DMA_CH_OUT, FALSE);    // Unmute Output Channel

            // IIS PCM output enable
            IIS_set_tx_mode_control(IIS_TRANSFER_NOPAUSE);

            // Output DMA Start
            DMA_set_channel_source(&g_OutputDMA, m_OutputDMABufferPhyPage[OUTPUT_DMA_BUFFER0], WORD_UNIT, BURST_1, INCREASE);
#ifdef	EBOOK2_VER
			DMA_set_channel_destination(&g_OutputDMA, IIS_get_output_physical_buffer_address(IIS_CH_0), WORD_UNIT, BURST_1, FIXED);
#else	EBOOK2_VER
            DMA_set_channel_destination(&g_OutputDMA, IIS_get_output_physical_buffer_address(IIS_CH_2), WORD_UNIT, BURST_1, FIXED);
#endif	EBOOK2_VER
            DMA_set_channel_transfer_size(&g_OutputDMA, AUDIO_DMA_PAGE_SIZE);
            DMA_set_initial_LLI(&g_OutputDMA, 1);
            DMA_channel_start(&g_OutputDMA);

            IIS_set_active_on();
        }
        else
        {
            WAV_ERR((_T("[WAV:ERR] StartOutputDMA() : There is no data to transfer\r\n")));
            m_bOutputDMARunning = FALSE;
        }
    }
    else
    {
        WAV_ERR((_T("[WAV:ERR] StartOutputDMA() : Output DMA is already running or m_Dx[%d] is not D0\r\n"), m_Dx));
        return FALSE;
    }

    return TRUE;
}


void
HardwareContext::StopOutputDMA()
{
    WAV_MSG((_T("[WAV] StopOutputDMA()\r\n")));

    if (m_bOutputDMARunning)
    {
        m_OutputDMAStatus = DMA_CLEAR;

        // Stop output DMA
        DMA_channel_stop(&g_OutputDMA);

        // IIS PCM output disable
        IIS_set_tx_mode_control(IIS_TRANSFER_PAUSE);

        if (m_bInputDMARunning == FALSE) IIS_set_active_off();
    }

    m_bOutputDMARunning = FALSE;

    CodecMuteControl(DMA_CH_OUT, TRUE);
    CodecPowerControl();
}


BOOL
HardwareContext::StartInputDMA()
{
    WAV_MSG((_T("[WAV] StartInputDMA()\r\n")));

    if(m_bInputDMARunning == FALSE)
    {
        m_bInputDMARunning = TRUE;

        m_nInByte[INPUT_DMA_BUFFER0] = 0;
        m_nInByte[INPUT_DMA_BUFFER1] = 0;

        m_nInputBufferInUse = INPUT_DMA_BUFFER0;    // Start DMA with Buffer 0
        m_InputDMAStatus = (DMA_DONEA | DMA_DONEB) & ~DMA_BIU;

        CodecPowerControl();                    // Turn On Channel
        CodecMuteControl(DMA_CH_IN, FALSE);    // Unmute Input Channel

        // IIS PCM input enable
        IIS_set_rx_mode_control(IIS_TRANSFER_NOPAUSE);

#ifdef	EBOOK2_VER
		DMA_set_channel_source(&g_InputDMA, IIS_get_input_physical_buffer_address(IIS_CH_0), WORD_UNIT, BURST_1, FIXED);
#else	EBOOK2_VER
        DMA_set_channel_source(&g_InputDMA, IIS_get_input_physical_buffer_address(IIS_CH_2), WORD_UNIT, BURST_1, FIXED);
#endif	EBOOK2_VER
        DMA_set_channel_destination(&g_InputDMA, m_InputDMABufferPhyPage[INPUT_DMA_BUFFER0], WORD_UNIT, BURST_1, INCREASE);
        DMA_set_channel_transfer_size(&g_InputDMA, AUDIO_DMA_PAGE_SIZE);
        DMA_set_initial_LLI(&g_InputDMA, 1);
        DMA_channel_start(&g_InputDMA);

        IIS_set_active_on();
    }
    else
    {
        WAV_ERR((_T("[WAV:ERR] StartInputDMA() : Input DMA is already running\r\n")));
        return FALSE;
    }

    return TRUE;
}


void
HardwareContext::StopInputDMA()
{
    WAV_MSG((_T("[WAV] StopInputDMA()\r\n")));

    if (m_bInputDMARunning)
    {
        DMA_channel_stop(&g_InputDMA);
        // IIS PCM input disable
        IIS_set_rx_mode_control(IIS_TRANSFER_PAUSE);

        m_InputDMAStatus = DMA_CLEAR;

        if (m_bOutputDMARunning == FALSE) IIS_set_active_off();
    }

    m_bInputDMARunning = FALSE;

    CodecMuteControl(DMA_CH_IN, TRUE);
    CodecPowerControl();
}


DWORD
HardwareContext::GetOutputGain (void)
{
    return m_dwOutputGain;
}


MMRESULT
HardwareContext::SetOutputGain (DWORD dwGain)
{
    WAV_MSG((_T("[WAV] SetOutputGain(0x%08x)\r\n"), dwGain));

    m_dwOutputGain = dwGain & 0xffff;    // save off so we can return this from GetGain - but only MONO

    // convert 16-bit gain to 5-bit attenuation
    UCHAR ucGain;
    if (m_dwOutputGain == 0)
    {
        ucGain = 0x3F; // mute: set maximum attenuation
    }
    else
    {
        ucGain = (UCHAR) ((0xffff - m_dwOutputGain) >> 11);    // codec supports 64dB attenuation, we'll only use 32
    }

    //ASSERT((ucGain & 0xC0) == 0); // bits 6,7 clear indicate DATA0 in Volume mode.

    return MMSYSERR_NOERROR;
}


DWORD
HardwareContext::GetInputGain (void)
{
    return m_dwInputGain;
}


MMRESULT
HardwareContext::SetInputGain (DWORD dwGain)
{
    WAV_MSG((_T("[WAV] SetInputGain(0x%08x)\r\n"), dwGain));

    m_dwInputGain = dwGain;

    if (!m_bInputMute)
    {
        m_InputDeviceContext.SetGain(dwGain);
    }

    return MMSYSERR_NOERROR;
}


BOOL
HardwareContext::GetOutputMute (void)
{
    return m_bOutputMute;
}


MMRESULT
HardwareContext::SetOutputMute (BOOL bMute)
{
    WAV_INF((_T("[WAV:INF] ++SetOutputMute(%d)\n\r"), bMute));

    m_bOutputMute = bMute;

#ifdef	EBOOK2_VER
{
	USHORT usData1, usData2, usData3, usData4;
	usData1 = ReadCodecRegister(0x34);
	usData2 = ReadCodecRegister(0x35);
	usData3 = ReadCodecRegister(0x36);
	usData4 = ReadCodecRegister(0x37);
	if (bMute)
	{
		WriteCodecRegister(0x34, usData1 | (1<<6));
		WriteCodecRegister(0x35, usData2 | (1<<6));
		WriteCodecRegister(0x36, usData3 | (1<<6));
		WriteCodecRegister(0x37, usData4 | (1<<6));
	}
	else
	{
		WriteCodecRegister(0x34, usData1 & ~(1<<6));
		WriteCodecRegister(0x35, usData2 & ~(1<<6));
		WriteCodecRegister(0x36, usData3 & ~(1<<6));
		WriteCodecRegister(0x37, usData4 & ~(1<<6));
	}
}
#else	EBOOK2_VER
    if (bMute)
    {
        WriteCodecRegister(WM8580_DAC_CONTROL5, 0x010);    
    }
    else
    {
        WriteCodecRegister(WM8580_DAC_CONTROL5, 0x000);
    }
#endif	EBOOK2_VER

    WAV_INF((_T("[WAV:INF] --SetOutputMute()\n\r")));

    return MMSYSERR_NOERROR;
}


BOOL
HardwareContext::GetInputMute (void)
{
    return m_bInputMute;
}


MMRESULT
HardwareContext::SetInputMute (BOOL bMute)
{
    m_bInputMute = bMute;
    return m_InputDeviceContext.SetGain(bMute ? 0: m_dwInputGain);
}


DWORD
HardwareContext::ForceSpeaker(BOOL bForceSpeaker)
{
    // If m_NumForcedSpeaker is non-zero, audio should be routed to an
    // external speaker (if hw permits).
    if (bForceSpeaker)
    {
        m_NumForcedSpeaker++;
        if (m_NumForcedSpeaker == 1)
        {
            SetSpeakerEnable(TRUE);
        }
    }
    else
    {
        m_NumForcedSpeaker--;
        if (m_NumForcedSpeaker ==0)
        {
            SetSpeakerEnable(FALSE);
        }
    }

    return MMSYSERR_NOERROR;
}


void
HardwareContext::InterruptThreadOutputDMA()
{
    ULONG OutputTransferred;

#if (_WIN32_WCE < 600)
    // Fast way to access embedded pointers in wave headers in other processes.
    SetProcPermissions((DWORD)-1);
#endif

    WAV_INF((_T("[WAV:INF] ++InterruptThreadOutputDMA()\n\r")));

    while(TRUE)
    {
        WaitForSingleObject(m_hOutputDMAInterrupt, INFINITE);

        Lock();

        __try
        {
            DMA_set_interrupt_mask(&g_OutputDMA);
            DMA_clear_interrupt_pending(&g_OutputDMA);

            InterruptDone(m_dwSysintrOutput);

            DMA_clear_interrupt_mask(&g_OutputDMA);

            if ( m_Dx == D0 )
            {
                // DMA Output Buffer is Changed by LLI
                if (m_nOutputBufferInUse == OUTPUT_DMA_BUFFER0)
                {
                    // Buffer0 DMA finished
                    // DMA start with Buffer 1
                    m_nOutputBufferInUse = OUTPUT_DMA_BUFFER1;
                }
                else
                {
                    // Buffer 1 DMA finished
                    // DMA start with Buffer 0
                    m_nOutputBufferInUse = OUTPUT_DMA_BUFFER0;
                }

                if(m_OutputDMAStatus & DMA_BIU)
                {
                    m_OutputDMAStatus &= ~DMA_STRTB;    // Buffer B just completed...
                    m_OutputDMAStatus |= DMA_DONEB;
                    m_OutputDMAStatus &= ~DMA_BIU;        // Buffer A is in use
                }
                else
                {
                    m_OutputDMAStatus &= ~DMA_STRTA;    // Buffer A just completed...
                    m_OutputDMAStatus |= DMA_DONEA;
                    m_OutputDMAStatus |= DMA_BIU;        // Buffer B is in use
                }

                OutputTransferred = TransferOutputBuffer(m_OutputDMAStatus);
            }
        }
        __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            WAV_ERR((_T("WAVDEV2.DLL:InterruptThreadOutputDMA() - EXCEPTION: %d"), GetExceptionCode()));
        }

        Unlock();
    }

    WAV_INF((_T("[WAV:INF] --InterruptThreadOutputDMA()\n\r")));
}


void
HardwareContext::InterruptThreadInputDMA()
{
    ULONG InputTransferred;        // How can I use it ???

#if (_WIN32_WCE < 600)
    // Fast way to access embedded pointers in wave headers in other processes.
    SetProcPermissions((DWORD)-1);
#endif

    WAV_INF((_T("[WAV:INF] ++InterruptThreadInputDMA()\n\r")));

    while(TRUE)
    {
        WaitForSingleObject(m_hInputDMAInterrupt, INFINITE);

        Lock();

        __try
        {
            DMA_set_interrupt_mask(&g_InputDMA);
            DMA_clear_interrupt_pending(&g_InputDMA);

            InterruptDone(m_dwSysintrInput);

            DMA_clear_interrupt_mask(&g_InputDMA);

            if ( m_Dx == D0 )
            {
                if (m_nInputBufferInUse == INPUT_DMA_BUFFER0)
                {
                    // Buffer0 DMA finished
                    // DMA start with Buffer 1
                    m_nInputBufferInUse = INPUT_DMA_BUFFER1;
                }
                else
                {
                    // Buffer 1 DMA finished
                    // DMA start with Buffer 0
                    m_nInputBufferInUse = INPUT_DMA_BUFFER0;
                }

                if(m_InputDMAStatus & DMA_BIU)
                {
                    m_InputDMAStatus &= ~DMA_STRTB;        // Buffer B just completed...
                    m_InputDMAStatus |= DMA_DONEB;
                    m_InputDMAStatus &= ~DMA_BIU;        // Buffer A is in use
                }
                else
                {
                    m_InputDMAStatus &= ~DMA_STRTA;        // Buffer A just completed...
                    m_InputDMAStatus |= DMA_DONEA;
                    m_InputDMAStatus |= DMA_BIU;            // Buffer B is in use
                }

                InputTransferred = TransferInputBuffers(m_InputDMAStatus);
                WAV_INF((_T("[WAV:INF] InputTransferred = %d\n\r"), InputTransferred));
            }
        }
        __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            WAV_ERR((_T("WAVDEV2.DLL:InterruptThreadInputDMA() - EXCEPTION: %d"), GetExceptionCode()));
        }

        Unlock();
    }

    WAV_INF((_T("[WAV:INF] --InterruptThreadInputDMA()\n\r")));
}


BOOL
HardwareContext::MapRegisters()
{
    BOOL bRet = TRUE;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    WAV_MSG((_T("[WAV] ++MapRegisters()\n\r")));

    // Alloc and Map GPIO SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    g_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (g_pGPIOReg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pGPIOReg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map DMAC0 SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA0;    
    g_pDMAC0Reg = (S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (g_pDMAC0Reg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pDMAC0Reg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map DMAC1 SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA1;
    g_pDMAC1Reg = (S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (g_pDMAC1Reg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pDMAC1Reg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map IIS Interface SFR
#ifdef	EBOOK2_VER
	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_IIS0;
#else	EBOOK2_VER
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_I2S_40;      
#endif	EBOOK2_VER
    g_pIISReg = (S3C6410_IIS_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_IIS_REG), FALSE);
    if (g_pIISReg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pIISReg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map System Controller SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;      
    g_pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (g_pSysConReg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pSysConReg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

CleanUp:

    if (bRet == FALSE)
    {
        UnMapRegisters();
    }

    WAV_MSG((_T("[WAV] --MapRegisters()\n\r")));

    return(TRUE);
}


BOOL
HardwareContext::UnMapRegisters()
{
    WAV_MSG((_T("[WAV] UnMapRegisters()\n\r")));

    if (g_pGPIOReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
        g_pGPIOReg = NULL;
    }

    if (g_pIISReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pIISReg, sizeof(S3C6410_IIS_REG));
        g_pIISReg = NULL;
    }

    if (g_pDMAC0Reg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pDMAC0Reg, sizeof(S3C6410_DMAC_REG));
        g_pDMAC0Reg = NULL;
    }

    if (g_pDMAC1Reg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pDMAC1Reg, sizeof(S3C6410_DMAC_REG));
        g_pDMAC1Reg = NULL;
    }

    if (g_pSysConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pSysConReg, sizeof(S3C6410_SYSCON_REG));
        g_pSysConReg = NULL;
    }

    return TRUE;
}


BOOL
HardwareContext::MapDMABuffers()
{
    PVOID pVirtDMABufferAddr = NULL;
    DMA_ADAPTER_OBJECT Adapter;
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++MapDMABuffers()\n\r")));

    memset(&Adapter, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = Internal;

    // Allocate DMA Buffer
    pVirtDMABufferAddr = HalAllocateCommonBuffer(&Adapter, AUDIO_DMA_BUFFER_SIZE, &g_PhyDMABufferAddr, FALSE);
    if (pVirtDMABufferAddr == NULL)
    {
        WAV_MSG((_T("[WAV:ERR] MapDMABuffers() : DMA Buffer Allocation Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Setup the Physical Address of DMA Buffer Page Address
    m_OutputDMABufferPhyPage[0] = (UINT32)g_PhyDMABufferAddr.LowPart;
    m_OutputDMABufferPhyPage[1] = (UINT32)(g_PhyDMABufferAddr.LowPart+AUDIO_DMA_PAGE_SIZE);
    m_InputDMABufferPhyPage[0] = (UINT32)(g_PhyDMABufferAddr.LowPart+AUDIO_DMA_PAGE_SIZE*2);
    m_InputDMABufferPhyPage[1] = (UINT32)(g_PhyDMABufferAddr.LowPart+AUDIO_DMA_PAGE_SIZE*3);

    // Setup the Virtual Address of DMA Buffer Page Address
    m_OutputDMABufferVirPage[0] = (PBYTE)pVirtDMABufferAddr;
    m_OutputDMABufferVirPage[1] = (PBYTE)((UINT32)pVirtDMABufferAddr+AUDIO_DMA_PAGE_SIZE);
    m_InputDMABufferVirPage[0] = (PBYTE)((UINT32)pVirtDMABufferAddr+AUDIO_DMA_PAGE_SIZE*2);
    m_InputDMABufferVirPage[1] = (PBYTE)((UINT32)pVirtDMABufferAddr+AUDIO_DMA_PAGE_SIZE*3);

CleanUp:

    WAV_MSG((_T("[WAV] --MapDMABuffers() : %d\n\r"), bRet));

    return bRet;
}


BOOL
HardwareContext::UnMapDMABuffers()
{
    WAV_MSG((_T("[WAV] UnMapDMABuffers()\n\r")));

    if(m_OutputDMABufferVirPage[0])
    {
        PHYSICAL_ADDRESS PhysicalAddress;
        PhysicalAddress.LowPart = m_OutputDMABufferPhyPage[0];    // No Meaning just for compile

        HalFreeCommonBuffer(0, 0, PhysicalAddress, (PVOID)m_OutputDMABufferVirPage[0], FALSE);

        m_OutputDMABufferVirPage[0] = NULL;
        m_OutputDMABufferVirPage[1] = NULL;
        m_InputDMABufferVirPage[0] = NULL;
        m_InputDMABufferVirPage[1] = NULL;
    }

    return TRUE;
}


BOOL
HardwareContext::InitIISCodec()
{
    DWORD   dwErr = TRUE;
    DWORD   bytes = 0;

    WAV_MSG((_T("[WAV] ++InitIISCodec()\n\r")));

    if ( m_hI2C == INVALID_HANDLE_VALUE )
    {
        dwErr = GetLastError();
        WAV_ERR((_T("[WAV:ERR] InitIISCodec() : m_hI2C is INVALID_HANDLE_VALUE\n\r")));
        return FALSE;
    }

    I2S_Init8580Driver();

    WAV_MSG((_T("[WAV] --InitIISCodec()\n\r")));

    return dwErr;
}


BOOL
HardwareContext::InitOutputDMA()
{
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++InitOutputDMA()\n\r")));

    if (!g_PhyDMABufferAddr.LowPart)
    {
        WAV_ERR((_T("[WAV:ERR] InitOutputDMA() : DMA Buffer is Not Allocated Yet\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

#ifdef	EBOOK2_VER
	bRet = DMA_request_channel(&g_OutputDMA, DMA_I2S0_TX);
#else	EBOOK2_VER
    bRet = DMA_request_channel(&g_OutputDMA, DMA_I2S_V40_TX);
#endif	EBOOK2_VER
    if (bRet)
    {
        DMA_initialize_channel(&g_OutputDMA, TRUE);
        DMA_set_channel_source(&g_OutputDMA, m_OutputDMABufferPhyPage[0], WORD_UNIT, BURST_1, INCREASE);
#ifdef	EBOOK2_VER
		DMA_set_channel_destination(&g_OutputDMA, IIS_get_output_physical_buffer_address(IIS_CH_0), WORD_UNIT, BURST_1, FIXED);
#else	EBOOK2_VER
        DMA_set_channel_destination(&g_OutputDMA, IIS_get_output_physical_buffer_address(IIS_CH_2), WORD_UNIT, BURST_1, FIXED);
#endif	EBOOK2_VER
        DMA_set_channel_transfer_size(&g_OutputDMA, AUDIO_DMA_PAGE_SIZE);
        DMA_initialize_LLI(&g_OutputDMA, 2);
#ifdef	EBOOK2_VER
		DMA_set_LLI_entry(&g_OutputDMA, 0, LLI_NEXT_ENTRY, m_OutputDMABufferPhyPage[0],
							IIS_get_output_physical_buffer_address(IIS_CH_0), AUDIO_DMA_PAGE_SIZE);
		DMA_set_LLI_entry(&g_OutputDMA, 1, LLI_FIRST_ENTRY, m_OutputDMABufferPhyPage[1],
							IIS_get_output_physical_buffer_address(IIS_CH_0), AUDIO_DMA_PAGE_SIZE);
#else	EBOOK2_VER
        DMA_set_LLI_entry(&g_OutputDMA, 0, LLI_NEXT_ENTRY, m_OutputDMABufferPhyPage[0],
                            IIS_get_output_physical_buffer_address(IIS_CH_2), AUDIO_DMA_PAGE_SIZE);
        DMA_set_LLI_entry(&g_OutputDMA, 1, LLI_FIRST_ENTRY, m_OutputDMABufferPhyPage[1],
                            IIS_get_output_physical_buffer_address(IIS_CH_2), AUDIO_DMA_PAGE_SIZE);
#endif	EBOOK2_VER
        DMA_set_initial_LLI(&g_OutputDMA, 1);
    }

CleanUp:

    WAV_MSG((_T("[WAV] --InitOutputDMA()\n\r")));

    return bRet;
}


BOOL HardwareContext::InitInputDMA()
{
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++InitInputDMA()\n\r")));

    if (!g_PhyDMABufferAddr.LowPart)
    {
        WAV_ERR((_T("[WAV:ERR] InitInputDMA() : DMA Buffer is Not Allocated Yet\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

#ifdef	EBOOK2_VER
	bRet = DMA_request_channel(&g_InputDMA, DMA_I2S0_RX);
#else	EBOOK2_VER
    bRet = DMA_request_channel(&g_InputDMA, DMA_I2S_V40_RX);
#endif	EBOOK2_VER
    if (bRet)
    {
        DMA_initialize_channel(&g_InputDMA, TRUE);
#ifdef	EBOOK2_VER
		DMA_set_channel_source(&g_InputDMA, IIS_get_input_physical_buffer_address(IIS_CH_0), WORD_UNIT, BURST_1, FIXED);
#else	EBOOK2_VER
        DMA_set_channel_source(&g_InputDMA, IIS_get_input_physical_buffer_address(IIS_CH_2), WORD_UNIT, BURST_1, FIXED);
#endif	EBOOK2_VER
        DMA_set_channel_destination(&g_InputDMA, m_InputDMABufferPhyPage[0], WORD_UNIT, BURST_1, INCREASE);
        DMA_set_channel_transfer_size(&g_InputDMA, AUDIO_DMA_PAGE_SIZE);
        DMA_initialize_LLI(&g_InputDMA, 2);
        DMA_set_initial_LLI(&g_InputDMA, 1);
#ifdef	EBOOK2_VER
		DMA_set_LLI_entry(&g_InputDMA, 0, LLI_NEXT_ENTRY, IIS_get_input_physical_buffer_address(IIS_CH_0),
							m_InputDMABufferPhyPage[0], AUDIO_DMA_PAGE_SIZE);
		DMA_set_LLI_entry(&g_InputDMA, 1, LLI_FIRST_ENTRY, IIS_get_input_physical_buffer_address(IIS_CH_0),
							m_InputDMABufferPhyPage[1], AUDIO_DMA_PAGE_SIZE);
#else	EBOOK2_VER
        DMA_set_LLI_entry(&g_InputDMA, 0, LLI_NEXT_ENTRY, IIS_get_input_physical_buffer_address(IIS_CH_2),
                            m_InputDMABufferPhyPage[0], AUDIO_DMA_PAGE_SIZE);
        DMA_set_LLI_entry(&g_InputDMA, 1, LLI_FIRST_ENTRY, IIS_get_input_physical_buffer_address(IIS_CH_2),
                            m_InputDMABufferPhyPage[1], AUDIO_DMA_PAGE_SIZE);
#endif	EBOOK2_VER
    }

CleanUp:

    WAV_MSG((_T("[WAV] --InitInputDMA()\n\r")));

    return bRet;
}


BOOL
HardwareContext::InitInterruptThread()
{
    DWORD Irq;
    DWORD dwPriority;

    Irq = g_OutputDMA.dwIRQ;
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(DWORD), &m_dwSysintrOutput, sizeof(DWORD), NULL))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA IOCTL_HAL_REQUEST_SYSINTR Failed \n\r")));
        return FALSE;
    }

    Irq = g_InputDMA.dwIRQ;
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(DWORD), &m_dwSysintrInput, sizeof(DWORD), NULL))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA IOCTL_HAL_REQUEST_SYSINTR Failed \n\r")));
        return FALSE;
    }

    m_hOutputDMAInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hOutputDMAInterrupt == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA CreateEvent() Failed \n\r")));
        return(FALSE);
    }

    m_hInputDMAInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hInputDMAInterrupt == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA CreateEvent() Failed \n\r")));
        return(FALSE);
    }

    if (!InterruptInitialize(m_dwSysintrOutput, m_hOutputDMAInterrupt, NULL, 0))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA InterruptInitialize() Failed \n\r")));
        return FALSE;
    }

    if (! InterruptInitialize(m_dwSysintrInput, m_hInputDMAInterrupt, NULL, 0))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA InterruptInitialize() Failed \n\r")));
        return FALSE;
    }


    m_hOutputDMAInterruptThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
                            (LPTHREAD_START_ROUTINE)CallInterruptThreadOutputDMA, this, 0, NULL);

    if (m_hOutputDMAInterruptThread == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA CreateThread() Failed \n\r")));
        return FALSE;
    }

    m_hInputDMAInterruptThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
                            (LPTHREAD_START_ROUTINE)CallInterruptThreadInputDMA, this, 0, NULL);

    if (m_hInputDMAInterruptThread == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA CreateThread() Failed \n\r")));
        return FALSE;
    }

    dwPriority = GetInterruptThreadPriority();

    // Bump up the priority since the interrupt must be serviced immediately.
    CeSetThreadPriority(m_hOutputDMAInterruptThread, dwPriority);
    CeSetThreadPriority(m_hInputDMAInterruptThread, dwPriority);
    WAV_INF((_T("[WAV:INF] InitInterruptThread() : IST Priority = %d\n\r"), dwPriority));

    return(TRUE);
}


BOOL
HardwareContext::DeinitInterruptThread()
{
    return TRUE;
}


DWORD
HardwareContext::GetInterruptThreadPriority()
{
    HKEY hDevKey;
    DWORD dwValType;
    DWORD dwValLen;
    DWORD dwPrio = INTERRUPT_THREAD_PRIORITY_DEFAULT;
    LONG lResult;

    hDevKey = OpenDeviceKey((LPWSTR)m_DriverIndex);
    if (INVALID_HANDLE_VALUE != hDevKey)
    {
        dwValLen = sizeof(DWORD);
        lResult = RegQueryValueEx(hDevKey, TEXT("Priority256"), NULL, &dwValType, (PUCHAR)&dwPrio, &dwValLen);
        RegCloseKey(hDevKey);
    }
    else
    {
        WAV_ERR((_T("[WAV:ERR] GetInterruptThreadPriority() : OpenDeviceKey() Failed\n\r")));
    }

    return dwPrio;
}


ULONG
HardwareContext::TransferOutputBuffer(DWORD dwStatus)
{
    ULONG BytesTransferred = 0;
    ULONG BytesTotal = 0;

    dwStatus &= (DMA_DONEA|DMA_DONEB|DMA_BIU);

    WAV_MSG((_T("[WAV] TransferOutputBuffer(0x%08x)\n\r"), dwStatus));

    switch (dwStatus)
    {
    case 0:
    case DMA_BIU:
        // No done bits set- must not be my interrupt
        return 0;
    case DMA_DONEA|DMA_DONEB|DMA_BIU:
        // Load B, then A
        BytesTransferred = FillOutputBuffer(OUTPUT_DMA_BUFFER1);
        // fall through
    case DMA_DONEA: // This should never happen!
    case DMA_DONEA|DMA_BIU:
        BytesTransferred += FillOutputBuffer(OUTPUT_DMA_BUFFER0);        // charlie, A => B
        break;
    case DMA_DONEA|DMA_DONEB:
        // Load A, then B
        BytesTransferred = FillOutputBuffer(OUTPUT_DMA_BUFFER0);
        BytesTransferred += FillOutputBuffer(OUTPUT_DMA_BUFFER1);
        break;
    case DMA_DONEB|DMA_BIU: // This should never happen!
    case DMA_DONEB:
        // Load B
        BytesTransferred += FillOutputBuffer(OUTPUT_DMA_BUFFER1);        // charlie, B => A
        break;
    }

    // If it was our interrupt, but we weren't able to transfer any bytes
    // (e.g. no full buffers ready to be emptied)
    // and all the output DMA buffers are now empty, then stop the output DMA
    BytesTotal = m_nOutByte[OUTPUT_DMA_BUFFER0]+m_nOutByte[OUTPUT_DMA_BUFFER1];

    if (BytesTotal == 0)
    {
        StopOutputDMA();
    }
    else
    {
        StartOutputDMA();        // for DMA resume when wake up
    }

    return BytesTransferred;
}


ULONG
HardwareContext::FillOutputBuffer(int nBufferNumber)
{
    ULONG BytesTransferred = 0;
    PBYTE pBufferStart = m_OutputDMABufferVirPage[nBufferNumber];
    PBYTE pBufferEnd = pBufferStart + AUDIO_DMA_PAGE_SIZE;
    PBYTE pBufferLast;

    WAV_MSG((_T("[WAV] FillOutputBuffer(%d)\n\r"), nBufferNumber));

    __try
    {
        pBufferLast = m_OutputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, NULL);

        BytesTransferred = pBufferLast-pBufferStart;
        m_nOutByte[nBufferNumber] = BytesTransferred;

        // Enable if you need to clear the rest of the DMA buffer
        StreamContext::ClearBuffer(pBufferLast, pBufferEnd);

        if(nBufferNumber == OUTPUT_DMA_BUFFER0)            // Output Buffer A
        {
            m_OutputDMAStatus &= ~DMA_DONEA;
            m_OutputDMAStatus |= DMA_STRTA;
        }
        else                                // Output Buffer B
        {
            m_OutputDMAStatus &= ~DMA_DONEB;
            m_OutputDMAStatus |= DMA_STRTB;
        }
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        WAV_ERR((_T("[WAV:ERR] FillOutputBuffer() : Exception ccurs [%d]\n\r"), GetExceptionCode()));
    }

    return BytesTransferred;
}


ULONG
HardwareContext::TransferInputBuffers(DWORD dwStatus)
{
    ULONG BytesTransferred=0;

    dwStatus &= (DMA_DONEA|DMA_DONEB|DMA_BIU);

    WAV_MSG((_T("[WAV] TransferInputBuffers(0x%08x)\n\r"), dwStatus));

    switch (dwStatus)
    {
    case 0:
    case DMA_BIU:
        // No done bits set- must not be my interrupt
        return 0;
    case DMA_DONEA|DMA_DONEB|DMA_BIU:
        // Load B, then A
        BytesTransferred = FillInputBuffer(INPUT_DMA_BUFFER1);
        // fall through
    case DMA_DONEA: // This should never happen!
    case DMA_DONEA|DMA_BIU:
        // Load A
        BytesTransferred += FillInputBuffer(INPUT_DMA_BUFFER0);
        break;
    case DMA_DONEA|DMA_DONEB:
        // Load A, then B
        BytesTransferred = FillInputBuffer(INPUT_DMA_BUFFER0);
        BytesTransferred += FillInputBuffer(INPUT_DMA_BUFFER1);
        break;
    case DMA_DONEB|DMA_BIU: // This should never happen!
    case DMA_DONEB:
        // Load B
        BytesTransferred += FillInputBuffer(INPUT_DMA_BUFFER1);
        break;
    }

    // If it was our interrupt, but we weren't able to transfer any bytes
    // (e.g. no empty buffers ready to be filled)
    // Then stop the input DMA
    if (BytesTransferred==0)
    {
        StopInputDMA();
    }
    else
    {
        StartInputDMA();        // for DMA resume when wake up
    }

    return BytesTransferred;
}


ULONG
HardwareContext::FillInputBuffer(int nBufferNumber)
{
    ULONG BytesTransferred = 0;

    PBYTE pBufferStart = m_InputDMABufferVirPage[nBufferNumber];
    PBYTE pBufferEnd = pBufferStart + AUDIO_DMA_PAGE_SIZE;
    PBYTE pBufferLast;

    WAV_MSG((_T("[WAV] FillInputBuffer(%d)\n\r"), nBufferNumber));

    __try
    {
        pBufferLast = m_InputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, NULL);
        BytesTransferred = m_nInByte[nBufferNumber] = pBufferLast-pBufferStart;

        if(nBufferNumber == INPUT_DMA_BUFFER0)            // Input Buffer A
        {
            m_InputDMAStatus &= ~DMA_DONEA;
            m_InputDMAStatus |= DMA_STRTA;
        }
        else                                                // Input Buffer B
        {
            m_InputDMAStatus &= ~DMA_DONEB;
            m_InputDMAStatus |= DMA_STRTB;
        }

    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        WAV_ERR((_T("[WAV:ERR] FillInputBuffer() : Exception ccurs [%d]\n\r"), GetExceptionCode()));
    }

    return BytesTransferred;
}


void
HardwareContext::WriteCodecRegister(UCHAR Reg, USHORT Val)
{
    // WM8580
    // SFR[B15..B9], DATA[B8..B0]
    // +------------------------+
    // | B15...B9 B8.........B0 |
    // +------------------------+
    //
    UCHAR command0, command1;

    m_WM8580_SFR_Table[Reg] = Val;

    WAV_MSG((_T("[WAV] WriteCodecRegister(0x%02x, 0x%x)\n\r"), Reg, Val));

    command0 = (UCHAR)(Reg<<1)|(UCHAR)((Val>>8)&1);
    command1 = (UCHAR)(Val&0xFF);

    HW_WriteRegisters((PUCHAR)&command1, command0, 1);
}


DWORD
HardwareContext::WriteCodecRegister2(UCHAR Reg, USHORT Val)
{
    DWORD nReturned=0, bytes=0;
    BOOL  bRet = FALSE;

    UCHAR buff[2];
    IIC_IO_DESC IIC_Data;

    // WM8580
    // SFR[B15..B9], DATA[B8..B0]
    // +------------------------+
    // | B15...B9 B8.........B0 |
    // +------------------------+
    //
    UCHAR command0, command1;

    m_WM8580_SFR_Table[Reg] = Val;

    WAV_MSG((_T("[WAV] WriteCodecRegister2(0x%02x, 0x%x)\n\r"), Reg, Val));

    buff[0] = command0 = (UCHAR)(Reg<<1)|(UCHAR)((Val>>8)&1);
    buff[1] = command1 = (UCHAR)(Val&0xFF);

    IIC_Data.SlaveAddress = CHIP_ID;
    IIC_Data.Data = buff;
    IIC_Data.Count = 2;

    bRet = DeviceIoControl(m_hI2C,
                        IOCTL_IIC_WRITE,
                        &IIC_Data, sizeof(IIC_IO_DESC),
                        NULL, 0,
                        &bytes, NULL);

    if (bRet == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("[HWCTXT] ERROR: Device I/O Control Request FAIL, 0x%08X\r\n"), GetLastError()));
        nReturned = -1;
    }

    WAV_MSG((_T("[WAV] WriteCodecRegister2()-\n\r")));

    return nReturned;
}


USHORT
HardwareContext::ReadCodecRegister(UCHAR Reg)
{
    USHORT RegValue = 0;

    //-----------------------------------------------------------------------------------------------------
    // !!!!! Caution !!!!!
    // WM8580 support write operation only.
    // I write sfr data value onto internal table(m_WM8580_SFR_Table), on every write operation for backup.
    // And To support read operation, I just return some value from internal table(m_WM8580_SFR_Table).
    //-----------------------------------------------------------------------------------------------------

    RegValue = m_WM8580_SFR_Table[(int)Reg];

    WAV_MSG((_T("[WAV] ReadCodecRegister(0x%02x) = 0x%03x\n\r"), Reg, RegValue));

    return (USHORT)RegValue;
}


BOOL
HardwareContext::CodecPowerControl()
{
#ifdef	EBOOK2_VER
	if( m_bInputDMARunning & m_bOutputDMARunning )
	{
		WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC & DAC On\n\r")));
		WriteCodecRegister(0x01, 0x01D);
		WriteCodecRegister(0x02, 0x195);
		WriteCodecRegister(0x03, 0x06F);
	}
	else if( m_bInputDMARunning )
	{
		WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC On\n\r")));
		WriteCodecRegister(0x01, 0x01D);
		WriteCodecRegister(0x02, 0x015);
		WriteCodecRegister(0x03, 0x000);
	}
	else if( m_bOutputDMARunning )
	{
		WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() DAC On\n\r")));
		WriteCodecRegister(0x01, 0x00D);
		WriteCodecRegister(0x02, 0x180);
		WriteCodecRegister(0x03, 0x06F);
	}
	else
	{
		WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC & DAC Off\n\r")));
		WriteCodecRegister(0x01, 0x000);
		WriteCodecRegister(0x02, 0x000);
		WriteCodecRegister(0x03, 0x000);
	}
#else	EBOOK2_VER
    if( m_bInputDMARunning & m_bOutputDMARunning )
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC & DAC On\n\r")));
        WriteCodecRegister(WM8580_PWRDN1, WM8580_ALL_POWER_ON);                    // ADC, DAC power up
    }
    else if( m_bInputDMARunning )
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC On\n\r")));
        WriteCodecRegister(WM8580_PWRDN1, WM8580_PWRDN1_ADCPD_ENABLE|WM8580_PWRDN1_DACPD_ALL_DISABLE);        // ADC power up, DAC power down
    }
    else if( m_bOutputDMARunning )
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() DAC On\n\r")));
        WriteCodecRegister(WM8580_PWRDN1, WM8580_PWRDN1_DACPD_ALL_ENABLE|WM8580_PWRDN1_ADCPD_DISABLE);   // DAC power up, ADC power down    
    }
    else
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC & DAC Off\n\r")));
        WriteCodecRegister(WM8580_PWRDN1, WM8580_PWRDN1_ADCPD_DISABLE|WM8580_PWRDN1_DACPD_ALL_DISABLE ); // ADC/DAC power down
    }
#endif	EBOOK2_VER

    return(TRUE);
}


BOOL
HardwareContext::CodecMuteControl(DWORD channel, BOOL bMute)
{

    if(channel & DMA_CH_OUT)
    {
#ifdef	EBOOK2_VER
		USHORT usData1, usData2, usData3, usData4;
		usData1 = ReadCodecRegister(0x34);
		usData2 = ReadCodecRegister(0x35);
		usData3 = ReadCodecRegister(0x36);
		usData4 = ReadCodecRegister(0x37);
		if (bMute)
		{
			WriteCodecRegister(0x34, usData1 | (1<<6));
			WriteCodecRegister(0x35, usData2 | (1<<6));
			WriteCodecRegister(0x36, usData3 | (1<<6));
			WriteCodecRegister(0x37, usData4 | (1<<6));
		}
		else
		{
			WriteCodecRegister(0x34, usData1 & ~(1<<6));
			WriteCodecRegister(0x35, usData2 & ~(1<<6));
			WriteCodecRegister(0x36, usData3 & ~(1<<6));
			WriteCodecRegister(0x37, usData4 & ~(1<<6));
		}
#else	EBOOK2_VER
        if(bMute)
        {
            WriteCodecRegister(WM8580_DAC_CONTROL5, 0x010);
        }
        else
        {
            WriteCodecRegister(WM8580_DAC_CONTROL5, 0x000);
        }
#endif	EBOOK2_VER
    }
    if(channel & DMA_CH_IN) 
    {
#ifdef	EBOOK2_VER
		USHORT usData;
		usData = ReadCodecRegister(0x2D);
		if (bMute)
			WriteCodecRegister(0x2D, usData | (1<<6));
		else
			WriteCodecRegister(0x2D, usData & ~(1<<6));
#else	EBOOK2_VER
        if(bMute)
        {
            WriteCodecRegister(WM8580_ADC_CONTROL1, 0x044);
        }
        else
        {
            WriteCodecRegister(WM8580_ADC_CONTROL1, 0x040);
        }
#endif	EBOOK2_VER
    }

    return(TRUE);
}


void HardwareContext::SetSpeakerEnable(BOOL bEnable)
{
    // Code to turn speaker on/off here
    return;
}


void CallInterruptThreadOutputDMA(HardwareContext *pHWContext)
{
    pHWContext->InterruptThreadOutputDMA();
}


void CallInterruptThreadInputDMA(HardwareContext *pHWContext)
{
    pHWContext->InterruptThreadInputDMA();
}


// Write WM8580 IIS Codec registers directly from our cache
DWORD HardwareContext::HW_WriteRegisters(
    PUCHAR pBuff,       // Optional buffer
    UCHAR StartReg,     // start register
    DWORD nRegs         // number of registers
    )
{
    DWORD dwErr=0,bytes;
    UCHAR buff[2];
    IIC_IO_DESC IIC_Data;


    buff[0] = StartReg;
    buff[1] = pBuff[0];

    IIC_Data.SlaveAddress = WM8580_WRITE;
    
    IIC_Data.Data = buff;
    IIC_Data.Count = 2;

    WAV_MSG((_T("[WAV] HW_WriteRegisters()\r\n")));

    // use iocontrol to write
    if ( !DeviceIoControl(m_hI2C,
                        IOCTL_IIC_WRITE,
                        &IIC_Data, sizeof(IIC_IO_DESC),
                        NULL, 0,
                        &bytes, NULL) )
    {
        dwErr = GetLastError();
    }

    if ( dwErr )
    {
        WAV_ERR((_T("[WAV::ERR]I2CWrite ERROR: %u \r\n"), dwErr));
    }

    return dwErr;
}


DWORD HardwareContext::HW_ReadRegisters(
    PUCHAR pBuff,       // Optional buffer
    UCHAR StartReg,     // start register
    DWORD nRegs         // number of registers
    )
{
    DWORD dwErr=0;
    DWORD bytes;
    IIC_IO_DESC IIC_AddressData, IIC_Data;

    IIC_AddressData.SlaveAddress = WM8580_READ;
    
    IIC_AddressData.Data = &StartReg;
    IIC_AddressData.Count = 1;

    IIC_Data.SlaveAddress = WM8580_READ;
    
    IIC_Data.Data = pBuff;
    IIC_Data.Count = 1;

    WAV_MSG((_T("[WAV] HW_ReadRegisters()\r\n")));

    // use iocontrol to read
    if ( !DeviceIoControl(m_hI2C,
                        IOCTL_IIC_READ,
                        &IIC_AddressData, sizeof(IIC_IO_DESC),
                        &IIC_Data, sizeof(IIC_IO_DESC),
                        &bytes, NULL) )
    {
        dwErr = GetLastError();
    }

    if ( dwErr )
    {
        WAV_ERR((_T("[WAV::ERR]I2CRead ERROR: %u \r\n"), dwErr));
    }

    return dwErr;
}


void HardwareContext::I2S_Init8580Driver()
{
    int i;

    WAV_MSG((_T("[WAV] +I2S_Init8580Driver()\n\r")));

    // Initialize WM8580_SFR_Table data
    for (i=0; i<WM8580_MAX_REGISTER_COUNT; i++)
    {
        m_WM8580_SFR_Table[i] = 0;
    }

    // Default setting value
    for(i=0; i<(sizeof(WM8580_Codec_Init_Table)/sizeof(unsigned int)/2); i++)
    {
        WriteCodecRegister(WM8580_Codec_Init_Table[i][0], WM8580_Codec_Init_Table[i][1]);
    }
}
