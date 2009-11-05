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
	{  9, 0x040 },	// GPIO Pin
	{ 23, 0x1C1 },	// Enable DMONOMIX, Thermal shutdown enabled (R23=0x1C0 : Disable DMONOMIX)
	{ 51, 0x08D },	// DCGAIN=1.27x and ACGAIN=1.8 with SPKVDD=4.2V
	//{ 51, 0x09D },	// DCGAIN = 1.52x (+3.6dB) and ACGAIN = 1.8x with SPKVDD=5V
	//{ 51, 0x084 },	// DCGAIN=1.0x and ACGAIN=1.67 with SPKVDD=3.6V)
	{  7, 0x002 },	// I2S, 16bit, Slave mode ( For Master mode : R7=0x042)
	{ 24, 0x060 },	// HPSWEN Enable, HPSWPOL High
	{  6, 0x00E },	// DACSMM, DACMR, DACSLOPE

	{ 34, 0x100 },	// Left DAC to left output mixer enabled (LD2LO), 0dB
	{ 37, 0x100 },	// Right DAC to right output mixer enabled (RD2RO), 0dB
	{  2, 0x170 },	// LHP Vol = 0dB, volume update enabled
	{  3, 0x170 },	// RHP Vol = 0dB, volume update enabled
	{ 40, 0x179 },	// LSPK Vol = 0dB, volume update enabled
	{ 41, 0x179 },	// RSPK Vol = 0dB, volume update enabled

	{ 32, 0x128 },	// LINPUT1 to PGA(LMN1), Connect left input PGA to left input boost(LMIC2B)
	{  0, 0x13F },	// Unmute left input PGA(LINMUTE), Left Input PGA Vol 0dB, Volume Update
	{ 21, 0x1C3 },	// Left ADC Vol 0dB, Volume Update
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
