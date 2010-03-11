//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#ifndef __S3C6410_IIS_INTERFACE_H__
#define __S3C6410_IIS_INTERFACE_H__

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
    IIS_CH_0 = 0,
    IIS_CH_1,
    IIS_CH_2
} IIS_CHANNEL;

typedef enum
{
    IIS_CH_OFF = 0,
    IIS_CH_ON
} IIS_CHANNEL_MODE;

typedef enum
{
    IIS_INTERNEL_CDCLK = 0,
    IIS_EXTERNEL_CDCLK
} IIS_CLOCK_SOURCE;

typedef enum
{
    IIS_MASTER_DIVIDE_MODE = 0,    // using PCLK
    IIS_MASTER_BYPASS_MODE,        // using IISCLK
    IIS_SLAVE_MODE
} IIS_MASTER_SLAVE_MODE;


typedef enum
{
    LOW_FOR_LEFT = 0,    // HIGH_FOR_RIGHT
    HIGH_FOR_LEFT        // LOW_FOR_LEFT
} IIS_CLOCK_POLARITY;

typedef enum
{
    IIS_FORMAT = 0,
    MSB_JUSTIFIED_FORMAT,
    LSB_JUSTIFIED_FORMAT
} IIS_DATA_FORMAT;

typedef enum
{
    IIS_CODEC_CLOCK_256FS = 0,
    IIS_CODEC_CLOCK_512FS,
    IIS_CODEC_CLOCK_384FS,
    IIS_CODEC_CLOCK_768FS
} IIS_CODEC_CLOCK_MODE;

typedef enum
{
    IIS_BIT_CLOCK_32FS = 0,
    IIS_BIT_CLOCK_48FS,
    IIS_BIT_CLOCK_16FS,
    IIS_BIT_CLOCK_24FS
} IIS_BIT_CLOCK_MODE;

typedef enum
{
    IIS_BIT_LENGTH_16BIT = 0,
    IIS_BIT_LENGTH_8BIT,
    IIS_BIT_LENGTH_24BIT
} IIS_BIT_LENGTH;

typedef enum
{
    IIS_TX_FIFO_FLUSH = 0,
    IIS_TX_FIFO_NOFLUSH,
    IIS_RX_FIFO_FLUSH,
    IIS_RX_FIFO_NOFLUSH    
} IIS_FIFO_FLUSH;


typedef enum
{
    IIS_TRANSFER_TX        = 0,
    IIS_TRANSFER_RX,
    IIS_TRANSFER_BOTH
} IIS_TRANSFER_MODE;

typedef enum
{
    IIS_TRANSFER_NOPAUSE    = 0,
    IIS_TRANSFER_PAUSE
} IIS_TRANSFER_PAUSE_CONTROL;

typedef enum
{
    IIS_SUCCESS,
    IIS_ERROR_NULL_PARAMETER,
    IIS_ERROR_ILLEGAL_PARAMETER,
    IIS_ERROR_NOT_INITIALIZED,
    IIS_ERROR_NOT_IMPLEMENTED,
    IIS_ERROR_XXX
} IIS_ERROR;

IIS_ERROR IIS_initialize_register_address(void *pIISReg, void *pGPIOReg);

void IIS_initialize_interface(void);

void IIS_write_codec(unsigned char ucReg, unsigned short usData);
unsigned short IIS_read_codec(unsigned char ucReg);

unsigned int IIS_get_output_physical_buffer_address(IIS_CHANNEL mChannel);
unsigned int IIS_get_input_physical_buffer_address(IIS_CHANNEL mChannel);

#define PRIVATE 
//#define PRIVATE static

PRIVATE void IIS_port_initialize(IIS_CHANNEL mChnNum);

PRIVATE void IIS_set_prescale_value(DWORD mPrescaleValue);
PRIVATE void IIS_set_prescale_enable();
PRIVATE void IIS_set_prescale_disable();

PRIVATE void IIS_fifo_flush(IIS_FIFO_FLUSH mFlushMode);
PRIVATE void IIS_set_transfer_mode(IIS_TRANSFER_MODE mTransferMode);

void IIS_set_tx_mode_control(IIS_TRANSFER_PAUSE_CONTROL mMode);
void IIS_set_rx_mode_control(IIS_TRANSFER_PAUSE_CONTROL mMode);

PRIVATE void IIS_set_interface_codec_clock_source(IIS_CODEC_CLOCK_MODE mCodecClockSource);
PRIVATE void IIS_set_interface_master_slave_mode(IIS_MASTER_SLAVE_MODE mMode);
PRIVATE void IIS_set_interface_tranmit_receive_mode(IIS_TRANSFER_MODE mMode);
PRIVATE void IIS_set_interface_clock_polarity(IIS_CLOCK_POLARITY mMode);
PRIVATE void IIS_set_interface_serial_data_format(IIS_DATA_FORMAT mMode);
PRIVATE void IIS_set_interface_codec_clock_frequency(IIS_CODEC_CLOCK_MODE mMode);
PRIVATE void IIS_set_interface_bit_clock_frequency(IIS_BIT_CLOCK_MODE mMode);
PRIVATE void IIS_set_interface_bit_length(IIS_BIT_LENGTH mMode);

PRIVATE void IIS_set_active_on(void);
PRIVATE void IIS_set_active_off(void);

PRIVATE void DelayLoop(unsigned int count);

#if __cplusplus
}
#endif

#endif    // __S3C6410_IIS_INTERFACE_H__
