//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#include <windows.h>
#include <wavedbg.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "s3c6410_iis_interface_macro.h"
#include "s3c6410_iis_interface.h"

#define IIS_MSG(x)
#define IIS_INF(x)    DEBUGMSG(ZONE_FUNCTION, x)
#define IIS_ERR(x)    DEBUGMSG(ZONE_ERROR, x)

#define DELAY_LOOP_COUNT    (S3C6410_ACLK/100000)
#define CONTROL_DELAY        (10)
#define CMD_DELAY            (3)

static volatile S3C6410_GPIO_REG * g_pGPIOReg = NULL;
static volatile S3C6410_IIS_REG * g_pIISReg = NULL;

IIS_ERROR IIS_initialize_register_address(void *pIISReg, void *pGPIOReg)
{
    IIS_ERROR error = IIS_SUCCESS;

    IIS_MSG((_T("[IIS]++IIS_initialize_register_address(0x%08x, 0x%08x)\n\r"), pIISReg, pGPIOReg));

    if (pIISReg == NULL || pGPIOReg == NULL)
    {
        IIS_ERR((_T("[IIS:ERR] IIS_initialize_register_address() : NULL pointer parameter\n\r")));
        error = IIS_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pIISReg = (S3C6410_IIS_REG *)pIISReg;
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        IIS_INF((_T("[IIS:INF] g_pIISReg = 0x%08x\n\r"), g_pIISReg));
        IIS_INF((_T("[IIS:INF] g_pGPIOReg = 0x%08x\n\r"), g_pGPIOReg));
    }

    IIS_MSG((_T("[IIS]--IIS_initialize_register_address() : %d\n\r"), error));

    return error;
}

void IIS_initialize_interface(void)
{
    IIS_MSG((_T("[IIS] ++IIS_initialize_interface()\n\r")));

    IIS_set_active_off();

    // Initialize IIS Interface
#ifdef	OMNIBOOK_VER
	IIS_port_initialize(IIS_CH_0);
#else	//!OMNIBOOK_VER
    IIS_port_initialize(IIS_CH_2);
#endif	OMNIBOOK_VER

#if    1    // Sampling Rate 44.1 KHz with EPLL 84,666,667 Hz
    // Configurre IIS clock prescale value
#ifdef	OMNIBOOK_VER
	IIS_set_prescale_value(5);            // EPLL Fout/(5+1) = 67,738,000 Hz / 6 = 11,289,667 Hz (256fs)
#else	//!OMNIBOOK_VER
    IIS_set_prescale_value(4);            // EPLL Fout/(4+1) = 84,666,667 Hz / 5 = 16,933,334 Hz
#endif	OMNIBOOK_VER
    IIS_set_prescale_enable();

    // Flush IIS fifo
    IIS_fifo_flush(IIS_TX_FIFO_FLUSH);
    IIS_fifo_flush(IIS_TX_FIFO_NOFLUSH);
    IIS_fifo_flush(IIS_RX_FIFO_FLUSH);
    IIS_fifo_flush(IIS_RX_FIFO_NOFLUSH);

    //
    IIS_set_transfer_mode(IIS_TRANSFER_TX);

    //rI2SMOD : TxOnlyMode, I2SRootClk, BitClk, BitLength
    //IIS_set_interface_mode(ISS_TRANSFER_TX, eLEFT_LOW_RIGHT_HIGH, eI2S_DATA_I2S, ucCODECLK, ucBCLK, ucBLTH);

    IIS_set_interface_codec_clock_source(IIS_INTERNEL_CDCLK);
    IIS_set_interface_master_slave_mode(IIS_MASTER_BYPASS_MODE);
    IIS_set_interface_tranmit_receive_mode(IIS_TRANSFER_BOTH);
    IIS_set_interface_clock_polarity(LOW_FOR_LEFT);
    IIS_set_interface_serial_data_format(IIS_FORMAT);
#ifdef	OMNIBOOK_VER
    IIS_set_interface_codec_clock_frequency(IIS_CODEC_CLOCK_256FS);
#else	//!OMNIBOOK_VER
    IIS_set_interface_codec_clock_frequency(IIS_CODEC_CLOCK_384FS);
#endif	OMNIBOOK_VER
    IIS_set_interface_bit_clock_frequency(IIS_BIT_CLOCK_32FS);
    IIS_set_interface_bit_length(IIS_BIT_LENGTH_16BIT);
#else
    // Configurre IIS clock prescale value
    IIS_set_prescale_value(1);
    IIS_set_prescale_enable();

    // Flush IIS fifo
    IIS_fifo_flush(IIS_TX_FIFO_FLUSH);
    IIS_fifo_flush(IIS_TX_FIFO_NOFLUSH);
    IIS_fifo_flush(IIS_RX_FIFO_FLUSH);
    IIS_fifo_flush(IIS_RX_FIFO_NOFLUSH);

    //
    IIS_set_transfer_mode(IIS_TRANSFER_TX); //IIS_TRANSFER_TX);

    //rI2SMOD : TxOnlyMode, I2SRootClk, BitClk, BitLength
    //IIS_set_interface_mode(ISS_TRANSFER_TX, eLEFT_LOW_RIGHT_HIGH, eI2S_DATA_I2S, ucCODECLK, ucBCLK, ucBLTH);

    IIS_set_interface_codec_clock_source(IIS_TRANSFER_BOTH); //IIS_TRANSFER_BOTH);
    IIS_set_interface_master_slave_mode(IIS_MASTER_DIVIDE_MODE);
    IIS_set_interface_tranmit_receive_mode(IIS_TRANSFER_BOTH); //IIS_TRANSFER_BOTH
    IIS_set_interface_clock_polarity(LOW_FOR_LEFT);
    IIS_set_interface_serial_data_format(IIS_FORMAT);
    IIS_set_interface_codec_clock_frequency(IIS_CODEC_CLOCK_384FS);
    IIS_set_interface_bit_clock_frequency(IIS_BIT_CLOCK_32FS);
    IIS_set_interface_bit_length(IIS_BIT_LENGTH_16BIT);
#endif

    IIS_MSG((_T("[IIS] --IIS_initialize_interface()\n\r")));
}

unsigned int IIS_get_output_physical_buffer_address(IIS_CHANNEL mChannel)
{
    unsigned int m_physical_buffer_address = 0;
    if (mChannel == IIS_CH_0) m_physical_buffer_address = (unsigned int)(0x7F002010);
    if (mChannel == IIS_CH_1) m_physical_buffer_address = (unsigned int)(0x7F003010);
    if (mChannel == IIS_CH_2) m_physical_buffer_address = (unsigned int)(0x7F00D010);
    
    return m_physical_buffer_address;
}

unsigned int IIS_get_input_physical_buffer_address(IIS_CHANNEL mChannel)
{
    unsigned int m_physical_buffer_address = 0;
    if (mChannel == IIS_CH_0) m_physical_buffer_address = (unsigned int)(0x7F002014);
    if (mChannel == IIS_CH_1) m_physical_buffer_address = (unsigned int)(0x7F003014);
    if (mChannel == IIS_CH_2) m_physical_buffer_address = (unsigned int)(0x7F00D014);
    
    return m_physical_buffer_address;
}

PRIVATE void IIS_port_initialize(IIS_CHANNEL mChnNum)
{
    IIS_MSG((_T("[IIS] IIS_port_initialize()\n\r")));

    ASSERT( (mChnNum == IIS_CH_0) || (mChnNum == IIS_CH_1)  || (mChnNum == IIS_CH_2));

    // IIS CLK        : GPD[0], GPE[0]
    // IIS CDCLK    : GPD[1], GPE[1]
    // IIS LRCLK    : GPD[2], GPE[2]
    // IIS DI        : GPD[3], GPE[3]
    // IIS DO        : GPD[4], GPE[4]

    if (mChnNum == IIS_CH_0)
    {
        // IIS CH 0 Use GPD Port
        g_pGPIOReg->GPDPUD &= ~(0x3ff);    // Pull-Up/Down Disable
        g_pGPIOReg->GPDCON = 0x33333;         // GPD -> IIS
    }
    else if (mChnNum == IIS_CH_1)
    {
        // IIS CH 1 Use GPE Port
        g_pGPIOReg->GPEPUD &= ~(0x3ff);    // Pull-Up/Down Disable
        g_pGPIOReg->GPECON = 0x33333;         // GPD -> IIS
    }
    else if (mChnNum == IIS_CH_2)
    {
        // IIS CH 2 Use GPC, GPH Port
        g_pGPIOReg->GPCPUD &= ~((0x3<<8)|(0x3<<10)|(0x3<<14));    // Pull-Up/Down Disable
        g_pGPIOReg->GPCCON = (g_pGPIOReg->GPCCON & ~((0xf<<16)|(0xf<<20)|(0xf<<28))) | ((5<<16)|(5<<20)|(5<<28));         // GPC -> IIS

        g_pGPIOReg->GPHPUD &= ~((0x3<<12)|(0x3<<14)|(0x3<<16)|(0x3<<18));    // Pull-Up/Down Disable
        g_pGPIOReg->GPHCON0 = (g_pGPIOReg->GPHCON0 & ~((0xf<<24)|(0xf<<28))) | ((5<<24)|(5<<28));         // GPH -> IIS
        g_pGPIOReg->GPHCON1 = (g_pGPIOReg->GPHCON1 & ~((0xf<< 0)|(0xf<< 4))) | ((5<< 0)|(5<< 4));
    }

}

PRIVATE void IIS_set_prescale_value(DWORD mPrescaleValue)
{
    mPrescaleValue = mPrescaleValue & PRESCALER_A_DIVISION_VALUE_MASK; // PSVALA[9:0]

    IIS_MSG((_T("[IIS] Prescale Value = %d\r\n"), mPrescaleValue));
    IIS_MSG((_T("[IIS] Masked Prescale Value = %d\r\n"), (mPrescaleValue&PRESCALER_A_DIVISION_VALUE_MASK)));

    g_pIISReg->IISPSR = (g_pIISReg->IISPSR & ~(PRESCALER_A_DIVISION_VALUE_MASK<<PRESCALER_A_DIVISION_VALUE_SHIFT));
    g_pIISReg->IISPSR |= (mPrescaleValue<<PRESCALER_A_DIVISION_VALUE_SHIFT);
}

PRIVATE void IIS_set_prescale_enable()
{
    IIS_MSG((_T("[IIS] Prescale Enabled\r\n")));

    g_pIISReg->IISPSR |= PRESCALER_A_ACTIVE;
}

PRIVATE void IIS_set_prescale_disable()
{
    IIS_MSG((_T("[IIS] Prescale Disabled\r\n")));

    g_pIISReg->IISPSR &= ~(PRESCALER_A_ACTIVE);
}


PRIVATE void IIS_fifo_flush(IIS_FIFO_FLUSH mFlushMode)
{
    ASSERT( mFlushMode <= IIS_RX_FIFO_NOFLUSH );

    IIS_MSG((_T("[IIS] FIFO Flush Mode [%d] -> [%s][%s]\r\n"), mFlushMode,
        ((mFlushMode/2)? _T("RX") : _T("TX")), ((mFlushMode%2)? _T("NOFLUSH") : _T("FLUSH"))));

    switch(mFlushMode)
    {
        case IIS_TX_FIFO_FLUSH: g_pIISReg->IISFIC |= (TX_FIFO_FLUSH); break;
        case IIS_TX_FIFO_NOFLUSH: g_pIISReg->IISFIC &= ~(TX_FIFO_FLUSH); break;
        case IIS_RX_FIFO_FLUSH: g_pIISReg->IISFIC |= (RX_FIFO_FLUSH); break;
        case IIS_RX_FIFO_NOFLUSH: g_pIISReg->IISFIC &= ~(RX_FIFO_FLUSH); break;
        default: break;
    }
}

PRIVATE void IIS_set_transfer_mode(IIS_TRANSFER_MODE mTransferMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_transfer_mode [%d]\r\n"), mTransferMode));

    uRegValue = g_pIISReg->IISCON & ~(TX_MOD_MASK|RX_MOD_MASK);

    // IISCON : TxDMAPause(6), RxDMAPause(5), TxCHPause(4), RxCHPause(3), TxDMAActive(2), RxDMAActive(1)
    switch(mTransferMode)
    {
        case IIS_TRANSFER_TX :
            uRegValue = uRegValue |(TX_DMA_NOPAUSE) | (RX_DMA_PAUSE) | (TX_CHANNEL_NOPAUSE) | (RX_CHANNEL_PAUSE) | (TX_DMA_ACTIVE) | (RX_DMA_INACTIVE);
            break;
        case IIS_TRANSFER_RX :
            uRegValue = uRegValue | (TX_DMA_PAUSE) | (RX_DMA_NOPAUSE) | (TX_CHANNEL_PAUSE) | (RX_CHANNEL_NOPAUSE) | (TX_DMA_INACTIVE) | (RX_DMA_ACTIVE);
            break;
        case IIS_TRANSFER_BOTH :
            uRegValue = uRegValue | (TX_DMA_NOPAUSE) | (RX_DMA_NOPAUSE) | (TX_CHANNEL_NOPAUSE) | (RX_CHANNEL_NOPAUSE) | (TX_DMA_ACTIVE) | (RX_DMA_ACTIVE);
            break;
        default :
            break;
    }

    g_pIISReg->IISCON = uRegValue;
}

void IIS_set_tx_mode_control(IIS_TRANSFER_PAUSE_CONTROL mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_tx_mode_control [%d]\r\n"), mMode));

    uRegValue = g_pIISReg->IISCON & ~(TX_MOD_MASK);

    switch(mMode)
    {
        case IIS_TRANSFER_NOPAUSE :
            uRegValue = uRegValue |(TX_DMA_NOPAUSE) | (TX_CHANNEL_NOPAUSE) | (TX_DMA_ACTIVE);
            break;
        case IIS_TRANSFER_PAUSE :
            uRegValue = uRegValue | (TX_DMA_PAUSE) | (TX_CHANNEL_PAUSE) | (TX_DMA_INACTIVE);
            break;
        default :
            break;
    }

    g_pIISReg->IISCON = uRegValue;
}

void IIS_set_rx_mode_control(IIS_TRANSFER_PAUSE_CONTROL mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_rx_mode_control [%d]\r\n"), mMode));

    uRegValue = g_pIISReg->IISCON & ~(RX_MOD_MASK);

    switch(mMode)
    {
        case IIS_TRANSFER_NOPAUSE :
            uRegValue = uRegValue | (RX_DMA_NOPAUSE) | (RX_CHANNEL_NOPAUSE) | (RX_DMA_ACTIVE);
            break;
        case IIS_TRANSFER_PAUSE :
            uRegValue = uRegValue | (RX_DMA_PAUSE) | (RX_CHANNEL_PAUSE) | (RX_DMA_INACTIVE);
            break;
        default :
            break;
    }

    g_pIISReg->IISCON = uRegValue;
}


PRIVATE void IIS_set_interface_codec_clock_source(IIS_CODEC_CLOCK_MODE mCodecClockSource)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_mode_codec_clock_source(%d)\r\n"), mCodecClockSource));

    uRegValue = (g_pIISReg->IISMOD & ~(CODEC_CLOCK_SOURCE_MASK));
    if (mCodecClockSource == IIS_INTERNEL_CDCLK) uRegValue |= CODEC_CLK_USE_INTERNEL_CLK;
    if (mCodecClockSource == IIS_EXTERNEL_CDCLK) uRegValue |= CODEC_CLK_USE_EXTERNEL_CLK;
    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_master_slave_mode(IIS_MASTER_SLAVE_MODE mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_master_slave_mode(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(IIS_CLK_MASTER_SLAVE_MASK));
    if (mMode == IIS_MASTER_DIVIDE_MODE) uRegValue |= IIS_CLK_MASTER_PCLK_DIV_MODE;
    if (mMode == IIS_MASTER_BYPASS_MODE) uRegValue |= IIS_CLK_MASTER_IISCLK_BYPASS_MODE;
    if (mMode == IIS_SLAVE_MODE) uRegValue |= IIS_CLK_SLAVE_MODE;

    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_tranmit_receive_mode(IIS_TRANSFER_MODE mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_tranmit_receive_mode(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(TRANSFER_MODE_MASK));
    if (mMode == IIS_TRANSFER_TX) uRegValue |= TRANSFER_MODE_TX_ONLY;
    if (mMode == IIS_TRANSFER_RX) uRegValue |= TRANSFER_MODE_RX_ONLY;
    if (mMode == IIS_TRANSFER_BOTH) uRegValue |= TRANSFER_MODE_BOTH;

    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_clock_polarity(IIS_CLOCK_POLARITY mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_clock_polarity(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(LR_CH_POL_MASK));
    if (mMode == LOW_FOR_LEFT) uRegValue |= LR_CH_POL_LOW_FOR_LEFT_HIGH_FOR_RIGHT;
    if (mMode == HIGH_FOR_LEFT) uRegValue |= LR_CH_POL_HIGH_FOR_LEFT_LOW_FOR_RIGHT;

    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_serial_data_format(IIS_DATA_FORMAT mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_serial_data_format(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(SERIAL_DATA_FORMAT_MASK));
    if (mMode == IIS_FORMAT) uRegValue |= SERIAL_DATA_FORMAT_IIS;
    if (mMode == MSB_JUSTIFIED_FORMAT) uRegValue |= SERIAL_DATA_FORMAT_MSB_JUSTIFIED;
    if (mMode == LSB_JUSTIFIED_FORMAT) uRegValue |= SERIAL_DATA_FORMAT_LSB_JUSTIFIED;

    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_codec_clock_frequency(IIS_CODEC_CLOCK_MODE mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_codec_clock_frequency(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(IIS_CODEC_CLOCK_FREQUENCY_MASK));
    if (mMode == IIS_CODEC_CLOCK_256FS) uRegValue |= IIS_CODEC_CLOCK_FREQUENCY_256FS;
    if (mMode == IIS_CODEC_CLOCK_512FS) uRegValue |= IIS_CODEC_CLOCK_FREQUENCY_512FS;
    if (mMode == IIS_CODEC_CLOCK_384FS) uRegValue |= IIS_CODEC_CLOCK_FREQUENCY_384FS;
    if (mMode == IIS_CODEC_CLOCK_768FS) uRegValue |= IIS_CODEC_CLOCK_FREQUENCY_768FS;

    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_bit_clock_frequency(IIS_BIT_CLOCK_MODE mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_bit_clock_frequency(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(BIT_CLOCK_FREQUENCY_MASK));
    if (mMode == IIS_BIT_CLOCK_32FS) uRegValue |= BIT_CLOCK_FREQUENCY_32FS;
    if (mMode == IIS_BIT_CLOCK_48FS) uRegValue |= BIT_CLOCK_FREQUENCY_48FS;
    if (mMode == IIS_BIT_CLOCK_16FS) uRegValue |= BIT_CLOCK_FREQUENCY_16FS;
    if (mMode == IIS_BIT_CLOCK_24FS) uRegValue |= BIT_CLOCK_FREQUENCY_24FS;

    g_pIISReg->IISMOD = uRegValue;
}

PRIVATE void IIS_set_interface_bit_length(IIS_BIT_LENGTH mMode)
{
    DWORD uRegValue = 0;

    IIS_MSG((_T("[IIS] IIS_set_interface_bit_length(%d)\r\n"), mMode));

    uRegValue = (g_pIISReg->IISMOD & ~(BIT_LENGTH_PER_CHANNEL_MASK));
    if (mMode == IIS_BIT_LENGTH_16BIT) uRegValue |= BIT_LENGTH_PER_CHANNEL_16BIT;
    if (mMode == IIS_BIT_LENGTH_8BIT) uRegValue |= BIT_LENGTH_PER_CHANNEL_8BIT;
    if (mMode == IIS_BIT_LENGTH_24BIT) uRegValue |= BIT_LENGTH_PER_CHANNEL_24BIT;

    g_pIISReg->IISMOD = uRegValue;
}


PRIVATE void IIS_set_active_on(void)
{
    IIS_MSG((_T("[IIS] IIS_set_active_on()\n\r")));

    if (g_pIISReg->IISCON & IIS_ACTIVE)
        IIS_MSG((_T("IIS is already active\r\n")));
    else
        g_pIISReg->IISCON |= IIS_ACTIVE;
}

PRIVATE void IIS_set_active_off(void)
{
    IIS_MSG((_T("[IIS] IIS_set_active_off()\n\r")));

    g_pIISReg->IISCON &= ~IIS_ACTIVE;
}

PRIVATE void DelayLoop(unsigned int count)
{
    volatile int i, j=0;

    for(;count > 0;count--)
        for(i=0;i < DELAY_LOOP_COUNT; i++) { j++; }
}

