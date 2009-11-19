
#include <bsp.h>
#include "s3c6410_acceleration.h"
#include "bma150.h"


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)


bma150_t bma150;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static DWORD g_dwSysIntrAcc = SYSINTR_UNDEFINED;
static HANDLE g_hEventAcc = NULL;
static HANDLE g_hEventAccNotify = NULL;
static HANDLE g_hThreadAcc = NULL;
static BOOL g_bExitThread = FALSE;


INT WINAPI AccelerationThread(void)
{
	while (!g_bExitThread)
	{
		WaitForSingleObject(g_hEventAcc, INFINITE);
		if (g_bExitThread)
			break;

		g_pGPIOReg->EINT0MASK |= (0x1<<10);		// Mask EINT10
		g_pGPIOReg->EINT0PEND  = (0x1<<10);		// Clear pending EINT10

		// ...
		SetEvent(g_hEventAccNotify);
#if	0
{
		BYTE int_status=0, int_mask=0;
		bma150_get_interrupt_status(&int_status);
		bma150_get_interrupt_mask(&int_mask);
		RETAILMSG(1, (_T("[ACC] interrupt(0x%X, 0x%X)\r\n"), int_status, int_mask));
		bma150_reset_interrupt();
}
#endif
		// ...

		InterruptDone(g_dwSysIntrAcc);
		g_pGPIOReg->EINT0MASK &= ~(0x1<<10);	// Unmask EINT10
	}

	return 0;
}



DWORD ACC_Init(LPCTSTR pContext)
{
	PHYSICAL_ADDRESS ioPhysicalBase = {0,0};
	DWORD dwIRQ;

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
	g_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
	if (g_pGPIOReg == NULL)
	{
		RETAILMSG(1, (_T("[ACC:ERR] %s() : pGPIOReg MmMapIoSpace() Failed \n\r"), _T(__FUNCTION__)));
		return 0;
	}

	dwIRQ = IRQ_EINT10;
	g_dwSysIntrAcc = SYSINTR_UNDEFINED;
	g_hEventAcc = NULL;

	if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrAcc, sizeof(DWORD), NULL))
	{
		RETAILMSG(1, (_T("[ACC:ERR] %s() : IOCTL_HAL_REQUEST_SYSINTR Failed \n\r"), _T(__FUNCTION__)));
		g_dwSysIntrAcc = SYSINTR_UNDEFINED;
		return FALSE;
	}

	g_hEventAcc = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(NULL == g_hEventAcc)
	{
		RETAILMSG(1, (_T("[ACC:ERR] %s() : CreateEvent() Failed \n\r"), _T(__FUNCTION__)));
		return FALSE;
	}
	g_hEventAccNotify = CreateEvent(NULL, TRUE, FALSE, ACC_NOTIFY_EVENT);
	if(NULL == g_hEventAccNotify)
	{
		RETAILMSG(1, (_T("[ACC:ERR] %s() : CreateEvent() Failed \n\r"), _T(__FUNCTION__)));
		return FALSE;
	}

	if (!(InterruptInitialize(g_dwSysIntrAcc, g_hEventAcc, 0, 0)))
	{
		RETAILMSG(1, (_T("[ACC:ERR] %s() : InterruptInitialize() Failed \n\r"), _T(__FUNCTION__)));
		return FALSE;
	}

	g_pGPIOReg->EINT0MASK |= (0x1<<10);		// Mask EINT10
	SET_GPIO(g_pGPIOReg, GPNCON, 10, GPNCON_EXTINT);
	SET_GPIO(g_pGPIOReg, GPNPUD, 10, GPNPUD_PULLDOWN);
	g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT10)) | (EINT_SIGNAL_RISE_EDGE<<EINT0CON_EINT10);
	g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(0x1<<FLTSEL_10)) | (0x1<<FLTEN_10);
	g_pGPIOReg->EINT0PEND = (0x1<<10);		// Clear pending EINT10
	g_pGPIOReg->EINT0MASK &= ~(0x1<<10);	// Unmask EINT10

	g_hThreadAcc = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AccelerationThread, NULL, 0, NULL);
	if (g_hThreadAcc == NULL)
	{
		RETAILMSG(1, (_T("[ACC:ERR] %s() : CreateThread() Failed \n\r"), _T(__FUNCTION__)));
		return FALSE;
	}

#ifdef	DoInitInDriverLoad
	if (0 != bma150_init(&bma150))
		return 0;
#endif	DoInitInDriverLoad

#if	0
	bma150_set_any_motion_threshold(BMA150_ANY_MOTION_THRES_IN_G(1.2, 2.0));
	bma150_set_any_motion_int(1);
	bma150_set_advanced_int(1);
	bma150_set_interrupt_mask(INT_MASK_ANY_MOTION|INT_MASK_EN_ADV_INT);
#endif

#if	0
	bma150_set_low_g_threshold(BMA150_LG_THRES_IN_G(0.35, 2));
	bma150_set_low_g_int(1);
	bma150_set_interrupt_mask(INT_MASK_LG);
#endif

#if	0
	bma150_set_new_data_int(1);
	bma150_set_interrupt_mask(INT_MASK_NEW_DATA);
#endif

	return (DWORD)pContext;
}

BOOL ACC_Deinit(DWORD hDeviceContext)
{
#ifdef	DoInitInDriverLoad
	bma150_deinit(&bma150);
#endif	DoInitInDriverLoad

	g_bExitThread = TRUE;
	if (g_hThreadAcc)		  // Make Sure if thread is exist
	{
		g_pGPIOReg->EINT0MASK |= (0x1<<10);		// Mask EINT10
		g_pGPIOReg->EINT0PEND  = (0x1<<10);		// Clear pending EINT10

		// Signal Thread to Finish
		SetEvent(g_hEventAcc);
		// Wait for Thread to Finish
		WaitForSingleObject(g_hThreadAcc, INFINITE);
		CloseHandle(g_hThreadAcc);
		g_hThreadAcc = NULL;
	}

	if (g_pGPIOReg != NULL)
	{
		MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
		g_pGPIOReg = NULL;
	}

	if (g_dwSysIntrAcc != SYSINTR_UNDEFINED)
	{
		InterruptDisable(g_dwSysIntrAcc);
	}

	if (g_hEventAccNotify != NULL)
	{
		CloseHandle(g_hEventAccNotify);
		g_hEventAccNotify = NULL;
	}
	if (g_hEventAcc != NULL)
	{
		CloseHandle(g_hEventAcc);
		g_hEventAcc = NULL;
	}

	if (g_dwSysIntrAcc != SYSINTR_UNDEFINED)
	{
		KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrAcc, sizeof(DWORD), NULL, 0, NULL);
		g_dwSysIntrAcc = SYSINTR_UNDEFINED;
	}

	return TRUE;
}

DWORD ACC_Open(DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode)
{
	return hDeviceContext;
}

BOOL ACC_Close(DWORD hOpenContext)
{
	return TRUE;
}

BOOL ACC_IOControl(DWORD hOpenContext, DWORD dwCode,
	PBYTE pBufIn, DWORD dwLenIn,
	PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
	DWORD dwErr = ERROR_INVALID_PARAMETER;

	switch (dwCode)
	{
	case IOCTL_ACC_INIT:
#ifndef	DoInitInDriverLoad
		dwErr = bma150_init(&bma150);
#else
		dwErr = ERROR_SUCCESS;
#endif	DoInitInDriverLoad
		break;	// 0x00
	case  IOCTL_ACC_DEINIT:
#ifndef	DoInitInDriverLoad
		dwErr = bma150_deinit(&bma150);
#else
		dwErr = ERROR_SUCCESS;
#endif	DoInitInDriverLoad
		break;	// 0x01

	case  IOCTL_ACC_SOFT_RESET:
		dwErr = bma150_soft_reset();
		break;	// 0x02

	case  IOCTL_ACC_UPDATE_IMAGE:
		dwErr = bma150_update_image();
		break;	// 0x03

	case  IOCTL_ACC_SET_IMAGE:
		if (pBufIn != NULL && dwLenIn == sizeof(IMAGE_T))
			dwErr = bma150_set_image((IMAGE_T *)pBufIn);
		break;	// 0x04
	case  IOCTL_ACC_GET_IMAGE:
		if (pBufOut != NULL && dwLenOut == sizeof(IMAGE_T))
			dwErr = bma150_get_image((IMAGE_T *)pBufOut);
		break;	// 0x05

	case  IOCTL_ACC_SET_OFFSET:
		if (pBufIn != NULL && dwLenIn == sizeof(OFFSET_T))
			dwErr = bma150_set_offset(((OFFSET_T *)pBufIn)->axis, ((OFFSET_T *)pBufIn)->offset);
		break;	// 0x06
	case  IOCTL_ACC_GET_OFFSET:
		if (pBufOut != NULL && dwLenOut == sizeof(OFFSET_T))
			dwErr = bma150_get_offset(((OFFSET_T *)pBufOut)->axis, &((OFFSET_T *)pBufOut)->offset);
		break;	// 0x07

	case  IOCTL_ACC_SET_OFFSET_EEPROM:
		if (pBufIn != NULL && dwLenIn == sizeof(OFFSET_T))
			dwErr = bma150_set_offset_eeprom(((OFFSET_T *)pBufIn)->axis, ((OFFSET_T *)pBufIn)->offset);
		break;	// 0x08

	case  IOCTL_ACC_SET_EE_W:
		if (pBufIn != NULL && dwLenIn == sizeof(EE_W_TYPE))
			dwErr = bma150_set_ee_w(*(unsigned char *)pBufIn);
		break;	// 0x09

	case  IOCTL_ACC_WRITE_EE:
		if (pBufIn !=NULL && dwLenIn == sizeof(EE_T))
			dwErr = bma150_write_ee(((EE_T *)pBufIn)->addr, ((EE_T *)pBufIn)->data);
		break;	// 0x0A

	case  IOCTL_ACC_SELFTEST:
		if (pBufIn != NULL && dwLenIn == sizeof(SELF_TEST_TYPE))
			dwErr = bma150_selftest(*(unsigned char *)pBufIn);
		break;	// 0x0B

	case  IOCTL_ACC_SET_RANGE:
		if (pBufIn != NULL && dwLenIn == sizeof(RANGE_TYPE))
			dwErr = bma150_set_range(*(unsigned char *)pBufIn);
		break;	// 0x0C
	case  IOCTL_ACC_GET_RANGE:
		if (pBufOut != NULL && dwLenOut == sizeof(RANGE_TYPE))
			dwErr = bma150_get_range((unsigned char *)pBufOut);
		break;	// 0x0D

	case  IOCTL_ACC_SET_MODE:
		if (pBufIn != NULL && dwLenIn == sizeof(MODE_TYPE))
			dwErr = bma150_set_mode(*(unsigned char *)pBufIn);
		break;	// 0x0E
	case  IOCTL_ACC_GET_MODE:
		if (pBufOut != NULL && dwLenOut == sizeof(MODE_TYPE))
			dwErr = bma150_get_mode((unsigned char *)pBufOut);
		break;	// 0x0F

	case  IOCTL_ACC_SET_BANDWIDTH:
		if (pBufIn != NULL && dwLenIn == sizeof(BANDWIDTH_TYPE))
			dwErr = bma150_set_bandwidth(*(unsigned char *)pBufIn);
		break;	// 0x10
	case  IOCTL_ACC_GET_BANDWIDTH:
		if (pBufOut != NULL && dwLenOut == sizeof(BANDWIDTH_TYPE))
			dwErr = bma150_get_bandwidth((unsigned char *)pBufOut);
		break;	// 0x11

	case  IOCTL_ACC_SET_WAKE_UP_PAUSE:
		if (pBufIn != NULL && dwLenIn == sizeof(WAKE_UP_PAUSE_TYPE))
			dwErr = bma150_set_wake_up_pause(*(unsigned char *)pBufIn);
		break;	// 0x12
	case  IOCTL_ACC_GET_WAKE_UP_PAUSE:
		if (pBufOut != NULL && dwLenOut == sizeof(WAKE_UP_PAUSE_TYPE))
			dwErr = bma150_get_wake_up_pause((unsigned char *)pBufOut);
		break;	// 0x13

	case  IOCTL_ACC_SET_LOW_G_THRESHOLD:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_low_g_threshold(*(unsigned char *)pBufIn);
		break;	// 0x14
	case  IOCTL_ACC_GET_LOW_G_THRESHOLD:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_low_g_threshold((unsigned char *)pBufOut);
		break;	// 0x15

	case  IOCTL_ACC_SET_LOW_G_COUNTDOWN:
		if (pBufIn != NULL && dwLenIn == sizeof(COUNTER_TYPE))
			dwErr = bma150_set_low_g_countdown(*(unsigned char *)pBufIn);
		break;	// 0x16
	case  IOCTL_ACC_GET_LOW_G_COUNTDOWN:
		if (pBufOut != NULL && dwLenOut == sizeof(COUNTER_TYPE))
			dwErr = bma150_get_low_g_countdown((unsigned char *)pBufOut);
		break;	// 0x17

	case  IOCTL_ACC_SET_LOW_G_DURATION:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_low_g_duration(*(unsigned char *)pBufIn);
		break;	// 0x18
	case  IOCTL_ACC_GET_LOW_G_DURATION:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_low_g_duration((unsigned char *)pBufOut);
		break;	// 0x19

	case  IOCTL_ACC_SET_LOW_G_HYST:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_low_g_hysteresis(*(unsigned char *)pBufIn);
		break;	// 0x1A
	case  IOCTL_ACC_GET_LOW_G_HYST:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_low_g_hysteresis((unsigned char *)pBufOut);
		break;	// 0x1B

	case  IOCTL_ACC_SET_HIGH_G_THRESHOLD:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_high_g_threshold(*(unsigned char *)pBufIn);
		break;	// 0x1C
	case  IOCTL_ACC_GET_HIGH_G_THRESHOLD:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_high_g_threshold((unsigned char *)pBufOut);
		break;	// 0x1D

	case  IOCTL_ACC_SET_HIGH_G_COUNTDOWN:
		if (pBufIn != NULL && dwLenIn == sizeof(COUNTER_TYPE))
			dwErr = bma150_set_high_g_countdown(*(unsigned char *)pBufIn);
		break;	// 0x1E
	case  IOCTL_ACC_GET_HIGH_G_COUNTDOWN:
		if (pBufOut != NULL && dwLenOut == sizeof(COUNTER_TYPE))
			dwErr = bma150_get_high_g_countdown((unsigned char *)pBufOut);
		break;	// 0x1F

	case  IOCTL_ACC_SET_HIGH_G_DURATION:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_high_g_duration(*(unsigned char *)pBufIn);
		break;	// 0x20
	case  IOCTL_ACC_GET_HIGH_G_DURATION:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_high_g_duration((unsigned char *)pBufOut);
		break;	// 0x21

	case  IOCTL_ACC_SET_HIGH_G_HYST:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_high_g_hysteresis(*(unsigned char *)pBufIn);
		break;	// 0x22
	case  IOCTL_ACC_GET_HIGH_G_HYST:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_high_g_hysteresis((unsigned char *)pBufOut);
		break;	// 0x23

	case  IOCTL_ACC_SET_ANY_MOTION_THRESHOLD:
		if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
			dwErr = bma150_set_any_motion_threshold(*(unsigned char *)pBufIn);
		break;	// 0x24
	case  IOCTL_ACC_GET_ANY_MOTION_THRESHOLD:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_get_any_motion_threshold((unsigned char *)pBufOut);
		break;	// 0x25

	case  IOCTL_ACC_SET_ANY_MOTION_COUNT:
		if (pBufIn != NULL && dwLenIn == sizeof(ANY_MOTION_DUR_TYPE))
			dwErr = bma150_set_any_motion_count(*(unsigned char *)pBufIn);
		break;	// 0x26
	case  IOCTL_ACC_GET_ANY_MOTION_COUNT:
		if (pBufOut != NULL && dwLenOut == sizeof(ANY_MOTION_DUR_TYPE))
			dwErr = bma150_get_any_motion_count((unsigned char *)pBufOut);
		break;	// 0x27

	case  IOCTL_ACC_SET_INTERRUPT_MASK:
		if (pBufIn != NULL && dwLenIn == sizeof(INT_MASK_TYPE))
			dwErr = bma150_set_interrupt_mask(*(unsigned char *)pBufIn);
		break;	// 0x28
	case  IOCTL_ACC_GET_INTERRUPT_MASK:
		if (pBufOut != NULL && dwLenOut == sizeof(INT_MASK_TYPE))
			dwErr = bma150_get_interrupt_mask((unsigned char *)pBufOut);
		break;	// 0x29

	case  IOCTL_ACC_RESET_INTERRUPT:
		dwErr = bma150_reset_interrupt();
		break;	// 0x2A

	case  IOCTL_ACC_READ_ACCEL_X:
		if (pBufOut != NULL && dwLenOut == sizeof(short))
			dwErr = bma150_read_accel_x((short *)pBufOut);
		break;	// 0x2B
	case  IOCTL_ACC_READ_ACCEL_Y:
		if (pBufOut != NULL && dwLenOut == sizeof(short))
			dwErr = bma150_read_accel_y((short *)pBufOut);
		break;	// 0x2C
	case  IOCTL_ACC_READ_ACCEL_Z:
		if (pBufOut != NULL && dwLenOut == sizeof(short))
			dwErr = bma150_read_accel_z((short *)pBufOut);
		break;	// 0x2D
	case  IOCTL_ACC_READ_TEMPERATURE:
		if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
			dwErr = bma150_read_temperature((unsigned char *)pBufOut);
		break;	// 0x2E
	case  IOCTL_ACC_READ_ACCEL_XYZT:
		if (pBufOut != NULL && dwLenOut == sizeof(ACC_T))
			dwErr = bma150_read_accel_xyzt((ACC_T *)pBufOut);
		break;	// 0x2F

	case  IOCTL_ACC_GET_INTERRUPT_STATUS:
		if (pBufOut != NULL && dwLenOut == sizeof(INT_STATUS_TYPE))
			dwErr = bma150_get_interrupt_status((unsigned char *)pBufOut);
		break;	// 0x30

	case  IOCTL_ACC_SET_LOW_G_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_set_low_g_int(*pBufIn ? 1 : 0);
		break;	// 0x31
	case  IOCTL_ACC_SET_HIGH_G_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_set_high_g_int(*pBufIn ? 1 : 0);
		break;	// 0x32
	case  IOCTL_ACC_SET_ANY_MOTION_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_set_any_motion_int(*pBufIn ? 1 : 0);
		break;	// 0x33
	case  IOCTL_ACC_SET_ALERT_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_set_alert_int(*pBufIn ? 1 : 0);
		break;	// 0x34
	case  IOCTL_ACC_SET_ADVANCED_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_set_advanced_int(*pBufIn ? 1 : 0);
		break;	// 0x35
	case  IOCTL_ACC_LATCH_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_latch_int(*pBufIn ? 1 : 0);
		break;	// 0x36
	case  IOCTL_ACC_SET_NEW_DATA_INT:
		if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
			dwErr = bma150_set_new_data_int(*pBufIn ? 1 : 0);
		break;	// 0x37

	case  IOCTL_ACC_PAUSE:
		if (pBufIn != NULL && dwLenIn == sizeof(int))
			dwErr = bma150_pause(*(int*)pBufIn);
		break;	// 0x38

	case  IOCTL_ACC_WRITE_REG:
		if (pBufIn != NULL && dwLenIn == sizeof(REG_T))
			dwErr = bma150_write_reg(((REG_T *)pBufIn)->addr, ((REG_T *)pBufIn)->data, ((REG_T *)pBufIn)->len);
		break;	// 0x39
	case  IOCTL_ACC_READ_REG:
		if (pBufOut != NULL && dwLenOut == sizeof(REG_T))
			dwErr = bma150_read_reg(((REG_T *)pBufOut)->addr, ((REG_T *)pBufOut)->data, ((REG_T *)pBufOut)->len);
		break;	// 0x3A

	case  IOCTL_ACC_WAIT_INTERRUPT:
		if (pBufOut != NULL && dwLenOut == sizeof(INT_MASK_TYPE))
			dwErr = bma150_wait_interrupt((unsigned char *)pBufOut);
		break;	// 0x3B

	default:
		break;
	}

	//TCHAR szBuf[MAX_PATH];
	//_stprintf_s(szBuf, MAX_PATH, _T("ACC_IOControl - dwCode(%08x), dwErr(%d)\r\n"), dwCode, dwErr);
	//MYERR((szBuf));

	// pass back appropriate response codes
	SetLastError(dwErr);
	return (ERROR_SUCCESS == dwErr) ? TRUE : FALSE;
}

void ACC_PowerUp(DWORD hDeviceContext)
{
}

void ACC_PowerDown(DWORD hDeviceContext)
{
}

DWORD ACC_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}

DWORD ACC_Write(DWORD hOpenContext, LPCVOID pBuffer, DWORD Count)
{
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}

DWORD ACC_Seek(DWORD hOpenContext, long Amount, WORD Type)
{
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}


BOOL WINAPI ACC_DllMain(HINSTANCE DllInstance, DWORD Reason, LPVOID Reserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		DEBUGREGISTER(DllInstance);
		break;
	}
	return TRUE;
}

