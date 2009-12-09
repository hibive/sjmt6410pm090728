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
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

   s3c6410_button.c   

Abstract:

   Low Level HW Button control Library, this will contorl HW register with External Interrupt and GPIOs

Functions:

    

Notes:

--*/

#include "precomp.h"

static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;

BOOL Button_initialize_register_address(void *pGPIOReg)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN]++%s(0x%08x)\n\r"), _T(__FUNCTION__), pGPIOReg));

    if (pGPIOReg == NULL)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : NULL pointer parameter\n\r"), _T(__FUNCTION__)));
        return FALSE;
    }
    else
    {
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        RETAILMSG(PWR_ZONE_TEMP, (_T("[BTN:INF] g_pGPIOReg    = 0x%08x\n\r"), g_pGPIOReg));
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN]--%s()\n\r"), _T(__FUNCTION__)));

    return TRUE;
}

#ifdef	OMNIBOOK_VER
// Power Button -> GPN[9] : EINT9
#else	//!OMNIBOOK_VER
// In SMDK6410 Eval. Board, Button is mapped to this GPIOs
// Power Button -> GPN[11] : EINT11
// Reset Button -> GPN[9] : EINT9 / ADDR_CF[1]
#endif	OMNIBOOK_VER
void Button_port_initialize(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));
    
    // GPN[9] to EINT9, GPN[11] to EINT11
    SET_GPIO(g_pGPIOReg, GPNCON, 9, GPNCON_EXTINT);
#ifndef	OMNIBOOK_VER
    SET_GPIO(g_pGPIOReg, GPNCON, 11, GPNCON_EXTINT);
#endif	/!OMNIBOOK_VER

#ifdef	OMNIBOOK_VER
	// GPN[9], GPN[11] set Pull-down Enable
	SET_GPIO(g_pGPIOReg, GPNPUD, 9, GPNPUD_PULLDOWN);
#else	//!OMNIBOOK_VER
    // GPN[9], GPN[11] set Pull-up Enable
    SET_GPIO(g_pGPIOReg, GPNPUD, 9, GPNPUD_PULLUP);
    SET_GPIO(g_pGPIOReg, GPNPUD, 11, GPNPUD_PULLUP);
#endif	OMNIBOOK_VER
}

BOOL Button_pwrbtn_set_interrupt_method(EINT_SIGNAL_METHOD eMethod)
{
    BOOL bRet = TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s(%d)\n\r"), _T(__FUNCTION__), eMethod));

    switch(eMethod)
    {
    case EINT_SIGNAL_LOW_LEVEL:
    case EINT_SIGNAL_HIGH_LEVEL:        
    case EINT_SIGNAL_FALL_EDGE:        
    case EINT_SIGNAL_RISE_EDGE:        
    case EINT_SIGNAL_BOTH_EDGE:        
#ifdef	OMNIBOOK_VER
		g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT9)) | (eMethod<<EINT0CON_EINT9);
#else	//!OMNIBOOK_VER
        g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT11)) | (eMethod<<EINT0CON_EINT11);        
#endif	OMNIBOOK_VER
        break;
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

BOOL Button_pwrbtn_set_filter_method(EINT_FILTER_METHOD eMethod, unsigned int uiFilterWidth)
{
    BOOL bRet =TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s(%d, %d)\n\r"), _T(__FUNCTION__), eMethod, uiFilterWidth));

    switch(eMethod)
    {
#ifdef	OMNIBOOK_VER
    case EINT_FILTER_DISABLE:
		g_pGPIOReg->EINT0FLTCON1 &= ~(0x1<<FLTSEL_9);
		break;
	case EINT_FILTER_DELAY:
		g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(0x1<<FLTSEL_9)) | (0x1<<FLTEN_9);
		break;
	case EINT_FILTER_DIGITAL:
		g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(EINT0FILTERCON_MASK<<FLTWIDTH_9))
                                    | ((0x1<<FLTSEL_9) | (0x1<<FLTEN_9)
                                    | (uiFilterWidth & (EINT0FILTER_WIDTH_MASK<<FLTWIDTH_9)));
		break;
#else	//!OMNIBOOK_VER
    case EINT_FILTER_DISABLE:
        g_pGPIOReg->EINT0FLTCON1 &= ~(0x1<<FLTSEL_11);
        break;
    case EINT_FILTER_DELAY:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(0x1<<FLTSEL_11)) | (0x1<<FLTEN_11);
        break;
    case EINT_FILTER_DIGITAL:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(EINT0FILTERCON_MASK<<FLTWIDTH_11))
                                    | ((0x1<<FLTSEL_11) | (0x1<<FLTEN_11)
                                    | (uiFilterWidth & (EINT0FILTER_WIDTH_MASK<<FLTWIDTH_11)));
#endif	OMNIBOOK_VER
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

void Button_pwrbtn_enable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

#ifdef	OMNIBOOK_VER
	g_pGPIOReg->EINT0MASK &= ~(0x1<<9);		// Unmask EINT9
#else	//!OMNIBOOK_VER
    g_pGPIOReg->EINT0MASK &= ~(0x1<<11);    // Unmask EINT11
#endif	OMNIBOOK_VER
}

void Button_pwrbtn_disable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

#ifdef	OMNIBOOK_VER
	g_pGPIOReg->EINT0MASK |= (0x1<<9);		// Mask EINT9
#else	//!OMNIBOOK_VER
    g_pGPIOReg->EINT0MASK |= (0x1<<11);    // Mask EINT11
#endif	OMNIBOOK_VER
}

void Button_pwrbtn_clear_interrupt_pending(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

#ifdef	OMNIBOOK_VER
	g_pGPIOReg->EINT0PEND = (0x1<<9);		// Clear pending EINT9
#else	//!OMNIBOOK_VER
    g_pGPIOReg->EINT0PEND = (0x1<<11);        // Clear pending EINT11
#endif	OMNIBOOK_VER
}

BOOL Button_pwrbtn_is_pushed(void)
{
    RETAILMSG(PWR_ZONE_ENTER,(_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

#ifdef	OMNIBOOK_VER
	if (!(g_pGPIOReg->GPNDAT & (0x1<<9)))	// We can read GPDAT pin level when configured as EINT
#else	//!OMNIBOOK_VER
    if (g_pGPIOReg->GPNDAT & (0x1<<11))        // We can read GPDAT pin level when configured as EINT
#endif	OMNIBOOK_VER
    {
        return FALSE;    // Low Active Switch (Pull-up switch)
    }
    else
    {
        return TRUE;
    }
}

BOOL Button_rstbtn_set_interrupt_method(EINT_SIGNAL_METHOD eMethod)
{
    BOOL bRet =TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s(%d)\n\r"), _T(__FUNCTION__), eMethod));

#ifndef	OMNIBOOK_VER
    switch(eMethod)
    {
    case EINT_SIGNAL_LOW_LEVEL:
    case EINT_SIGNAL_HIGH_LEVEL:
    case EINT_SIGNAL_FALL_EDGE:
    case EINT_SIGNAL_RISE_EDGE:
    case EINT_SIGNAL_BOTH_EDGE:
        g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT9)) | (eMethod<<EINT0CON_EINT9);
        break;
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }
#endif	//!OMNIBOOK_VER

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

BOOL Button_rstbtn_set_filter_method(EINT_FILTER_METHOD eMethod, unsigned int uiFilterWidth)
{
    BOOL bRet =TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s(%d, %d)\n\r"), _T(__FUNCTION__), eMethod, uiFilterWidth));

#ifndef	OMNIBOOK_VER
    switch(eMethod)
    {
    case EINT_FILTER_DISABLE:
        g_pGPIOReg->EINT0FLTCON1 &= ~(0x1<<FLTSEL_9);
        break;
    case EINT_FILTER_DELAY:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(0x1<<FLTSEL_9)) | (0x1<<FLTEN_9);
        break;
    case EINT_FILTER_DIGITAL:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(EINT0FILTERCON_MASK<<FLTWIDTH_11))
                                    | ((0x1<<FLTSEL_9) | (0x1<<FLTEN_9)
                                    | (uiFilterWidth & (EINT0FILTER_WIDTH_MASK<<FLTWIDTH_9)));
        break;
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }
#endif	//!OMNIBOOK_VER

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

void Button_rstbtn_enable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

#ifndef	OMNIBOOK_VER
    g_pGPIOReg->EINT0MASK &= ~(0x1<<9);    // Unmask EINT9
#endif	//!OMNIBOOK_VER
}

void Button_rstbtn_disable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

#ifndef	OMNIBOOK_VER
    g_pGPIOReg->EINT0MASK |= (0x1<<9);        // Mask EINT9
#endif	//!OMNIBOOK_VER
}

void Button_rstbtn_clear_interrupt_pending(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

#ifndef	OMNIBOOK_VER
    g_pGPIOReg->EINT0PEND = (0x1<<9);        // Clear pending EINT9
#endif	//!OMNIBOOK_VER
}

BOOL Button_rstbtn_is_pushed(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

#ifdef	OMNIBOOK_VER
	if (1)
#else	//!OMNIBOOK_VER
    if (GET_GPIO(g_pGPIOReg, GPNDAT, 9))        // We can read GPDAT pin level when configured as EINT
#endif	OMNIBOOK_VER
    {
        return FALSE;    // Low Active Switch (Pull-up switch)
    }
    else
    {
        return TRUE;
    }
}

