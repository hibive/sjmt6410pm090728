
#include <battimpl.h>
#include <bsp.h>


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)

#define MUTEX_TIMEOUT	5000
#define BATT_LEVEL_CNT	5
#define ADC_SAMPLE_NUM	8
#define ADC_LEVEL_MAX	2860
#define ADC_LEVEL_MIN	2300


static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_ADC_REG *g_pADCReg = NULL;

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
	DWORD dwRet;
	int nAvgLevel, nLevel, i=0, nPercent, nLEDCount=0;
	BOOL fAcOn, fUsbOn, fChgDone, fCharging;
	BYTE fBatteryState = 0;

	while (1)
	{
		dwRet = WaitForSingleObject(g_hEventExit, 1000);
		if (WAIT_OBJECT_0 == dwRet)
			break;

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
			nPercent = ADC_LEVEL_MAX;
		else if (ADC_LEVEL_MIN > nAvgLevel)
			nPercent = ADC_LEVEL_MIN;
		else
			nPercent = nAvgLevel;
		nPercent = (int)(((nPercent-ADC_LEVEL_MIN) * 100.0) / (ADC_LEVEL_MAX-ADC_LEVEL_MIN));
		// 90% = 2809 ~ 2804
		// 80% = 2753 ~ 2748
		// 70% = 2697 ~ 2692
		// 60% = 2641 ~ 2636
		// 50% = 2585 ~ 2581
		// 40% = 2529 ~ 2524
		// 30% = 2473 ~ 2468	// 25% = 2445 ~ 2440
		// 20% = 2417 ~ 2412

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
			if (70 <= nPercent)
				g_PowerStatus.BatteryFlag = BATTERY_FLAG_HIGH;
			else if (25 <= nPercent)
			{
				g_PowerStatus.BatteryFlag = BATTERY_FLAG_LOW;
				if (30 > nPercent)
				{
					LPCTSTR lpszPathName = _T("\\Windows\\EBook2Command.exe");
					PROCESS_INFORMATION pi;

					ZeroMemory(&pi,sizeof(pi));
					if (CreateProcess(lpszPathName,
									  _T("LOWBATTERY"),	// pszCmdLine
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
			}
			else
				g_PowerStatus.BatteryFlag = BATTERY_FLAG_CRITICAL;
		}
		g_PowerStatus.BatteryLifePercent    = nPercent;
		g_PowerStatus.BatteryVoltage        = nLevel;
		g_PowerStatus.BatteryCurrent        = nAvgLevel;
		g_PowerStatus.BatteryAverageCurrent = fBatteryState;

		if (fCharging)	// LED_B[6] : ON
		{
			g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x1<<6)) | (0x1<<6);
		}
		else			// LED_B[6]
		{
			i = fChgDone ? 2 : 4;
			if (nLEDCount % i)
				g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x1<<6)) | (0x0<<6);
			else
				g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x1<<6)) | (0x1<<6);
		}
		nLEDCount++;

		unlockBattery();
	}

	return 0;
}

BOOL WINAPI BatteryPDDInitialize(LPCTSTR pszRegistryContext)
{
	PHYSICAL_ADDRESS ioPhysicalBase;

	SETFNAME(_T("BatteryPDDInitialize"));
	UNREFERENCED_PARAMETER(pszRegistryContext);

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
	g_pGPIOReg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
	if (NULL == g_pGPIOReg)
	{
		MYERR((_T("[BAT_ERR] g_pGPIOReg = MmMapIoSpace()\r\n")));
		goto goto_err;
	}
	// GPN[3:0] : CHARGING#(3), CHG_DONW#(2), USBPWR_OK#(1), DCPWR_OK#(0)
	g_pGPIOReg->GPNCON = (g_pGPIOReg->GPNCON & ~(0xFF<<0)) | (0x00<<0);	// input mode
	g_pGPIOReg->GPNPUD = (g_pGPIOReg->GPNPUD & ~(0xFF<<0)) | (0x00<<0);	// pull-up/down disable
	// LED_R, LED_B - GPA[7:6] - Output(1), Pull-up/down disabled(0)
	g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<24)) | (0x11<<24);
	g_pGPIOReg->GPAPUD &= ~(0xf<<12);
	g_pGPIOReg->GPADAT = (g_pGPIOReg->GPADAT & ~(0x3<<6)) | (0<<6);

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_ADC;
	g_pADCReg = (volatile S3C6410_ADC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_ADC_REG), FALSE);
	if (NULL == g_pADCReg)
	{
		MYERR((_T("[BAT_ERR] g_pADCReg = MmMapIoSpace()\r\n")));
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

