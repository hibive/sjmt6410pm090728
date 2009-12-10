
#include <bsp.h>
#include "touch_uart.h"


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)

#define COMPORT		_T("COM2:")
#define BAUDRATE	19200
#define BITSYNC		0x80
#define BITPENDOWN	0x01
#define PRESSURE	256		// Max 1023


static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static HANDLE m_MutexTouch = NULL;

static DWORD g_dwSysIntrPenDet = SYSINTR_UNDEFINED;
static HANDLE g_hEventPenDet = NULL;
static HANDLE g_hThreadPenDet = NULL;
static BOOL g_bExitThread = FALSE;

static HANDLE g_hComPort = INVALID_HANDLE_VALUE;
static HANDLE g_hDriverThread = NULL;
static BOOL g_bDriverRunning = TRUE;

static BYTE g_bOrientation = 2;	// Display(0->1, 1->0)


DWORD TSP_Init(DWORD dwContext);
BOOL TSP_Deinit(DWORD InitHandle);
DWORD TSP_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode);
BOOL TSP_Close(DWORD OpenHandle);
BOOL TSP_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
    PBYTE pInBuf, DWORD nInBufSize,
    PBYTE pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned);
void TSP_PowerUp(DWORD InitHandle);
void TSP_PowerDown(DWORD InitHandle);


static BOOL parsePacket(UINT8 *pPacket)
{
	static BOOL bIsDown = FALSE;

	POINT ptCurTabletPos, ptCurScreenPos;
	LONG curPressure;
	UINT8 penDown, verInfo;

	penDown = (pPacket[0]&0x1);
	verInfo = ((pPacket[0]&0xE)>>1);
	ptCurTabletPos.x = ((LONG)((pPacket[1]<<9)|(pPacket[2]<<2)|((pPacket[6]&0x60)>>5)));
	ptCurTabletPos.y = ((LONG)((pPacket[3]<<9)|(pPacket[4]<<2)|((pPacket[6]&0x18)>>3)));
	curPressure = ((LONG)(pPacket[5]|((pPacket[6]&0x07)<<7)));
	MYMSG((_T("[TSP] x, y, p, o(%d, %d, %d, %d)\r\n"), ptCurTabletPos.x, ptCurTabletPos.y, curPressure, g_bOrientation));

	double ratioX, ratioY;
	ratioX = (double)ptCurTabletPos.x / 6145.0;
	ratioY = (double)ptCurTabletPos.y / 8193.0;
	switch (g_bOrientation)
	{
	case 0:	// 0
		ptCurScreenPos.x = (LONG)(65535.0 * ratioX);
		ptCurScreenPos.y = (LONG)(65535.0 * ratioY);
		break;
	case 1:	// 90
		ptCurScreenPos.x = (LONG)(65535.0 * ratioY);
		ptCurScreenPos.y = (LONG)(65535.0 * ratioX);
		
		ptCurScreenPos.y = 65535 - ptCurScreenPos.y;
		break;
	case 2:	// 180
		ptCurScreenPos.x = 65535 - (LONG)(65535.0 * ratioX);
		ptCurScreenPos.y = 65535 - (LONG)(65535.0 * ratioY);
		break;
	case 3:	// 270
		ptCurScreenPos.x = (LONG)(65535.0 * ratioY);
		ptCurScreenPos.y = (LONG)(65535.0 * ratioX);

		ptCurScreenPos.x = 65535 - ptCurScreenPos.x;
		break;
	default:
		break;
	}

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = ptCurScreenPos.x;
	input.mi.dy = ptCurScreenPos.y;
	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	if ((pPacket[0] & BITPENDOWN) && (PRESSURE < curPressure))
	{
		bIsDown = TRUE;
		input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
		SendInput(1, &input, sizeof(input));
		MYMSG((_T("[TSP] DN (%d, %d)\r\n"), input.mi.dx, input.mi.dy));
	}
	else if (TRUE == bIsDown)
	{
		bIsDown = FALSE;
		input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
		SendInput(1, &input, sizeof(input));
		MYMSG((_T("[TSP] UP (%d, %d)\r\n"), input.mi.dx, input.mi.dy));
	}

	return TRUE;
}
static DWORD WINAPI DriverThreadProc(LPVOID lpParameter)
{
	DWORD dwIntr, dwRead;
	UINT8 packetByte, packet[7];

	MYMSG((_T("+++ [TSP] DriverThreadProc Start\r\n")));

	SetCommMask(g_hComPort, EV_RXCHAR);
	while (g_bDriverRunning)
	{
		WaitCommEvent(g_hComPort, &dwIntr, NULL);
		do
		{
			ReadFile(g_hComPort, &packetByte, 1, &dwRead, NULL);
			while (!(packetByte & BITSYNC) && dwRead)
				ReadFile(g_hComPort, &packetByte, 1, &dwRead, NULL);

			if (0 < dwRead)
			{
				packet[0] = packetByte;
				ReadFile(g_hComPort, &packet[1], 6, &dwRead, NULL);
				parsePacket(packet);
			}
		} while (dwRead > 0);
	}

	MYMSG((_T("--- [TSP] DriverThreadProc End\r\n")));

	return 0;
}
static void closeComPort(void)
{
	MYMSG((_T("+++ [TSP] closeComPort Start\r\n")));

	g_bDriverRunning = FALSE;
	
	if (INVALID_HANDLE_VALUE != g_hComPort)
	{
		// Wake read thread if it is blocked
		SetCommMask(g_hComPort, NULL);
	}

	if (g_hDriverThread)
	{
		WaitForSingleObject(g_hDriverThread, 2000);
		CloseHandle(g_hDriverThread);
		g_hDriverThread = NULL;
	}

	if (INVALID_HANDLE_VALUE != g_hComPort)
	{
		CloseHandle(g_hComPort);
		g_hComPort = INVALID_HANDLE_VALUE;
	}

	MYMSG((_T("--- [TSP] closeComPort End\r\n")));
}
static BOOL openComPort(void)
{
	DCB dcbComm;
	COMMTIMEOUTS commTimeout;
	DWORD dwDriverThreadId;

	if (INVALID_HANDLE_VALUE != g_hComPort)
		return FALSE;

	MYMSG((_T("+++ [TSP] openComPort Start\r\n")));

	g_hComPort = CreateFile(COMPORT,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (INVALID_HANDLE_VALUE == g_hComPort)
	{
		MYERR((_T("[TSP] INVALID_HANDLE_VALUE == g_hComPort\r\n")));
		goto goto_err;
	}

	dcbComm.DCBlength = sizeof(DCB);
	if (FALSE == GetCommState(g_hComPort, &dcbComm))
	{
		MYERR((_T("[TSP] FALSE == GetCommState()\r\n")));
		goto goto_err;
	}
	dcbComm.BaudRate = BAUDRATE;
	dcbComm.Parity	 = NOPARITY;
	dcbComm.ByteSize = 8;
	dcbComm.StopBits = ONESTOPBIT; 
	if (FALSE == SetCommState(g_hComPort, &dcbComm))
	{
		MYERR((_T("[TSP] FALSE == SetCommState()\r\n")));
		goto goto_err;
	}

	if (FALSE == GetCommTimeouts(g_hComPort, &commTimeout))
	{
		MYERR((_T("[TSP] FALSE == GetCommTimeouts()\r\n")));
		goto goto_err;
	}
	commTimeout.ReadIntervalTimeout = 1;
	commTimeout.ReadTotalTimeoutMultiplier = 1;
	commTimeout.ReadTotalTimeoutConstant = 0;
	if (FALSE == SetCommTimeouts(g_hComPort, &commTimeout))
	{
		MYERR((_T("[TSP] FALSE == SetCommTimeouts()\r\n")));
		goto goto_err;
	}

	g_bDriverRunning = TRUE;

	g_hDriverThread = CreateThread(NULL,
		0,
		DriverThreadProc,
		NULL,
		0,
		&dwDriverThreadId);
	if (NULL == g_hDriverThread)
	{
		MYERR((_T("[TSP] NULL == g_hDriverThread\r\n")));
		goto goto_err;
	}
	CeSetThreadPriority(g_hDriverThread, 248);	// default(251)

	MYMSG((_T("--- [TSP] openComPort End\r\n")));

	return TRUE;
goto_err:
	closeComPort();
	return FALSE;
}

static DWORD WINAPI PenDetThread(LPVOID lpParameter)
{
	BOOL bPenDet = FALSE, bRet;

	// GPE[2] : TOUCH_SLP(2)
	g_pGPIOReg->GPECON = (g_pGPIOReg->GPECON & ~(0xF<<8)) | (0x1<<8);	// output mode
	g_pGPIOReg->GPEPUD = (g_pGPIOReg->GPEPUD & ~(0x3<<4)) | (0x0<<4);	// pull-up/down disable
	g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<2)) | (0x0<<2);	// TOUCH_SLP(2)

	g_pGPIOReg->EINT0MASK |= (0x1<<4);	// Mask EINT4
	g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0x3<<8)) | (0x2<<8);	// Ext. Interrupt
	g_pGPIOReg->GPNPUD = (g_pGPIOReg->GPNPUD & ~(0x3<<8)) | (0x2<<8);	// pull-up enable
	g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT4))
		| (EINT_SIGNAL_BOTH_EDGE<<EINT0CON_EINT4);
	g_pGPIOReg->EINT0FLTCON0 = (g_pGPIOReg->EINT0FLTCON0 & ~(0x1<<FLTSEL_4)) | (0x1<<FLTEN_4);
	g_pGPIOReg->EINT0PEND = (0x1<<4);	// Clear pending EINT4
	g_pGPIOReg->EINT0MASK &= ~(0x1<<4);	// Unmask EINT4

	openComPort();

	bPenDet = (g_pGPIOReg->GPNDAT & (0x1<<4));
	TSP_IOControl(0, IOCTL_TSP_SET_ENABLE, NULL, bPenDet, NULL, 0, NULL);
	while (!g_bExitThread)
	{
		WaitForSingleObject(g_hEventPenDet, INFINITE);
		if (g_bExitThread)
			break;

		g_pGPIOReg->EINT0MASK |= (0x1<<4);	// Mask EINT4
		g_pGPIOReg->EINT0PEND = (0x1<<4);	// Clear pending EINT4
		InterruptDone(g_dwSysIntrPenDet);

		bPenDet = (g_pGPIOReg->GPNDAT & (0x1<<4));
		bRet = TSP_IOControl(0, IOCTL_TSP_SET_ENABLE, NULL, bPenDet, NULL, 0, NULL);
		MYMSG((_T("[TSP] IOCTL_TSP_SET_ENABLE(%d) = %d\r\n"), bPenDet, bRet));

		g_pGPIOReg->EINT0MASK &= ~(0x1<<4);	// Unmask EINT4
	}

	closeComPort();

	return 0;
}

DWORD TSP_Init(DWORD dwContext)
{
	PHYSICAL_ADDRESS ioPhysicalBase = {0,0};
	DWORD dwIRQ;

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
	g_pGPIOReg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
	if (NULL == g_pGPIOReg)
	{
		MYERR((_T("[TSP] NULL == g_pGPIOReg\r\n")));
		goto goto_err;
	}

	m_MutexTouch = CreateMutex(NULL, FALSE, NULL);
	if (NULL == m_MutexTouch)
	{
		MYERR((_T("[TSP] NULL == m_MutexTouch\r\n")));
		goto goto_err;
	}

	dwIRQ = IRQ_EINT4;
	if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrPenDet, sizeof(DWORD), NULL))
	{
		MYERR((_T("[TSP] !KernelIoControl()\n\r")));
		g_dwSysIntrPenDet = SYSINTR_UNDEFINED;
		goto goto_err;
	}
	g_hEventPenDet = CreateEvent(NULL, FALSE, FALSE, TSP_EVENT_NAME);
	if(NULL == g_hEventPenDet)
	{
		MYERR((_T("[TSP] NULL == g_hEventPenDet\n\r")));
		goto goto_err;
	}
	if (!InterruptInitialize(g_dwSysIntrPenDet, g_hEventPenDet, 0, 0))
	{
		MYERR((_T("[TSP] !InterruptInitialize()\n\r")));
		goto goto_err;
	}
	g_hThreadPenDet = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PenDetThread, NULL, 0, NULL);
	if (NULL == g_hThreadPenDet)
	{
		MYERR((_T("[TSP] NULL == g_hThreadPenDet\n\r")));
		goto goto_err;
	}

    return 0x12345678;
goto_err:
	TSP_Deinit(0);

	return 0;
}

BOOL TSP_Deinit(DWORD InitHandle)
{
	closeComPort();

	g_bExitThread = TRUE;
	if (g_hThreadPenDet)
	{
		g_pGPIOReg->EINT0MASK |= (0x1<<4);	// Mask EINT4
		g_pGPIOReg->EINT0PEND = (0x1<<4);	// Clear pending EINT4
		SetEvent(g_hEventPenDet);
		WaitForSingleObject(g_hThreadPenDet, INFINITE);
		CloseHandle(g_hThreadPenDet);
		g_hThreadPenDet = NULL;
	}
	if (SYSINTR_UNDEFINED != g_dwSysIntrPenDet)
		InterruptDisable(g_dwSysIntrPenDet);
	if (NULL != g_hEventPenDet)
		CloseHandle(g_hEventPenDet);
	if (SYSINTR_UNDEFINED != g_dwSysIntrPenDet)
		KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrPenDet, sizeof(DWORD), NULL, 0, NULL);
	g_dwSysIntrPenDet = SYSINTR_UNDEFINED;
	g_hEventPenDet = NULL;

	if (m_MutexTouch)
	{
		CloseHandle(m_MutexTouch);
		m_MutexTouch = NULL;
	}

	if (g_pGPIOReg)
	{
		MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
		g_pGPIOReg = NULL;
	}

	return TRUE;
}

DWORD TSP_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode)
{
	return InitHandle;
}

BOOL TSP_Close(DWORD OpenHandle)
{
	return TRUE;
}

BOOL TSP_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
    PBYTE pInBuf, DWORD nInBufSize,
    PBYTE pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned)
{
	BOOL bRet = FALSE;

	if (WAIT_OBJECT_0 != WaitForSingleObject(m_MutexTouch, INFINITE))
		MYERR((_T("[TSP] WAIT_OBJECT_0 != WaitForSingleObject\r\n")));
	switch (dwIoControlCode)
	{
	case IOCTL_TSP_SET_ORIENTATION:	// 0(0), 90(1), 180(2), 270(3)
		if (3 >= (BYTE)nInBufSize)
			g_bOrientation = (BYTE)nInBufSize;
	case IOCTL_TSP_GET_ORIENTATION:	// 0(0), 90(1), 180(2), 270(3)
		bRet = g_bOrientation;
		break;

	case IOCTL_TSP_SET_ENABLE:
		if ((BOOL)nInBufSize && (g_pGPIOReg->GPNDAT & (0x1<<4)))
			g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<2)) | (0x0<<2);
		else
			g_pGPIOReg->GPEDAT = (g_pGPIOReg->GPEDAT & ~(0x1<<2)) | (0x1<<2);
	case IOCTL_TSP_GET_ENABLE:
		bRet = !(g_pGPIOReg->GPEDAT & (0x1<<2));
		break;
	}
	ReleaseMutex(m_MutexTouch);

	return bRet;
}

void TSP_PowerUp(DWORD InitHandle)
{
	BOOL bPenDet = (g_pGPIOReg->GPNDAT & (0x1<<4));
	TSP_IOControl(0, IOCTL_TSP_SET_ENABLE, NULL, bPenDet, NULL, 0, NULL);
}

void TSP_PowerDown(DWORD InitHandle)
{
}

BOOL WINAPI TSP_DllMain(HINSTANCE DllInstance, DWORD Reason, LPVOID Reserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		DEBUGREGISTER(DllInstance);
		break;
	}
	return TRUE;
}

