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

   PowerButton.c   Power Controller Driver

Abstract:

   Stream interface of Power Button Driver (MDD).
   In SMDK6410, Powerbutton is mapped to EINT11, SW9 at default

Functions:



Notes:

--*/

#include "precomp.h"

#define SLEEP_AGING_TEST    (FALSE)     //< OEMs can enable this definition to do aging test for sleep/wakeup

static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static DWORD g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
static DWORD g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;
static HANDLE g_hEventPowerBtn = NULL;
static HANDLE g_hEventResetBtn = NULL;
static HANDLE g_hThreadPowerBtn = NULL;
static HANDLE g_hThreadResetBtn = NULL;
static BOOL g_bExitThread = FALSE;

DBGPARAM dpCurSettings =                                \
{                                                       \
    TEXT(__MODULE__),                                   \
    {                                                   \
        TEXT("Errors"),                 /* 0  */        \
        TEXT("Warnings"),               /* 1  */        \
        TEXT("Performance"),            /* 2  */        \
        TEXT("Temporary tests"),        /* 3  */        \
        TEXT("Enter,Exit"),             /* 4  */        \
        TEXT("Initialize"),             /* 5  */        \
        TEXT("Power Up"),               /* 6  */        \
        TEXT("Power Down"),             /* 7  */        \
        TEXT("Event Hook"),             /* 8  */        \
    },                                                  \
    (PWRBTN_ZONES)                               \
};

INT WINAPI PowerButtonThread(void)
{
    DWORD nBtnCount = 0;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] ++%s()\r\n"), _T(__FUNCTION__)));

    while(!g_bExitThread)
    {
        WaitForSingleObject(g_hEventPowerBtn, INFINITE);

        if(g_bExitThread)
        {
            break;
        }

        Button_pwrbtn_disable_interrupt();              // Mask EINT
        Button_pwrbtn_clear_interrupt_pending();        // Clear Interrupt Pending

        InterruptDone(g_dwSysIntrPowerBtn);

#if !(SLEEP_AGING_TEST)
        // Normal Button Push/Release Operation
        // Enter in when the power button is pushed
        // Hold in loop
        // Loop out when the power button is released
#ifdef	EBOOK2_VER
#define	TIMEOUT_POWEROFF	250
#define	TIMEOUT_SLEEP		18
		for (nBtnCount=0; nBtnCount<TIMEOUT_POWEROFF; nBtnCount++)
		{
			if (FALSE == Button_pwrbtn_is_pushed())
				break;
			Sleep(10);
		}
#else	EBOOK2_VER
        while(Button_pwrbtn_is_pushed())
        {
            // Wait for Button Released...
            Sleep(10);
        }
#endif	EBOOK2_VER
#endif

        nBtnCount++;
        RETAILMSG(PWR_ZONE_EVENT_HOOK, (_T("[PWR] Power Button Event [%d]\r\n"), nBtnCount));

#ifdef	EBOOK2_VER
		if (TIMEOUT_POWEROFF <= nBtnCount)	// GPC[3] - PWRHOLD(3)
		{
			LPCTSTR lpszPathName = _T("\\Windows\\EBook2Command.exe");
			PROCESS_INFORMATION pi;

			ZeroMemory(&pi,sizeof(pi));
			if (CreateProcess(lpszPathName,
							  _T("SHUTDOWN"),	// pszCmdLine
							  NULL,	// psaProcess
							  NULL,	// psaThread
							  FALSE,// fInheritHandle
							  0,	// fdwCreate
							  NULL,	// pvEnvironment
							  NULL,	// pszCurDir
							  NULL,	// psiStartInfo
							  &pi))	// pProcInfo
			{
				WaitForSingleObject(pi.hThread, 3000);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
			KernelIoControl(IOCTL_HAL_EBOOK2_SHUTDOWN, NULL, 0, NULL, 0, NULL);
		}
		else if (TIMEOUT_SLEEP < nBtnCount)
		{
			LPCTSTR lpszPathName = _T("\\Windows\\EBook2Command.exe");
			PROCESS_INFORMATION pi;

			ZeroMemory(&pi,sizeof(pi));
			if (CreateProcess(lpszPathName,
							  _T("SLEEP"),	// pszCmdLine
							  NULL,	// psaProcess
							  NULL,	// psaThread
							  FALSE,// fInheritHandle
							  0,	// fdwCreate
							  NULL,	// pvEnvironment
							  NULL,	// pszCurDir
							  NULL,	// psiStartInfo
							  &pi))	// pProcInfo
			{
				WaitForSingleObject(pi.hThread, 3000);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}

			SetSystemPowerState(NULL, POWER_STATE_SUSPEND, POWER_FORCE);

			ZeroMemory(&pi,sizeof(pi));
			if (CreateProcess(lpszPathName,
							  _T("WAKEUP"),	// pszCmdLine
							  NULL,	// psaProcess
							  NULL,	// psaThread
							  FALSE,// fInheritHandle
							  0,	// fdwCreate
							  NULL,	// pvEnvironment
							  NULL,	// pszCurDir
							  NULL,	// psiStartInfo
							  &pi))	// pProcInfo
			{
				WaitForSingleObject(pi.hThread, 3000);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
		}
#else	EBOOK2_VER
        // In the Windows Mobile, "PowerPolicyNotify(PPN_POWERBUTTONPRESSED, 0);" can be used
        SetSystemPowerState(NULL, POWER_STATE_SUSPEND, POWER_FORCE);
#endif	EBOOK2_VER

        Button_pwrbtn_enable_interrupt();            // UnMask EINT
#if (SLEEP_AGING_TEST)
        // To do Sleep/Wakeup aging test 
        SetEvent(g_hEventPowerBtn);
#endif
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] --%s()\r\n"), _T(__FUNCTION__)));

    return 0;
}

#ifndef	EBOOK2_VER
INT WINAPI ResetButtonThread(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] ++%s()\r\n"), _T(__FUNCTION__)));

    while(!g_bExitThread)
    {
        WaitForSingleObject(g_hEventResetBtn, INFINITE);

        if(g_bExitThread)
        {
            break;
        }

        Button_rstbtn_disable_interrupt();              // Mask EINT
        Button_rstbtn_clear_interrupt_pending();        // Clear Interrupt Pending

        InterruptDone(g_dwSysIntrResetBtn);

        RETAILMSG(PWR_ZONE_EVENT_HOOK, (_T("[PWR] Reset Button Event\r\n")));

        SetSystemPowerState(NULL, POWER_STATE_RESET, POWER_FORCE);
        //KernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL);

        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] Soft Reset Failed\r\n")));

        Button_rstbtn_enable_interrupt();                // UnMask EINT
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] --%s()\r\n"), _T(__FUNCTION__)));

    return 0;
}
#endif	EBOOK2_VER

static BOOL
AllocResources(void)
{
    DWORD dwIRQ;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s()\r\n"), _T(__FUNCTION__)));

    //------------------
    // GPIO Controller SFR
    //------------------
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    g_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (g_pGPIOReg == NULL)
    {
        RETAILMSG(PWR_ZONE_ERROR,(_T("[PWR:ERR] %s() : pGPIOReg MmMapIoSpace() Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

    //--------------------
    // Power Button Interrupt
    //--------------------
#ifdef	EBOOK2_VER
	dwIRQ = IRQ_EINT9;
#else	EBOOK2_VER
    dwIRQ = IRQ_EINT11;
#endif	EBOOK2_VER
    g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
    g_hEventPowerBtn = NULL;

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrPowerBtn, sizeof(DWORD), NULL))
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : IOCTL_HAL_REQUEST_SYSINTR Power Button Failed \n\r"), _T(__FUNCTION__)));
        g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
        return FALSE;
    }

    g_hEventPowerBtn = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == g_hEventPowerBtn)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateEvent() Power Button Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

    if (!(InterruptInitialize(g_dwSysIntrPowerBtn, g_hEventPowerBtn, 0, 0)))
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : InterruptInitialize() Power Button Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

#ifndef	EBOOK2_VER
    //--------------------
    // Reset Button Interrupt
    //--------------------
    dwIRQ = IRQ_EINT9;
    g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;
    g_hEventResetBtn = NULL;

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrResetBtn, sizeof(DWORD), NULL))
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : IOCTL_HAL_REQUEST_SYSINTR Reset Button Failed \n\r"), _T(__FUNCTION__)));
        g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;
        return FALSE;
    }

    g_hEventResetBtn = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == g_hEventResetBtn)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateEvent() Reset Button Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

    if (!(InterruptInitialize(g_dwSysIntrResetBtn, g_hEventResetBtn, 0, 0)))
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : InterruptInitialize() Reset Button Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }
#endif	EBOOK2_VER

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));

    return TRUE;
}

static void
ReleaseResources(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s()\r\n"), _T(__FUNCTION__)));

    if (g_pGPIOReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
        g_pGPIOReg = NULL;
    }

    if (g_dwSysIntrPowerBtn != SYSINTR_UNDEFINED)
    {
        InterruptDisable(g_dwSysIntrPowerBtn);
    }

    if (g_hEventPowerBtn != NULL)
    {
        CloseHandle(g_hEventPowerBtn);
    }

    if (g_dwSysIntrPowerBtn != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrPowerBtn, sizeof(DWORD), NULL, 0, NULL);
    }

    if (g_dwSysIntrResetBtn != SYSINTR_UNDEFINED)
    {
        InterruptDisable(g_dwSysIntrResetBtn);
    }

    if (g_hEventResetBtn != NULL)
    {
        CloseHandle(g_hEventResetBtn);
    }

    if (g_dwSysIntrResetBtn != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrResetBtn, sizeof(DWORD), NULL, 0, NULL);
    }

    g_pGPIOReg = NULL;

    g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
    g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;

    g_hEventPowerBtn = NULL;
    g_hEventResetBtn = NULL;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));
}

static void InitInterrupt(void)
{
    // Interrupt Disable and Clear Pending
    Button_pwrbtn_disable_interrupt();
    Button_rstbtn_disable_interrupt();

    // Initialize Port as External Interrupt
    Button_port_initialize();

    // Interrupt Siganl Method and Filtering
#ifdef	EBOOK2_VER
	Button_pwrbtn_set_interrupt_method(EINT_SIGNAL_RISE_EDGE);
#elif	EBOOK2_VER
    Button_pwrbtn_set_interrupt_method(EINT_SIGNAL_FALL_EDGE);
#endif	EBOOK2_VER
    Button_pwrbtn_set_filter_method(EINT_FILTER_DELAY, 0);
    Button_rstbtn_set_interrupt_method(EINT_SIGNAL_FALL_EDGE);
    Button_rstbtn_set_filter_method(EINT_FILTER_DELAY, 0);

    // Clear Interrupt Pending
    Button_pwrbtn_clear_interrupt_pending();
    Button_rstbtn_clear_interrupt_pending();

    // Enable Interrupt
    Button_pwrbtn_enable_interrupt();
    Button_rstbtn_enable_interrupt();
}


BOOL
DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DEBUGREGISTER(hinstDll);
        DEBUGMSG(PWR_ZONE_INIT, (_T("[PWR] %s() : Process Attach\r\n"), _T(__FUNCTION__)));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DEBUGMSG(PWR_ZONE_INIT, (_T("[PWR] %s() : Process Detach\r\n"), _T(__FUNCTION__)));
    }

    return TRUE;
}

// After Wake Up, This code will be called in single-threaded stage.
// Button Driver Monitor thread should get interrupt from external interrupt source in multi-threaded stage.
// then set System Power State to Resume in IST
// So, there are no need to disable interrupt for Power Button, and to clear interrupt pending bit in ISR handler.
BOOL
PWR_PowerUp(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));
    Button_rstbtn_enable_interrupt();
#ifdef	EBOOK2_VER
	Button_pwrbtn_enable_interrupt();	// UnMask EINT
#endif	EBOOK2_VER
    return TRUE;
}

BOOL
PWR_PowerDown(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    // Interrupt Disable and Clear Pending
    Button_pwrbtn_disable_interrupt();
    Button_pwrbtn_clear_interrupt_pending();
    Button_rstbtn_disable_interrupt();
    Button_rstbtn_clear_interrupt_pending();

    return TRUE;
}

BOOL PWR_Deinit(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    g_bExitThread = TRUE;

    if (g_hThreadPowerBtn)        // Make Sure if thread is exist
    {
        Button_pwrbtn_disable_interrupt();
        Button_pwrbtn_clear_interrupt_pending();

        // Signal Thread to Finish
        SetEvent(g_hEventPowerBtn);
        // Wait for Thread to Finish
        WaitForSingleObject(g_hThreadPowerBtn, INFINITE);
        CloseHandle(g_hThreadPowerBtn);
        g_hThreadPowerBtn = NULL;
    }

    if (g_hThreadResetBtn)        // Make Sure if thread is exist
    {
        Button_rstbtn_disable_interrupt();
        Button_rstbtn_clear_interrupt_pending();

        // Signal Thread to Finish
        SetEvent(g_hEventResetBtn);
        // Wait for Thread to Finish
        WaitForSingleObject(g_hThreadResetBtn, INFINITE);
        CloseHandle(g_hThreadResetBtn);
        g_hThreadResetBtn = NULL;
    }

    ReleaseResources();

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));

    return TRUE;
}

DWORD
PWR_Init(DWORD dwContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s(0x%08x)\r\n"), _T(__FUNCTION__), dwContext));

    if (AllocResources() == FALSE)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : AllocResources() Failed \n\r"), _T(__FUNCTION__)));

        goto CleanUp;
    }

    Button_initialize_register_address((void *)g_pGPIOReg);

    // Enable Interrupt
    InitInterrupt();
    
    // Create Power Button Thread
    g_hThreadPowerBtn = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PowerButtonThread, NULL, 0, NULL);
    if (g_hThreadPowerBtn == NULL )
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateThread() Power Button Failed \n\r"), _T(__FUNCTION__)));
        goto CleanUp;
    }

#ifndef	EBOOK2_VER
    // Create Reset Button Thread
    g_hThreadResetBtn = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ResetButtonThread, NULL, 0, NULL);
    if (g_hThreadResetBtn == NULL )
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateThread() Reset Button Failed \n\r"), _T(__FUNCTION__)));
        goto CleanUp;
    }
#endif	EBOOK2_VER
    
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));

    return TRUE;

CleanUp:

    RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] --%s() : Failed\r\n"), _T(__FUNCTION__)));

    PWR_Deinit(0);

    return FALSE;
}

DWORD
PWR_Open(DWORD pContext, DWORD dwAccess, DWORD dwShareMode)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, dwAccess, dwShareMode));

    return TRUE;
}

BOOL
PWR_Close(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    return TRUE;
}

DWORD
PWR_Read (DWORD pContext,  LPVOID pBuf, DWORD Len)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, pBuf, Len));

    return (0);    // End of File
}

DWORD
PWR_Write(DWORD pContext, LPCVOID pBuf, DWORD Len)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, pBuf, Len));

    return (0);    // Number of Byte
}

DWORD
PWR_Seek (DWORD pContext, long pos, DWORD type)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, pos, type));

    return (DWORD)-1;    // Failure
}

BOOL
PWR_IOControl(DWORD pContext, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, dwCode));

    return FALSE;    // Failure
}

