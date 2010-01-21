
#include <battimpl.h>
#include <bsp.h>


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)

//#define BATT_LOG_TEST	1

#define MUTEX_TIMEOUT	5000
#define BATT_LEVEL_CNT	5
#define ADC_SAMPLE_NUM	8
#define ADC_LEVEL_MAX	2800
#define ADC_LEVEL_MIN	2200


static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_ADC_REG *g_pADCReg = NULL;
static volatile BSP_ARGS *g_pBspArgs = NULL;

static HANDLE g_hMutex = NULL;
static HANDLE g_hEventExit = NULL;
static HANDLE g_hThreadGetADC = NULL;
static SYSTEM_POWER_STATUS_EX2 g_PowerStatus = {0,};
static int g_anBattLevels[BATT_LEVEL_CNT];
static int g_nBattIndex;


static void lockBattery(void)
{
	WaitForSingleObject(g_hMutex, MUTEX_TIMEOUT);
}
static void unlockBattery(void)
{
	ReleaseMutex(g_hMutex);
}
static int getADC(void)
{
	UINT32 Old_ADCCON = g_pADCReg->ADCCON;
	UINT32 Old_ADCTSC = g_pADCReg->ADCTSC;
	int nADC[ADC_SAMPLE_NUM], i, j, k, nLevel;

	g_pADCReg->ADCCON = (1<<16)		// 12-bit A/D conversion
						|(1<<14)	// A/D converter prescaler enable
						|(10<<6)	// A/D converter prescaler value
						|(0<<2);	// Standby mode select
	g_pADCReg->ADCTSC = 0x58;		// Reset Value

	for (i=0; i<ADC_SAMPLE_NUM; i++)
	{
		g_pADCReg->ADCCON = (g_pADCReg->ADCCON & ~(7<<3)) | (0<<3);	// Channel setup
		g_pADCReg->ADCCON = (g_pADCReg->ADCCON & ~(1<<0)) | (1<<0);	// A/D conversion starts by enable
		while (g_pADCReg->ADCCON & (1<<0));		// Wait for begin sampling
		while (!(g_pADCReg->ADCCON & (1<<15)));	// Wait for the EOC
		g_pADCReg->ADCCON = (g_pADCReg->ADCCON & ~(1<<0)) | (0<<0);	// A/D conversion starts by disable
		nADC[i] = (g_pADCReg->ADCDAT0 & 0xFFF);
	}

	g_pADCReg->ADCCON = Old_ADCCON;
	g_pADCReg->ADCTSC = Old_ADCTSC;

	// 8 samples Interpolation (weighted 4 samples)
	for (j=0; j<ADC_SAMPLE_NUM-1; ++j)
	{
		for (k=j+1; k<ADC_SAMPLE_NUM; ++k)
		{
			if(nADC[j] > nADC[k])
			{
				nLevel  = nADC[j];
				nADC[j] = nADC[k];
				nADC[k] = nLevel;
			}
		}
	}
    nLevel = (nADC[2] + ((nADC[3]+nADC[4])<<1) + (nADC[3]+nADC[4]) + nADC[5]);
    if ((nLevel & 0x7) > 3)
		nLevel = (nLevel>>3) + 1;
    else
		nLevel = (nLevel>>3);

	return nLevel;
}


static DWORD WINAPI GetADCThread(LPVOID lpParameter)
{
	int nAvgLevel, nLevel, i=0, nPercent, nLEDCount=0;
	BOOL fAcOn, fUsbOn, fChgDone, fCharging;
	BYTE fBatteryState = 0;

	do
	{
		lockBattery();

		g_anBattLevels[g_nBattIndex++] = nLevel = getADC();
		g_nBattIndex %= BATT_LEVEL_CNT;
		for (nAvgLevel=0, i=0; i<BATT_LEVEL_CNT; i++)
		{
			if (0 > g_anBattLevels[i])
				break;
			nAvgLevel += g_anBattLevels[i];
		}
		if (i)
			nAvgLevel /= i;
		if (ADC_LEVEL_MAX < nAvgLevel)
			nAvgLevel = ADC_LEVEL_MAX;
		else if (ADC_LEVEL_MIN > nAvgLevel)
			nAvgLevel = ADC_LEVEL_MIN;
		nPercent = (int)(((nAvgLevel-ADC_LEVEL_MIN) * 100.0) / (ADC_LEVEL_MAX-ADC_LEVEL_MIN));

		fBatteryState = (g_pGPIOReg->GPNDAT & 0x0F);
		fAcOn = !((1<<0) & fBatteryState);
		fUsbOn = !((1<<1) & fBatteryState);
		fChgDone = !((1<<2) & fBatteryState);
		fCharging = !((1<<3) & fBatteryState);
		MYMSG((_T("%d, %d, %d, AC_ON(%d), USB_ON(%d), CHG_DONE(%d), CHARGING(%d)\r\n"),
			nLevel, nAvgLevel, nPercent, fAcOn, fUsbOn, fChgDone, fCharging));
		if (fAcOn || fUsbOn)
		{
			g_PowerStatus.ACLineStatus = AC_LINE_ONLINE;
			g_PowerStatus.BatteryFlag = BATTERY_FLAG_CHARGING;
		}
		else
		{
			g_PowerStatus.ACLineStatus = AC_LINE_OFFLINE;
#ifdef	BATT_LOG_TEST
			g_PowerStatus.BatteryFlag = BATTERY_FLAG_HIGH;
#else	//!BATT_LOG_TEST
			g_PowerStatus.BatteryFlag = (60 <= nPercent) ? BATTERY_FLAG_HIGH : BATTERY_FLAG_LOW;
			/*if (60 <= nPercent)
				g_PowerStatus.BatteryFlag = BATTERY_FLAG_HIGH;
			else if (20 <= nPercent)
				g_PowerStatus.BatteryFlag = BATTERY_FLAG_LOW;
			else
				g_PowerStatus.BatteryFlag = BATTERY_FLAG_CRITICAL;*/
#endif	BATT_LOG_TEST
		}
		if (5 > nPercent)
			nPercent = 5;
		g_PowerStatus.BatteryLifePercent    = nPercent;
		g_PowerStatus.BatteryVoltage        = nLevel;
		g_PowerStatus.BatteryCurrent        = nAvgLevel;
		g_PowerStatus.BatteryAverageCurrent = fBatteryState;

		if (0 == g_pBspArgs->dwLEDCheck)
		{
			if (fAcOn || fCharging)
			{
				// LED_R#[7] : 1Sec ON, 1Sec OFF
				g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT | (0x1<<7)) & ~((nLEDCount%2)<<7);
			}
			else
			{
				if (fChgDone)	// LED_R#[7] : ON
					g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT | (0x1<<7)) & ~(0x1<<7);
				else	// LED_R#[7] : 1Sec ON, 2Sec OFF
				{
#if	1
					g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT | (0x1<<7)) & ~(0x0<<7);
#else
					if (nLEDCount % 3)
						g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT | (0x1<<7)) & ~(0x0<<7);
					else
						g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT | (0x1<<7)) & ~(0x1<<7);
#endif
				}
			}
		}
		nLEDCount++;

		unlockBattery();
	} while (WAIT_OBJECT_0 != WaitForSingleObject(g_hEventExit, 1000));

	return 0;
}

BOOL WINAPI BatteryPDDInitialize(LPCTSTR pszRegistryContext)
{
	PHYSICAL_ADDRESS ioPhysicalBase;

	SETFNAME(_T("BatteryPDDInitialize"));
	UNREFERENCED_PARAMETER(pszRegistryContext);

	g_PowerStatus.ACLineStatus              = AC_LINE_OFFLINE;
	g_PowerStatus.BatteryFlag               = BATTERY_FLAG_HIGH;
	g_PowerStatus.BatteryLifePercent        = 100;
	g_PowerStatus.Reserved1                 = 0;
	g_PowerStatus.BatteryLifeTime           = BATTERY_LIFE_UNKNOWN;
	g_PowerStatus.BatteryFullLifeTime       = BATTERY_LIFE_UNKNOWN;
	g_PowerStatus.Reserved2                 = 0;
	g_PowerStatus.BackupBatteryFlag         = BATTERY_FLAG_UNKNOWN;
	g_PowerStatus.BackupBatteryLifePercent  = BATTERY_PERCENTAGE_UNKNOWN;
	g_PowerStatus.Reserved3                 = 0;
	g_PowerStatus.BackupBatteryLifeTime     = BATTERY_LIFE_UNKNOWN;
	g_PowerStatus.BackupBatteryFullLifeTime = BATTERY_LIFE_UNKNOWN;
	g_PowerStatus.BatteryVoltage            = 0;
	g_PowerStatus.BatteryCurrent            = 0;
	g_PowerStatus.BatteryAverageCurrent     = 0;
	g_PowerStatus.BatteryAverageInterval    = 0;
	g_PowerStatus.BatterymAHourConsumed     = 0;
	g_PowerStatus.BatteryTemperature        = 0;
	g_PowerStatus.BackupBatteryVoltage      = 0;
	g_PowerStatus.BatteryChemistry          = BATTERY_CHEMISTRY_LIPOLY;

	memset(g_anBattLevels, -1, sizeof(g_anBattLevels));
	g_nBattIndex = 0;

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
	g_pGPIOReg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
	if (NULL == g_pGPIOReg)
	{
		MYERR((_T("[BAT_ERR] g_pGPIOReg = MmMapIoSpace()\r\n")));
		goto goto_err;
	}
	// GPN[3:0] : CHARGING#(3), CHG_DONE#(2), USBPWR_OK#(1), DCPWR_OK#(0)
	g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0xFF<<0)) | (0x00<<0);	// input mode
	g_pGPIOReg->GPNPUD = (g_pGPIOReg->GPNPUD & ~(0xFF<<0)) | (0xAA<<0);	// pull-up enable
	// LED_R# - GPA[7] - Output(1), Pull-up/down disabled(0)
	g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xf<<28)) | (0x1<<28);
	g_pGPIOReg->GPAPUD &= ~(0x3<<14);
	g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x1<<7)) | (1<<7);

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_ADC;
	g_pADCReg = (volatile S3C6410_ADC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_ADC_REG), FALSE);
	if (NULL == g_pADCReg)
	{
		MYERR((_T("[BAT_ERR] g_pADCReg = MmMapIoSpace()\r\n")));
		goto goto_err;
	}

	ioPhysicalBase.LowPart = IMAGE_SHARE_ARGS_PA_START;
	g_pBspArgs = (volatile BSP_ARGS *)MmMapIoSpace(ioPhysicalBase, sizeof(BSP_ARGS), FALSE);
	if (NULL == g_pBspArgs)
	{
		MYERR((_T("[BAT_ERR] NULL == g_pBspArgs\r\n")));
		goto goto_err;
	}

	g_hMutex = CreateMutex(NULL, FALSE, NULL);
	if (NULL == g_hMutex)
	{
		MYERR((_T("[BAT_ERR] g_hMutex = CreateMutex()\r\n")));
		goto goto_err;
	}

	g_hEventExit = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(NULL == g_hEventExit)
	{
		MYERR((_T("[BAT_ERR] NULL == g_hEventExit\n\r")));
		goto goto_err;
	}
	g_hThreadGetADC = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetADCThread, NULL, 0, NULL);
	if (NULL == g_hThreadGetADC)
	{
		MYERR((_T("[BAT_ERR] NULL == g_hThreadGetADC\n\r")));
		goto goto_err;
	}

	return TRUE;
goto_err:
	BatteryPDDDeinitialize();
	return FALSE;
}

void WINAPI BatteryPDDDeinitialize(void)
{
	SETFNAME(_T("BatteryPDDDeinitialize"));

	if (g_hThreadGetADC)
	{
		SetEvent(g_hEventExit);
		WaitForSingleObject(g_hThreadGetADC, INFINITE);
		CloseHandle(g_hThreadGetADC);
		g_hThreadGetADC = NULL;
		CloseHandle(g_hEventExit);
		g_hEventExit = NULL;
	}

	if (g_hMutex)
	{
		CloseHandle(g_hMutex);
		g_hMutex = NULL;
	}

	if (g_pBspArgs)
	{
		MmUnmapIoSpace((PVOID)g_pBspArgs, sizeof(BSP_ARGS));
		g_pBspArgs = NULL;
	}

	if(g_pADCReg)
	{
		MmUnmapIoSpace((PVOID)g_pADCReg, sizeof(S3C6410_ADC_REG));
		g_pADCReg = NULL;
	}

	if(g_pGPIOReg)
	{
		MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
		g_pGPIOReg = NULL;
	}
}

void WINAPI BatteryPDDResume(void)
{
	SETFNAME(_T("BatteryPDDResume"));
	memset(g_anBattLevels, -1, sizeof(g_anBattLevels));
	g_nBattIndex = 0;
}

void WINAPI BatteryPDDPowerHandler(BOOL bOff)
{
	SETFNAME(_T("BatteryPDDPowerHandler"));
	UNREFERENCED_PARAMETER(bOff);
}

BOOL WINAPI BatteryPDDGetStatus(
	PSYSTEM_POWER_STATUS_EX2 pstatus,
	PBOOL pfBatteriesChangedSinceLastCall)
{
	lockBattery();

	memcpy(pstatus, &g_PowerStatus, sizeof(SYSTEM_POWER_STATUS_EX2));
	*pfBatteriesChangedSinceLastCall = FALSE;

	unlockBattery();

	return (TRUE);
}

LONG BatteryPDDGetLevels(void)
{
	LONG lLevels = MAKELONG(3, 3);	// mainbattery/backupbattery levels

	SETFNAME(_T("BatteryPDDPowerHandler"));
	lockBattery();
	// ...
	unlockBattery();

	return lLevels;
}

BOOL BatteryPDDSupportsChangeNotification(void)
{
	BOOL fSupportsChange = FALSE;

	SETFNAME(_T("BatteryPDDPowerHandler"));
	lockBattery();
	// ...
	unlockBattery();

	return fSupportsChange;
}

