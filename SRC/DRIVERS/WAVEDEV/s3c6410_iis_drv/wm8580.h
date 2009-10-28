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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#ifndef _WM8580_H_
#define _WM8580_H_


typedef enum
{
    WM8580_PLLA1                        = 0x00,
    WM8580_PLLA2                        = 0x01,
    WM8580_PLLA3                        = 0x02,
    WM8580_PLLA4                        = 0x03,
    WM8580_PLLB1                        = 0x04,
    WM8580_PLLB2                        = 0x05,
    WM8580_PLLB3                        = 0x06,
    WM8580_PLLB4                        = 0x07,
    WM8580_CLKSEL                       = 0x08,
    WM8580_PAIF1                        = 0x09,
    WM8580_PAIF2                        = 0x0A,
    WM8580_SAIF1                        = 0x0B,
    WM8580_PAIF3                        = 0x0C,
    WM8580_PAIF4                        = 0x0D,
    WM8580_SAIF2                        = 0x0E,
    WM8580_DAC_CONTROL1                 = 0x0F,
    WM8580_DAC_CONTROL2                 = 0x10,
    WM8580_DAC_CONTROL3                 = 0x11,
    WM8580_DAC_CONTROL4                 = 0x12,
    WM8580_DAC_CONTROL5                 = 0x13,
    WM8580_DIGITAL_ATTENUATION_DACL1    = 0x14,
    WM8580_DIGITAL_ATTENUATION_DACR1    = 0x15,
    WM8580_DIGITAL_ATTENUATION_DACL2    = 0x16,
    WM8580_DIGITAL_ATTENUATION_DACR2    = 0x17,
    WM8580_DIGITAL_ATTENUATION_DACL3    = 0x18,
    WM8580_DIGITAL_ATTENUATION_DACR3    = 0x19,
    WM8580_MASTER_DIGITAL_ATTENUATION   = 0x1C,
    WM8580_ADC_CONTROL1                 = 0x1D,
    WM8580_SPDTXCHAN0                   = 0x1E,
    WM8580_SPDTXCHAN1                   = 0x1F,
    WM8580_SPDTXCHAN2                   = 0x20,
    WM8580_SPDTXCHAN3                   = 0x21,
    WM8580_SPDTXCHAN4                   = 0x22,
    WM8580_SPDTXCHAN5                   = 0x23,
    WM8580_SPDMODE                      = 0x24,
    WM8580_INTMASK                      = 0x25,
    WM8580_GPO1                         = 0x26,
    WM8580_GPO2                         = 0x27,
    WM8580_GPO3                         = 0x28,
    WM8580_GPO4                         = 0x29,
    WM8580_GPO5                         = 0x2A,
    WM8580_INTSTAT                      = 0x2B,
    WM8580_SPDRXCHAN1                   = 0x2C,
    WM8580_SPDRXCHAN2                   = 0x2D,
    WM8580_SPDRXCHAN3                   = 0x2E,
    WM8580_SPDRXCHAN4                   = 0x2F,
    WM8580_SPDRXCHAN5                   = 0x30,
    WM8580_SPDSTAT                      = 0x31,
    WM8580_PWRDN1                       = 0x32,
    WM8580_PWRDN2                       = 0x33,
    WM8580_READBACK                     = 0x34,
    WM8580_RESET                        = 0x35,
}  WM8580_RegAddr;

/* Powerdown Register 1 (register 32h) */
#define WM8580_PWRDN1_PWDN_ALL_ACTIVE      0<<0
#define WM8580_PWRDN1_PWDN_ALL_MUTE        1<<0
#define WM8580_PWRDN1_ADCPD_ENABLE         0<<1
#define WM8580_PWRDN1_ADCPD_DISABLE        1<<1
#define WM8580_PWRDN1_DACPD_0_ENABLE       0<<2
#define WM8580_PWRDN1_DACPD_0_DISABLE      1<<2
#define WM8580_PWRDN1_DACPD_1_ENABLE       0<<3
#define WM8580_PWRDN1_DACPD_1_DISABLE      1<<3
#define WM8580_PWRDN1_DACPD_2_ENABLE       0<<4
#define WM8580_PWRDN1_DACPD_2_DISABLE      1<<4
#define WM8580_PWRDN1_DACPD_ALL_ENABLE     0<<6
#define WM8580_PWRDN1_DACPD_ALL_DISABLE    1<<6

#define WM8580_ALL_POWER_ON                0x000

// WM8580 Codec-chip Initialization Value 
unsigned int WM8580_Codec_Init_Table[][2] =
{
#if	(EBOOK2_VER == 3)
//#define	USE_HEADPHONE
#define	USE_SPEAKER
#ifdef	USE_HEADPHONE
	{ 0x1c, 0x094 },	// Enable POBCTRL, SOFT_ST and BUFDCOPEN
	{ 0x1d, 0x040 },	// Enable DISOP
	// Delay (400mS) to remove any residual charge on HP output
	{ 0x1a, 0x061 },	// Enable LOUT1 and ROUT1 SPKL , SPKR disable
	{ 0x1d, 0x000 },	// Enable DISOP
	{ 0x19, 0x0c0 },	// Enable VMID SEL = 2x50K Ohm Dividere
	// Delay (50mSeconds) to allow HP amps to settle
	{ 0x1c, 0x001 },	// Enable POBCTRL, SOFT_ST and BUFDCOPEN
	{ 0x1a, 0x1d1 },	// Enable LOUT1 and ROUT1
	{ 0x17, 0x1d1 },	// Slow Clock Enable
	{ 0x2f, 0x00c },	// Enable left and right output mixers
	{ 0x22, 0x150 },	// Enable Left DAC to left mixer (LINPUT3 to Output Mixer)
	{ 0x25, 0x150 },	// Enable Right DAC to right mixer
	//General purpose input /output
	{ 0x09, 0x040 },	// GPIO pin setting
	{ 0x30, 0x002 },	// ADCLRC /GPIO set
	//HP jack dectect cap-less mode
	{ 0x18, 0x060 },	// HPDETECT High =Headphone, switch enable
	{ 0x1b, 0x00d },	// HPDETECT HIGH = Speaker /ALC Sample Rate 8K
	{ 0x0a, 0x0fe },	// LDACVOL
	{ 0x0b, 0x0fe },	// LDACVOL
	{ 0x04, 0x000 },	// DACDIV, CLKSEL
	{ 0x07, 0x000 },	// 
	{ 0x02, 0x17f },	// LOUT1VOL (HP) = -20dB
	{ 0x03, 0x17f },	// ROUT1VOL (HP) = -20dB, Enable OUT1VU, load volume settings to both left and right channels
	{ 0x28, 0x17f },	// LOUT2VOL (HP) = -20dB
	{ 0x29, 0x17f },	// ROUT2VOL (HP) = -20dB, Enable OUT2VU, load volume settings to both left and right channels
	{ 0x33, 0x0cc },	// Left and Right Speakers Enabled
	{ 0x07, 0x002 },	// 
	{ 0x31, 0x0f7 },	// Left and Right Speakers Enabled
	{ 0x05, 0x004 },	// DAC Digital Soft Mute = Unmute (Delay from R25 = 080 to unmute >250mS)
#endif	USE_HEADPHONE
#ifdef	USE_SPEAKER
	{ 0x1c, 0x094 },	// Enable POBCTRL, SOFT_ST and BUFDCOPEN
	{ 0x1d, 0x040 },	// Enable DISOP
	// Delay (400mS) to remove any residual charge on HP output
	{ 0x1a, 0x07f },	// Enable LOUT1 and ROUT1
	{ 0x1d, 0x000 },	// Enable DISOP
	{ 0x19, 0x0c0 },	// Enable VMID SEL = 2x50K Ohm Dividere
	// Delay (50mSeconds) to allow HP amps to settle
	{ 0x1c, 0x001 },	// Enable POBCTRL, SOFT_ST and BUFDCOPEN
	{ 0x1a, 0x1f8 },	// Enable LOUT1 and ROUT1
	{ 0x17, 0x1d1 },	// 
	{ 0x2f, 0x00c },	// Enable left and right output mixers
	{ 0x22, 0x150 },	// Enable Left DAC to left mixer (LINPUT3 to Output Mixer)
	{ 0x25, 0x150 },	// Enable Right DAC to right mixer
	{ 0x30, 0x00a },	// JD2 used for Jack Detect Input
	{ 0x18, 0x040 },	// HPDETECT HIGH = Speaker
	{ 0x1b, 0x008 },	// HPDETECT HIGH = Speaker
	{ 0x04, 0x000 },	// DACDIV, CLKSEL
	{ 0x07, 0x000 },	// 
	{ 0x02, 0x179 },	// LOUT1VOL (HP) = -20dB
	{ 0x03, 0x179 },	// ROUT1VOL (HP) = -20dB, Enable OUT1VU, load volume settings to both left and right channels
	{ 0x28, 0x179 },	// LOUT2VOL (HP) = -20dB
	{ 0x29, 0x179 },	// ROUT2VOL (HP) = -20dB, Enable OUT2VU, load volume settings to both left and right channels
	{ 0x33, 0x0cc },	// Left and Right Speakers Enabled
	{ 0x07, 0x002 },	// 
	{ 0x31, 0x0f7 },	// Left and Right Speakers Enabled
	{ 0x05, 0x004 },	// DAC Digital Soft Mute = Unmute (Delay from R25 = 080 to unmute >250mS)
#endif	USE_SPEAKER
#elif	(EBOOK2_VER == 2)
	{ 0x01, 0x01D },	// Power Management 1
	{ 0x02, 0x195 },	// Power Management 2
	{ 0x03, 0x06F },	// Power Management 3
	{ 0x04, 0x010 },	// Audio Interface
	{ 0x06, 0x000 },	// Clock Generator Control
	{ 0x07, 0x001 },	// Additional Control
	{ 0x09, 0x1C0 },	// Jack Detect Control
	{ 0x0B, 0x1FF },	// Left DAC Digital Volume
	{ 0x0C, 0x1FF },	// Right DAC Digital Volume
	{ 0x0D, 0x021 },	// Jack Detect Control
	{ 0x0F, 0x1FF },	// Left ADC Digital Volume
	//{ 0x10, 0x1FF },	// Right ADC Digital Volume
	{ 0x2B, 0x010 },	// Beep Control
	{ 0x2C, 0x002 },	// Input Control
	{ 0x2D, 0x130 },	// Left INP PGA Gain Control
	//{ 0x2E, 0x110 },	// Right INP PGA Gain Control
	{ 0x2F, 0x100 },	// ADC Boost Control
	//{ 0x30, 0x000 },	// Right PGA Boost Control
	{ 0x32, 0x001 },	// Left Mixer Control
	{ 0x33, 0x001 },	// Right Mixer Control
	{ 0x34, 0x139 },	// LOUT1(HP) Volume Control
	{ 0x35, 0x139 },	// ROUT1(HP) Volume Control
	{ 0x36, 0x13F },	// LOUT2(SPK) Volume Control
	{ 0x37, 0x13F },	// ROUT2(SPK) Volume Control
	{ 0x38, 0x040 },	// OUT3 Mixer Control
	{ 0x39, 0x040 },	// OUT3(MONO) Mixer Control
#else	EBOOK2_VER
#if 1
    { 0x35, 0x000 },    // R53
    { 0x32, 0x000 },    // R50
    { 0x1A, 0x0FF },    // R26
    { 0x1B, 0x0FF },    // R27
    { 0x1C, 0x1F0 },    // R28
    { 0x08, 0x01C },    // R08
    { 0x0C, 0x182 },    // R12
    { 0x0D, 0x082 },    // R13
#endif
#endif	EBOOK2_VER
};


#endif // _WM8580_H_