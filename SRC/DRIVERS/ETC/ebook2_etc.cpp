
#include <bsp.h>
#include "ebook2_etc.h"
#include <iic.h>


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)

#define	PMIC_ADDR	0xCC


volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;
volatile BSP_ARGS *g_pBspArgs = NULL;
static HANDLE m_MutexEtc = NULL;

static HANDLE g_hFileI2C = INVALID_HANDLE_VALUE;

#if	(EBOOK2_VER == 2)
static DWORD g_dwSysIntrKeyHold = SYSINTR_UNDEFINED;
static HANDLE g_hEventKeyHold = NULL;
static HANDLE g_hThreadKeyHold = NULL;
static BOOL g_bExitThreadKeyHold = FALSE;
#endif	(EBOOK2_VER == 2)


DWORD ETC_Init(DWORD dwContext);
BOOL ETC_Deinit(DWORD InitHandle);
DWORD ETC_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode);
BOOL ETC_Close(DWORD OpenHandle);
BOOL ETC_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
    PBYTE pInBuf, DWORD nInBufSize,
    PBYTE pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned);
void ETC_PowerUp(DWORD InitHandle);
void ETC_PowerDown(DWORD InitHandle);



static BOOL i2c_Initialize(void)
{
	UINT32 IICClock, uiIICDelay, bytes;
	BOOL bRet;

	g_hFileI2C = CreateFile(_T("IIC0:"),
						GENERIC_READ|GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						NULL, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == g_hFileI2C)
	{
		MYERR((_T("[ETC] g_hFileI2C = CreateFile(%d)\r\n"), GetLastError()));
		return FALSE;
	}

	IICClock = 10000;
	bRet = DeviceIoControl(g_hFileI2C, IOCTL_IIC_SET_CLOCK,
		&IICClock, sizeof(UINT32), NULL, 0, (LPDWORD)&bytes, NULL);
	if (FALSE == bRet)
	{
		MYERR((_T("[ETC] bRet = DeviceIoControl(IOCTL_IIC_SET_CLOCK)\r\n")));
		return FALSE;
	}

	uiIICDelay = Clk_0;
	bRet = DeviceIoControl(g_hFileI2C, IOCTL_IIC_SET_DELAY,
		&uiIICDelay, sizeof(UINT32), NULL, 0, (LPDWORD)&bytes, NULL);
	if (FALSE == bRet)
	{
		MYERR((_T("[ETC] bRet = DeviceIoControl(IOCTL_IIC_SET_DELAY)\r\n")));
		return FALSE;
	}

	return TRUE;
}
static void i2c_Deinitialize(void)
{
	if (INVALID_HANDLE_VALUE != g_hFileI2C)
	{
		CloseHandle(g_hFileI2C);
		g_hFileI2C = INVALID_HANDLE_VALUE;
	}
}
static UCHAR i2c_ReadRegister(UCHAR Reg)
{
	if (INVALID_HANDLE_VALUE == g_hFileI2C)
	{
		MYERR((_T("[ETC] INVALID_HANDLE_VALUE == g_hFileI2C\r\n")));
		return 0;
	}

#if	1
	if (0x00 == Reg)
		return g_pBspArgs->bPMICRegister_00;
	else if (0x01 == Reg)
		return g_pBspArgs->bPMICRegister_01;

	return 0;
#else
	IIC_IO_DESC IIC_AddressData, IIC_Data;
	UCHAR buff[2] = {0,};
	DWORD bytes;
	BOOL bRet=0;

	IIC_AddressData.SlaveAddress = PMIC_ADDR;
	IIC_AddressData.Data = &Reg;
	IIC_AddressData.Count = 1;

	IIC_Data.SlaveAddress = PMIC_ADDR;
	IIC_Data.Data = buff;
	IIC_Data.Count = 1;

	bRet = DeviceIoControl(g_hFileI2C,  IOCTL_IIC_READ,
			&IIC_AddressData, sizeof(IIC_IO_DESC),
			&IIC_Data, sizeof(IIC_IO_DESC),
			&bytes, NULL);
	if (FALSE == bRet)
		MYERR((_T("[ETC] %d - IIC_ReadRegister(%d=%d)\r\n"), bRet, Reg, buff[0]));

	return buff[0];
#endif
}
static void i2c_WriteRegister(UCHAR Reg, UCHAR Val)
{
	if (INVALID_HANDLE_VALUE == g_hFileI2C)
	{
		MYERR((_T("[ETC] INVALID_HANDLE_VALUE == g_hFileI2C\r\n")));
		return;
	}

	IIC_IO_DESC IIC_Data;
	UCHAR buff[2];
	BOOL bRet=0;

	IIC_Data.SlaveAddress = PMIC_ADDR;
	IIC_Data.Data = buff;
	IIC_Data.Count = 2;

	buff[0] = Reg;
	buff[1] = Val;
	bRet = DeviceIoControl(g_hFileI2C, IOCTL_IIC_WRITE,
			&IIC_Data, sizeof(IIC_IO_DESC), NULL, 0, NULL, NULL);
	if (FALSE == bRet)
		MYERR((_T("[ETC] %d - IIC_WriteRegister(%d, %d)\r\n"), bRet, Reg, Val));

	if (0x00 == Reg)
		g_pBspArgs->bPMICRegister_00 = Val;
	else if (0x01 == Reg)
		g_pBspArgs->bPMICRegister_01 = Val;
}


#if	(EBOOK2_VER == 2)
static DWORD WINAPI KeyHoldThread(LPVOID lpParameter)
{
	BOOL bKeyHold, bRet;

	g_pGPIOReg->EINT0MASK |= (0x1<<6);	// Mask EINT6
	g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<12)) | (0x2<<12);	// Ext. Interrupt
	g_pGPIOReg->GPNPUD = (g_pGPIOReg->GPNPUD & ~(0x3<<12)) | (0x1<<12);	// pull-down enable
	g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT6))
		| (EINT_SIGNAL_BOTH_EDGE<<EINT0CON_EINT6);
	g_pGPIOReg->EINT0FLTCON0 = (g_pGPIOReg->EINT0FLTCON0 & ~(0x1<<FLTSEL_6)) | (0x1<<FLTEN_6);
	g_pGPIOReg->EINT0PEND = (0x1<<6);	// Clear pending EINT6
	g_pGPIOReg->EINT0MASK &= ~(0x1<<6);	// Unmask EINT6

	bKeyHold = (g_pGPIOReg->GPNDAT & (0x1<<6));
	ETC_IOControl(0, IOCTL_SET_KEY_HOLD, NULL, bKeyHold, NULL, 0, NULL);
	while (!g_bExitThreadKeyHold)
	{
		WaitForSingleObject(g_hEventKeyHold, INFINITE);

		if (g_bExitThreadKeyHold)
			break;

		g_pGPIOReg->EINT0MASK |= (0x1<<6);	// Mask EINT6
		g_pGPIOReg->EINT0PEND = (0x1<<6);	// Clear pending EINT6
		InterruptDone(g_dwSysIntrKeyHold);

		bKeyHold = (g_pGPIOReg->GPNDAT & (0x1<<6));
		bRet = ETC_IOControl(0, IOCTL_SET_KEY_HOLD, NULL, bKeyHold, NULL, 0, NULL);
		MYMSG((_T("[ETC] IOCTL_SET_KEY_HOLD(%d) = %d\n\r"), bKeyHold, bRet));

		g_pGPIOReg->EINT0MASK &= ~(0x1<<6);	// Unmask EINT6
	}

	return 0;
}
static BOOL keyhold_Initialize(void)
{
	DWORD dwIRQ = IRQ_EINT6;

	if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrKeyHold, sizeof(DWORD), NULL))
	{
		MYERR((_T("[ETC] !KernelIoControl()\n\r")));
		g_dwSysIntrKeyHold = SYSINTR_UNDEFINED;
		return FALSE;
	}
	g_hEventKeyHold = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == g_hEventKeyHold)
	{
		MYERR((_T("[ETC] NULL == g_hEventKeyHold\n\r")));
		return FALSE;
	}
	if (!InterruptInitialize(g_dwSysIntrKeyHold, g_hEventKeyHold, 0, 0))
	{
		MYERR((_T("[ETC] !InterruptInitialize()\n\r")));
		return FALSE;
	}
	g_hThreadKeyHold = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)KeyHoldThread, NULL, 0, NULL);
	if (NULL == g_hThreadKeyHold)
	{
		MYERR((_T("[ETC] NULL == g_hThreadKeyHold\n\r")));
		return FALSE;
	}

	return TRUE;
}
static void keyhold_Deinitialize(void)
{
	g_bExitThreadKeyHold = TRUE;
	if (g_hThreadKeyHold)
	{
		g_pGPIOReg->EINT0MASK |= (0x1<<6);	// Mask EINT6
		g_pGPIOReg->EINT0PEND = (0x1<<6);	// Clear pending EINT6
		SetEvent(g_hEventKeyHold);
		WaitForSingleObject(g_hThreadKeyHold, INFINITE);
		CloseHandle(g_hThreadKeyHold);
		g_hThreadKeyHold = NULL;
	}
	if (SYSINTR_UNDEFINED != g_dwSysIntrKeyHold)
		InterruptDisable(g_dwSysIntrKeyHold);
	if (NULL != g_hEventKeyHold)
		CloseHandle(g_hEventKeyHold);
	if (SYSINTR_UNDEFINED != g_dwSysIntrKeyHold)
		KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrKeyHold, sizeof(DWORD), NULL, 0, NULL);
	g_dwSysIntrKeyHold = SYSINTR_UNDEFINED;
	g_hEventKeyHold = NULL;
}
#endif	(EBOOK2_VER == 2)



DWORD ETC_Init(DWORD dwContext)
{
	PHYSICAL_ADDRESS ioPhysicalBase = {0,0};

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
	g_pGPIOReg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
	if (NULL == g_pGPIOReg)
	{
		MYERR((_T("[ETC] g_pGPIOReg = MmMapIoSpace()\r\n")));
		goto goto_err;
	}

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
	g_pSysConReg = (volatile S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
	if (NULL == g_pSysConReg)
	{
		MYERR((_T("[ETC] g_pSysConReg = MmMapIoSpace()\r\n")));
		goto goto_err;
	}

	ioPhysicalBase.LowPart = IMAGE_SHARE_ARGS_PA_START;
	g_pBspArgs = (volatile BSP_ARGS *)MmMapIoSpace(ioPhysicalBase, sizeof(BSP_ARGS), FALSE);
	if (NULL == g_pBspArgs)
	{
		MYERR((_T("[ETC] NULL == g_pBspArgs\r\n")));
		goto goto_err;
	}

	m_MutexEtc = CreateMutex(NULL, FALSE, NULL);
	if (NULL == m_MutexEtc)
	{
		MYERR((_T("[ETC] NULL == m_MutexEtc\r\n")));
		goto goto_err;
	}

	if (FALSE == i2c_Initialize())
	{
		MYERR((_T("[ETC] FALSE == i2c_Initialize()\r\n")));
		goto goto_err;
	}

#if	(EBOOK2_VER == 2)
	if (FALSE == keyhold_Initialize())
	{
		MYERR((_T("[ETC] FALSE == keyhold_Initialize()\r\n")));
		goto goto_err;
	}
#endif	(EBOOK2_VER == 2)

    return 0x12345678;
goto_err:
	ETC_Deinit(0);

	return 0;
}

BOOL ETC_Deinit(DWORD InitHandle)
{
#if	(EBOOK2_VER == 2)
	keyhold_Deinitialize();
#endif	(EBOOK2_VER == 2)

	i2c_Deinitialize();

	if (m_MutexEtc)
	{
		CloseHandle(m_MutexEtc);
		m_MutexEtc = NULL;
	}

	if (g_pBspArgs)
	{
		MmUnmapIoSpace((PVOID)g_pBspArgs, sizeof(BSP_ARGS));
		g_pBspArgs = NULL;
	}

	if (g_pSysConReg)
	{
		MmUnmapIoSpace((PVOID)g_pSysConReg, sizeof(S3C6410_SYSCON_REG));
		g_pSysConReg = NULL;
	}

	if (g_pGPIOReg)
	{
		MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
		g_pGPIOReg = NULL;
	}

	return TRUE;
}

DWORD ETC_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode)
{
	return InitHandle;
}

BOOL ETC_Close(DWORD OpenHandle)
{
	return TRUE;
}

BOOL ETC_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
    PBYTE pInBuf, DWORD nInBufSize,
    PBYTE pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned)
{
	BOOL bRet = FALSE;
	HANDLE hEvent = NULL;

	if (WAIT_OBJECT_0 != WaitForSingleObject(m_MutexEtc, INFINITE))
		MYERR((_T("[ETC] WAIT_OBJECT_0 != WaitForSingleObject\r\n")));
	switch (dwIoControlCode)
	{
	case IOCTL_SET_POWER_WLAN:
		hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("SDMMCCH0CardDetect_Event"));
		if (hEvent)
		{
			UCHAR i2c_Val = i2c_ReadRegister(0x00);
			MYERR((_T("[ETC] %d = i2c_ReadRegister(0x00)\r\n"), i2c_Val));

			if ((BOOL)nInBufSize)
			{
				g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<0)) | (0x1<<0);

				g_pBspArgs->bSDMMCCH0CardDetect = TRUE;
				SetEvent(hEvent);

				i2c_Val |= (1<<2);
				i2c_WriteRegister(0x00, i2c_Val);
			}
			else
			{
				i2c_Val &= ~(1<<2);
				i2c_WriteRegister(0x00, i2c_Val);

				g_pBspArgs->bSDMMCCH0CardDetect = FALSE;
				SetEvent(hEvent);

				g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<0)) | (0x0<<0);
			}
			CloseHandle(hEvent);
		}
	case IOCTL_GET_POWER_WLAN:
		bRet = (g_pGPIOReg->GPEDAT & (0x1<<0));
		break;

	case IOCTL_SET_POWER_WCDMA:
		if ((BOOL)nInBufSize)
			g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<1)) | (0x1<<1);
		else
			g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<1)) | (0x0<<1);
	case IOCTL_GET_POWER_WCDMA:
		bRet = (g_pGPIOReg->GPEDAT & (0x1<<1));
		break;

#if	(EBOOK2_VER == 2)
	case IOCTL_SET_KEY_HOLD:
		hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("EBOOK2_TSP"));	// Ebook2_touch.h
		g_pBspArgs->bKeyHold = (BOOL)nInBufSize;
		if (hEvent)
		{
			SetEvent(hEvent);
			CloseHandle(hEvent);
		}
	case IOCTL_GET_KEY_HOLD:
		bRet = g_pBspArgs->bKeyHold;//(g_pGPIOReg->GPNDAT & (0x1<<6));
		break;
#endif	(EBOOK2_VER == 2)
	}
	ReleaseMutex(m_MutexEtc);

	return bRet;
}

void ETC_PowerUp(DWORD InitHandle)
{
	//BOOL bKeyHold = (g_pGPIOReg->GPNDAT & (0x1<<6));
	//ETC_IOControl(0, IOCTL_SET_KEY_HOLD, NULL, bKeyHold, NULL, 0, NULL);
}

void ETC_PowerDown(DWORD InitHandle)
{
}

BOOL WINAPI ETC_DllMain(HINSTANCE DllInstance, DWORD Reason, LPVOID Reserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		DEBUGREGISTER(DllInstance);
		break;
	}
	return TRUE;
}

