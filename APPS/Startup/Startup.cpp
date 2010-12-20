//
// EBook2Startup.cpp
//

#include <windows.h>
#include "s1d13521.h"
#include "iphlpapi.h"
#include "etc.h"
#include <usbfnioctl.h>
#include <devload.h>


#define OMNIBOOK_REG_KEY			_T("Software\\Omnibook")

#define APP_STARTUP_REG_STRING		_T("AppStartup")
#define APP_STARTUP_REG_DEFAULT		_T("\\eBook Store\\eBook_MainApp.exe")

#define APP_UPDATE_REG_STRING		_T("AppUpdate")
#define APP_UPDATE_REG_DEFAULT		_T("\\Storage Card\\eBook_UpdateApp.exe")

#define APP_SIPSYMBOL_REG_STRING	_T("AppSipSymbol")
#define APP_SIPSYMBOL_REG_DEFAULT	_T("\\Windows\\Omnibook_SipSymbol.exe")

#define	WIFI_CARDNAME_TCHAR			_T("SDIO86861")
#define	WIFI_CARDNAME_CHAR			"SDIO86861"
#define	WIFI_CARDNAME_LEN			9

#define	USB_FUN_DEV_NAME			_T("UFN1:")
#define	ARRAY_COUNT(x)				(sizeof(x) / sizeof(x[0]))


LPCTSTR	g_szUsbClassName[] = {
	_T("Serial_Class"),
	_T("Mass_Storage_Class"),
};
static int ufnGetCurrentClientName(void)
{
	HANDLE hUSBFn = INVALID_HANDLE_VALUE;

	hUSBFn = CreateFile(USB_FUN_DEV_NAME, DEVACCESS_BUSNAMESPACE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hUSBFn)
	{
		UFN_CLIENT_INFO ufnInfo = {0,};
		DWORD dwBytes;

		DeviceIoControl(hUSBFn,	IOCTL_UFN_GET_CURRENT_CLIENT, NULL, 0, &ufnInfo, sizeof(ufnInfo), &dwBytes, NULL);
		RETAILMSG(0, (_T("IOCTL_UFN_GET_CURRENT_CLIENT\r\n")));
		RETAILMSG(0, (_T("    ClientName = %s\r\n"), ufnInfo.szName));
		RETAILMSG(0, (_T("    Description = %s\r\n"), ufnInfo.szDescription));

		CloseHandle(hUSBFn);

		for (int nMode=0; nMode<ARRAY_COUNT(g_szUsbClassName); nMode++)
		{
			if (g_szUsbClassName[nMode][0] == ufnInfo.szName[0])
				return nMode;
		}
	}

	return -1;
}

static BOOL RegOpenCreateStr(LPCTSTR lpSubKey, LPCTSTR lpName, LPTSTR lpData, DWORD dwCnt, BOOL bCreate)
{
	HKEY hKey;
	DWORD dwValue, dwType = REG_MULTI_SZ;
	BOOL bRet = TRUE;

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		lpSubKey,
		0,
		NULL,
		0,	// REG_OPTION_NON_VOLATILE
		0,
		0,
		&hKey,
		&dwValue))
	{
		return FALSE;
	}

	if (FALSE == bCreate && REG_CREATED_NEW_KEY == dwValue)
	{
		bRet = FALSE;
	}
	else if (REG_OPENED_EXISTING_KEY == dwValue)
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, lpName, 0, &dwType, (BYTE *)lpData, &dwCnt))
			RETAILMSG(0, (_T("%s\r\n"), lpData));
		else
			bRet = FALSE;
	}
	else
	{
		bRet = FALSE;
	}

	RegCloseKey(hKey);

	return bRet;
}

static BOOL RegOpenCreateDword(LPCTSTR lpSubKey, LPCTSTR lpName, LPDWORD lpData, BOOL bCreate)
{
	HKEY hKey;
	DWORD dwValue, dwType = REG_DWORD, dwCnt = sizeof(DWORD);
	BOOL bRet = TRUE;

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		lpSubKey,
		0,
		NULL,
		0,	// REG_OPTION_NON_VOLATILE
		0,
		0,
		&hKey,
		&dwValue))
	{
		return FALSE;
	}

	if (FALSE == bCreate && REG_CREATED_NEW_KEY == dwValue)
	{
		bRet = FALSE;
	}
	else if (REG_OPENED_EXISTING_KEY == dwValue)
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, lpName, 0, &dwType, (BYTE *)lpData, &dwCnt))
			RETAILMSG(0, (_T("%d\r\n"), *lpData));
		else
			bRet = FALSE;
	}
	else
	{
		bRet = FALSE;
	}

	RegCloseKey(hKey);

	return bRet;
}


static BOOL IsProgram(LPCWSTR lpszImageName)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL bResult = TRUE;

	hFile = CreateFileW(lpszImageName,
		GENERIC_READ,
		NULL, NULL,
		OPEN_EXISTING,
		NULL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		bResult = FALSE;
	CloseHandle(hFile);

	return bResult;
}

static BOOL RunProgram(LPCWSTR lpszImageName, LPCWSTR lpszCmdLine, DWORD dwWait)
{
	PROCESS_INFORMATION pi = {0,};
	BOOL bResult = FALSE;

	bResult = CreateProcessW(lpszImageName,
		lpszCmdLine,// Command line.
		NULL,	// Process handle not inheritable.
		NULL,	// Thread handle not inheritable.
		FALSE,	// Set handle inheritance to FALSE.
		0,		// No creation flags.
		NULL,	// Use parent's environment block.
		NULL,	// Use parent's starting directory.
		NULL,	// Pointer to STARTUPINFO structure.
		&pi);	// Pointer to PROCESS_INFORMATION structure.
	if (bResult)
	{
		if (dwWait)
			WaitForSingleObject(pi.hThread, dwWait);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	return bResult;
}

static BOOL CheckWifiMacAddress(HANDLE hEtc)
{
	BOOL bRet = FALSE;
	BYTE abInfo[512]={0,};
 	int nRetry = 0;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;

	bRet = DeviceIoControl(hEtc, IOCTL_GET_BOARD_INFO, NULL, 100, abInfo, sizeof(abInfo), NULL, NULL);
	if (FALSE == bRet)
	{
		RETAILMSG(1, (_T("ERROR : DeviceIoControl(IOCTL_GET_BOARD_INFO)\r\n")));
		goto goto_Cleanup;
	}

	if (0 == memcmp(abInfo, "SJMT", 4) && 0 == memcmp(&abInfo[30], "MAC ", 4))
	{
		UINT8 szUUID[16]={0,};
		memcpy(szUUID, abInfo, 16);
		bRet = DeviceIoControl(hEtc, IOCTL_SET_BOARD_UUID, szUUID, sizeof(szUUID), NULL, 0, NULL, NULL);
		goto goto_Cleanup;
	}

goto_Retry:
	DeviceIoControl(hEtc, IOCTL_SET_POWER_WLAN, NULL, TRUE, NULL, 0, NULL, NULL);

	{
		ULONG ulSizeAdapterInfo = 0;
		DWORD dwReturnvalueGetAdapterInfo, i;
		PIP_ADAPTER_INFO pOriginalPtr;
		TCHAR szAdapterName[MAX_ADAPTER_NAME_LENGTH + 4 + 1];

		for (i=0; i<20; i++)
		{
			dwReturnvalueGetAdapterInfo = GetAdaptersInfo(pAdapterInfo, &ulSizeAdapterInfo);
			if (ERROR_NO_DATA == dwReturnvalueGetAdapterInfo)
			{
				RETAILMSG(0, (_T("Loop : ERROR_NO_DATA == dwReturnvalueGetAdapterInfo %d\r\n"), i));
				Sleep(100);
			}
			else if (ERROR_BUFFER_OVERFLOW == dwReturnvalueGetAdapterInfo)
			{
				RETAILMSG(0, (_T("ERROR : ERROR_BUFFER_OVERFLOW == dwReturnvalueGetAdapterInfo %d, %d\r\n"), i, ulSizeAdapterInfo));
				if (!(pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo)))
				{
					RETAILMSG(1, (_T("ERROR : Insufficient Memory\r\n")));
					goto goto_Cleanup;
				}
			}
			else if (ERROR_SUCCESS == dwReturnvalueGetAdapterInfo)
			{
				RETAILMSG(1, (_T("ERROR_SUCCESS == dwReturnvalueGetAdapterInfo %d, %d\r\n"), i, ulSizeAdapterInfo));
				break;
			}
			else
			{
				RETAILMSG(1, (_T("ERROR : GetAdaptersInfo failed (error %ld)\r\n"), dwReturnvalueGetAdapterInfo));
				goto goto_Cleanup;
			}
		}
		if (NULL == pAdapterInfo)
		{
			nRetry++;
			if (3 < nRetry)
			{
				RETAILMSG(1, (_T("ERROR : GetAdaptersInfo failed (error ERROR_NO_DATA)\r\n")));
				goto goto_Cleanup;
			}

			DeviceIoControl(hEtc, IOCTL_SET_POWER_WLAN, NULL, FALSE, NULL, 0, NULL, NULL);
			Sleep(100);
			RETAILMSG(1, (_T("ERROR : GetAdaptersInfo failed (goto_Retry %d)\r\n"), nRetry));
			goto goto_Retry;
		}

		pOriginalPtr = pAdapterInfo;
		while (NULL != pAdapterInfo)
		{
			i = MultiByteToWideChar(CP_ACP, 0, pAdapterInfo->AdapterName, strlen(pAdapterInfo->AdapterName)+1,
				szAdapterName, MAX_ADAPTER_NAME_LENGTH + 4 + 1);
			szAdapterName[i] = NULL;
			RETAILMSG(1, (_T("\t Adapter Name ...... : %s\r\n"), szAdapterName));
			if (0 == strncmp(pAdapterInfo->AdapterName, WIFI_CARDNAME_CHAR, WIFI_CARDNAME_LEN))
			{
				UINT8 szUUID[16] = {'S','J','M','T',};
				char *pTmp = (char *)&szUUID[4];
				RETAILMSG(1, (_T("\t Address............ : ")));
				for (i=0; i<pAdapterInfo->AddressLength; i++)
				{
					pTmp += sprintf(pTmp, "%02x", pAdapterInfo->Address[i]);
					RETAILMSG(1, (_T("%02x"), pAdapterInfo->Address[i]));
				}
				RETAILMSG(1, (_T("\r\n")));
				DeviceIoControl(hEtc, IOCTL_SET_BOARD_UUID, szUUID, sizeof(szUUID), NULL, 0, NULL, NULL);

				memcpy(abInfo, szUUID, 16);
				szUUID[0] = 'M'; szUUID[1] = 'A'; szUUID[2] = 'C'; szUUID[3] = ' ';
				memcpy(&abInfo[30], szUUID, 16);
				bRet = DeviceIoControl(hEtc, IOCTL_SET_BOARD_INFO, abInfo, sizeof(abInfo), NULL, 100, NULL, NULL);
				if (FALSE == bRet)
				{
					RETAILMSG(1, (_T("ERROR : DeviceIoControl(IOCTL_SET_BOARD_INFO)\r\n")));
					goto goto_Cleanup;
				}
				break;
			}

			pAdapterInfo = pAdapterInfo->Next;
			RETAILMSG(1, (_T("\r\n")));
		}
		pAdapterInfo = NULL;
		if (pOriginalPtr)  
			free(pOriginalPtr);
	}

	bRet = TRUE;

goto_Cleanup:
	if (pAdapterInfo)
		free(pAdapterInfo);

	return bRet;
}

static void DirtyRectUpdate(HDC hDC)
{
	ExtEscape(hDC, DRVESC_SET_DIRTYRECT, TRUE, NULL, 0, NULL);
	{
		DISPUPDATE du;
		du.bWriteImage = TRUE;
		du.pRect = NULL;
		du.duState = DSPUPD_FULL;
		du.bBorder = FALSE;
		du.wfMode = WAVEFORM_GC;
		ExtEscape(hDC, DRVESC_DISP_UPDATE, sizeof(du), (LPCSTR)&du, 0, NULL);
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HANDLE hEtc = INVALID_HANDLE_VALUE;
	BOOL bIsMassStorage = FALSE;
	TCHAR szProgram[MAX_PATH] = {0,};
	BOOL bDispUpdate = FALSE;

	hEtc = CreateFile(ETC_DRIVER_NAME,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE != hEtc)
		CheckWifiMacAddress(hEtc);

	bIsMassStorage = (1 == ufnGetCurrentClientName()) ? TRUE : FALSE;

	sndPlaySound(_T("\\Windows\\Startup.wav"), SND_FILENAME | SND_ASYNC);

	if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, APP_STARTUP_REG_STRING, szProgram, MAX_PATH, FALSE))
	{
		RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
			APP_STARTUP_REG_STRING, APP_STARTUP_REG_DEFAULT));
		_tcscpy(szProgram, APP_STARTUP_REG_DEFAULT);
	}
	if (IsProgram(szProgram))
	{
		if (RunProgram(szProgram, NULL, 0))
		{
			RETAILMSG(1, (_T("RunProgram(%s)\r\n"), szProgram));
			bDispUpdate = TRUE;
		}
		else
		{
			RETAILMSG(1, (_T("ERROR : RunProgram(%s)\r\n"), szProgram));
		}
	}
	if (FALSE == bDispUpdate)
	{
		HDC hDC = GetDC(HWND_DESKTOP);
		DirtyRectUpdate(hDC);
		InvalidateRect(HWND_DESKTOP, NULL, TRUE);
		ReleaseDC(HWND_DESKTOP, hDC);

		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, APP_UPDATE_REG_STRING, szProgram, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
				APP_UPDATE_REG_STRING, APP_UPDATE_REG_DEFAULT));
			_tcscpy(szProgram, APP_UPDATE_REG_DEFAULT);
		}
		if (IsProgram(szProgram))
		{
			if (RunProgram(szProgram, NULL, 0))
				RETAILMSG(1, (_T("RunProgram(%s)\r\n"), szProgram));
			else
				RETAILMSG(1, (_T("ERROR : RunProgram(%s)\r\n"), szProgram));
		}
		else
		{
			RETAILMSG(1, (_T("ERROR : Not Found - %s\r\n"), szProgram));
		}
	}

	if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, APP_SIPSYMBOL_REG_STRING, szProgram, MAX_PATH, FALSE))
	{
		RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
			APP_SIPSYMBOL_REG_STRING, APP_SIPSYMBOL_REG_DEFAULT));
		_tcscpy(szProgram, APP_SIPSYMBOL_REG_DEFAULT);
	}
	if (IsProgram(szProgram))
	{
		RunProgram(szProgram, NULL, 0);
		RETAILMSG(1, (_T("RunProgram : %s\r\n"), szProgram));
	}
	else
	{
		RETAILMSG(1, (_T("ERROR : Not Found - %s\r\n"), szProgram));
	}

	HDC hDC = GetDC(HWND_DESKTOP);
	BOOL bDirtyRect = (BOOL)ExtEscape(hDC, DRVESC_GET_DIRTYRECT, 0, NULL, 0, NULL);
	for (int i=0; (FALSE==bDirtyRect && i<5); i++)
	{
		Sleep(1000);
		bDirtyRect = (BOOL)ExtEscape(hDC, DRVESC_GET_DIRTYRECT, 0, NULL, 0, NULL);
		if (bIsMassStorage)
			bDirtyRect = DeviceIoControl(hEtc, IOCTL_IS_ATTACH_UFN, NULL, 0, NULL, 0, NULL, NULL);
	}
	if (FALSE == bDirtyRect)
		DirtyRectUpdate(hDC);
	ReleaseDC(HWND_DESKTOP, hDC);

	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("PowerManager/ReloadActivityTimeouts"));
	if (hEvent)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
		RETAILMSG(0, (_T("SetEvent(PowerManager/ReloadActivityTimeouts)\r\n")));
	}

	if (INVALID_HANDLE_VALUE != hEtc)
	{
		DeviceIoControl(hEtc, IOCTL_SET_POWER_WLAN, NULL, FALSE, NULL, 0, NULL, NULL);
		CloseHandle(hEtc);
	}

	return 0;
}

