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
//  File:  intr.h
//
//  This file contains SMDK6410 board specific interrupt code. 
//
#include <bsp.h>

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrInit
//
BOOL BSPIntrInit()
{
    OALMSG(OAL_INTR&&OAL_FUNC, (L"[OAL] ++BSPIntrInit()\r\n"));

    //---------------------
    // Static SYSINTR mapping
    //---------------------

    // Add static mapping for Built-In OHCI
    OALIntrStaticTranslate(SYSINTR_OHCI, IRQ_UHOST);        // for USB Host 1.1

    OALMSG(OAL_INTR&&OAL_FUNC, (L"[OAL] --BSPIntrInit()\r\n"));

    return TRUE;
}

//------------------------------------------------------------------------------

BOOL BSPIntrRequestIrqs(DEVICE_LOCATION *pDevLoc, UINT32 *pCount, UINT32 *pIrqs)
{
    BOOL bRet = FALSE;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x, 0x%08x)\r\n",
                            pDevLoc, pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
                            pDevLoc->Pin, pCount, pIrqs));
    OALMSG(TRUE, (L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x, 0x%08x)\r\n",
                            pDevLoc, pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
                            pDevLoc->Pin, pCount, pIrqs));
    

    if (pIrqs == NULL
        || pCount == NULL
        || *pCount < 1)
    {
        goto cleanUp;
    }
    else
    {
        switch (pDevLoc->IfcType)
        {
        case Internal:
            switch ((ULONG)pDevLoc->LogicalLoc)
            {
                case BSP_BASE_REG_PA_CS8900A_IOBASE:
                    pIrqs[0] = IRQ_EINT10;
                    *pCount = 1;
                    bRet = TRUE;
                    break;
                    
                case S3C6410_BASE_REG_PA_USBOTG_LINK:
                    pIrqs[0] = IRQ_OTG;
                    *pCount = 1;
                    bRet = TRUE;
                    break;                    
            }
            break;
        }
    }

cleanUp:

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-BSPIntrRequestIrq(rc = %d)\r\n", bRet));
    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrEnableIrq
//
//  This function is called from OALIntrEnableIrq to enable interrupt on
//  secondary interrupt controller.
//
UINT32 BSPIntrEnableIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrEnableIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrEnableIrq(irq = %d)\r\n", irq));
    return irq;
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrDisableIrq
//
//  This function is called from OALIntrDisableIrq to disable interrupt on
//  secondary interrupt controller.
//
UINT32 BSPIntrDisableIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrDisableIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrDisableIrq(irq = %d\r\n", irq));
    return irq;
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrDoneIrq
//
//  This function is called from OALIntrDoneIrq to finish interrupt on
//  secondary interrupt controller.
//
UINT32 BSPIntrDoneIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrDoneIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrDoneIrq(irq = %d)\r\n", irq));
    return irq;
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrActiveIrq
//
//  This function is called from interrupt handler to give BSP chance to
//  translate IRQ in case of secondary interrupt controller.
//
UINT32 BSPIntrActiveIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrActiveIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrActiveIrq(%d)\r\n", irq));
    return irq;
}

//------------------------------------------------------------------------------

