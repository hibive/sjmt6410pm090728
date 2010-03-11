// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/
//
// s3c6410_iis7_interface_macro.h

#ifndef _S3C6410_IIS_INTERFACE_MACRO_H_
#define _S3C6410_IIS_INTERFACE_MACRO_H_

#if __cplusplus
extern "C"
{
#endif

// IISCON : IIS Interface Control Register
#define TX_FIFO_UNDERRUN_STATUS                 (1 << 17)
#define TX_FIFO_UNDERRUN_INT_ENABLE             (1 << 16)
#define TX_FIFO_2_EMPTY_STATUS                  (1 << 15)
#define TX_FIFO_1_EMPTY_STATUS                  (1 << 14)
#define TX_FIFO_2_FULL_SATATUS                  (1 << 13)
#define TX_FIFO_1_FULL_SATATUS                  (1 << 12)

#define LEFT_RIGHT_INDICATION_STATUS            (1 << 11)
#define TX_FIFO_EMPTY_STATUS                    (1 << 10)
#define RX_FIFO_EMPTY_STATUS                    (1 <<  9)
#define TX_FIFO_FULL_STATUS                        (1 <<  8)
#define RX_FIFO_FULL_STATUS                        (1 <<  7)

#define IISCON_STATUS_MASK                        (0x1F << 7)


#define TX_DMA_NOPAUSE                            (0 <<  6)
#define TX_DMA_PAUSE                            (1 <<  6)
#define RX_DMA_NOPAUSE                            (0 <<  5)
#define RX_DMA_PAUSE                            (1 <<  5)
#define TX_CHANNEL_NOPAUSE                        (0 <<  4)
#define TX_CHANNEL_PAUSE                        (1 <<  4)
#define RX_CHANNEL_NOPAUSE                        (0 <<  3)
#define RX_CHANNEL_PAUSE                        (1 <<  3)
#define TX_DMA_INACTIVE                            (0 <<  2)
#define TX_DMA_ACTIVE                            (1 <<  2)
#define RX_DMA_INACTIVE                            (0 <<  1)
#define RX_DMA_ACTIVE                            (1 <<  1)
#define IIS_INACTIVE                            (0 <<  0)
#define IIS_ACTIVE                                (1 <<  0)

#define TX_MOD_MASK        (TX_DMA_PAUSE|TX_CHANNEL_PAUSE|TX_DMA_ACTIVE)
#define RX_MOD_MASK        (RX_DMA_PAUSE|RX_CHANNEL_PAUSE|RX_DMA_ACTIVE)


// IISMOD : IIS Interface Mode Register
#define IIS_CH2_DATA_DISCARD_MASK               (3 << 20)
#define IIS_CH2_DATA_NO_DISCARD                 (0 << 20)
#define IIS_CH2_DATA_RIGHT_HALFWORD_DISCARD     (1 << 20)
#define IIS_CH2_DATA_LEFT_HALFWORD_DISCARD      (2 << 20)

#define IIS_CH1_DATA_DISCARD_MASK               (3 << 18)
#define IIS_CH1_DATA_NO_DISCARD                 (0 << 18)
#define IIS_CH1_DATA_RIGHT_HALFWORD_DISCARD     (1 << 18)
#define IIS_CH1_DATA_LEFT_HALFWORD_DISCARD      (2 << 18)

#define IIS_DATA_CHANNEL_MASK                   (3 << 16)
#define IIS_SD2_CHANEEL_ENABLE                  (1 << 17)
#define IIS_SD1_CHANEEL_ENABLE                  (1 << 16)

#define BIT_LENGTH_PER_CHANNEL_MASK             (3 << 13)
#define BIT_LENGTH_PER_CHANNEL_16BIT            (0 << 13)
#define BIT_LENGTH_PER_CHANNEL_8BIT             (1 << 13)
#define BIT_LENGTH_PER_CHANNEL_24BIT            (2 << 13)


#define CODEC_CLOCK_SOURCE_MASK                    (1 << 12)
#define CODEC_CLK_USE_INTERNEL_CLK                (0 << 12)
#define CODEC_CLK_USE_EXTERNEL_CLK                (1 << 12)

#define IIS_CLK_MASTER_SLAVE_MASK                (3 << 10)
#define IIS_CLK_MASTER_PCLK_DIV_MODE            (0 << 10)
#define IIS_CLK_MASTER_IISCLK_BYPASS_MODE        (1 << 10)
#define IIS_CLK_SLAVE_MODE                        (2 << 10)

#define TRANSFER_MODE_MASK                        (3 << 8)
#define TRANSFER_MODE_TX_ONLY                    (0 << 8)
#define TRANSFER_MODE_RX_ONLY                    (1 << 8)
#define TRANSFER_MODE_BOTH                        (2 << 8)

#define LR_CH_POL_MASK                            (1 << 7)
#define LR_CH_POL_LOW_FOR_LEFT_HIGH_FOR_RIGHT    (0 << 7)
#define LR_CH_POL_HIGH_FOR_LEFT_LOW_FOR_RIGHT    (1 << 7)

#define SERIAL_DATA_FORMAT_MASK                    (3 << 5)
#define SERIAL_DATA_FORMAT_IIS                    (0 << 5)
#define SERIAL_DATA_FORMAT_MSB_JUSTIFIED        (1 << 5)
#define SERIAL_DATA_FORMAT_LSB_JUSTIFIED        (2 << 5)

#define IIS_CODEC_CLOCK_FREQUENCY_MASK            (3 << 3)
#define IIS_CODEC_CLOCK_FREQUENCY_256FS            (0 << 3)
#define IIS_CODEC_CLOCK_FREQUENCY_512FS            (1 << 3)
#define IIS_CODEC_CLOCK_FREQUENCY_384FS            (2 << 3)
#define IIS_CODEC_CLOCK_FREQUENCY_768FS            (3 << 3)

#define BIT_CLOCK_FREQUENCY_MASK                (3 << 1)
#define BIT_CLOCK_FREQUENCY_32FS                (0 << 1)
#define BIT_CLOCK_FREQUENCY_48FS                (1 << 1)
#define BIT_CLOCK_FREQUENCY_16FS                (2 << 1)
#define BIT_CLOCK_FREQUENCY_24FS                (3 << 1)

// IISFIC : IIS Interface FIFO Control Register
#define TX_FIFO_2_DATA_COUNT_MASK               (0x1F << 24) // [28:24] 5 bits
#define TX_FIFO_2_DATA_COUNT_SHIFT              (24)

#define TX_FIFO_1_DATA_COUNT_MASK               (0x1F << 16) // [20:16] 5 bits
#define TX_FIFO_1_DATA_COUNT_SHIFT              (16)

#define TX_FIFO_FLUSH                            (1 << 15)
#define TX_FIFO_DATA_COUNT_MASK                    (0x1F << 8) // [12:8] 5 bits
#define TX_FIFO_DATA_COUNT_SHIFT                (8)
#define RX_FIFO_FLUSH                            (1 << 7)
#define RX_FIFO_DATA_COUNT_SHIFT                (0)
#define RX_FIFO_DATA_COUNT_MASK                    (0x1F << 0) // [4:0] 5 bits

// IISPSR : IIS Interface Clock Divider Control Register
#define PRESCALER_A_ACTIVE                        (1 << 15)
#define PRESCALER_A_DIVISION_VALUE_SHIFT        (8)
#define PRESCALER_A_DIVISION_VALUE_MASK            (0x3F) // [13:8] 6 bits

// IISTXD : IIS Interface Transmit Data Register

// IISRXD : IIS Interface Receive Data Register

#if __cplusplus
    }
#endif

#endif    // _S3C6410_IIS_INTERFACE_MACRO_H_
