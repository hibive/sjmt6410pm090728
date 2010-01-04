//
// EBook2Startup.cpp
//

#include <windows.h>
#include "s1d13521.h"
#include <winsock2.h>
#include <iptypes.h>
#include "iphlpapi.h"
#include "etc.h"


#define OMNIBOOK_REG_KEY			_T("Software\\Omnibook")

#define APP_STARTUP_REG_STRING		_T("AppStartup")
#define APP_STARTUP_REG_DEFAULT		_T("\\Omnibook Store\\Omnibook_MainApp.exe")

#define APP_UPDATE_REG_STRING		_T("AppUpdate")
#define APP_UPDATE_REG_DEFAULT		_T("\\Storage Card\\Omnibook_UpdateApp.exe")

#define APP_SIPSYMBOL_REG_STRING	_T("AppSipSymbol")
#define APP_SIPSYMBOL_REG_DEFAULT	_T("\\Windows\\Omnibook_SipSymbol.exe")

#define BMP_STARTUPTIME_REG_STRING	_T("BmpStartupTime")
#define BMP_STARTUPTIME_REG_DEFAULT	3000	// mSec

#define CFG_SKIPREADMAC_REG_STRING	_T("CfgSkipReadMac")
#define CFG_SKIPREADMAC_REG_DEFAULT	0

#define	WIFI_CARDNAME_TCHAR			_T("SDIO86861")
#define	WIFI_CARDNAME_CHAR			"SDIO86861"
#define	WIFI_CARDNAME_LEN			9


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

static BOOL CheckWifiMacAddress(void)
{
	BOOL bRet = FALSE;
	HANDLE hEtc;
 	int nRetry = 0;
	WSADATA WsaData;

	hEtc = CreateFile(ETC_DRIVER_NAME,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, 0);
 	if (INVALID_HANDLE_VALUE == hEtc)
	{
		RETAILMSG(1, (_T("ERROR : INVALID_HANDLE_VALUE == CreateFile(%s)\r\n"), ETC_DRIVER_NAME));
		goto goto_Cleanup;
	}

goto_Retry:
	DeviceIoControl(hEtc, IOCTL_SET_POWER_WLAN, NULL, TRUE, NULL, 0, NULL, NULL);
	if (0 != WSAStartup(MAKEWORD(1, 1), &WsaData))
	{
		RETAILMSG(1, (_T("ERROR : WSAStartup failed (error %ld)\r\n"), GetLastError()));
		goto goto_Cleanup;
	}

	{
		PIP_ADAPTER_INFO pAdapterInfo = NULL, pOriginalPtr;
		ULONG ulSizeAdapterInfo = 0;
		DWORD dwReturnvalueGetAdapterInfo, i;
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
				DeviceIoControl(hEtc, IOCTL_SET_BOARD_UUID, szUUID, 16, NULL, 0, NULL, NULL);
			}

			pAdapterInfo = pAdapterInfo->Next;
			RETAILMSG(1, (_T("\r\n")));
		}
		if (pOriginalPtr)  
			free(pOriginalPtr);
	}

	WSACleanup();
	bRet = TRUE;

goto_Cleanup:
	if (INVALID_HANDLE_VALUE != hEtc)
	{
		DeviceIoControl(hEtc, IOCTL_SET_POWER_WLAN, NULL, FALSE, NULL, 0, NULL, NULL);
		CloseHandle(hEtc);
	}

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
	DWORD dwSkipReadMac=0, dwStartupTime=0, dwStart=0, dwTotal, i;
	TCHAR szProgram[MAX_PATH]={0,};
	BOOL bLoop=TRUE, bDispUpdate=FALSE;

	if (RunProgram(_T("\\Windows\\Omnibook_Command.exe"), _T("STARTUP"), 3000))
		dwStart = GetTickCount();

	if (FALSE == RegOpenCreateDword(OMNIBOOK_REG_KEY, CFG_SKIPREADMAC_REG_STRING, &dwSkipReadMac, FALSE))
	{
		RETAILMSG(1, (_T("RegOpenCreateDword(%s), Default(%d)\r\n"),
			CFG_SKIPREADMAC_REG_STRING, CFG_SKIPREADMAC_REG_DEFAULT));
		dwSkipReadMac = CFG_SKIPREADMAC_REG_DEFAULT;
	}
	if (0 == dwSkipReadMac)
		CheckWifiMacAddress();

	if (FALSE == RegOpenCreateDword(OMNIBOOK_REG_KEY, BMP_STARTUPTIME_REG_STRING, &dwStartupTime, FALSE))
	{
		RETAILMSG(1, (_T("RegOpenCreateDword(%s), Default(%d)\r\n"),
			BMP_STARTUPTIME_REG_STRING, BMP_STARTUPTIME_REG_DEFAULT));
		dwStartupTime = BMP_STARTUPTIME_REG_DEFAULT;
	}
	if (BMP_STARTUPTIME_REG_DEFAULT > dwStartupTime)
		dwStartupTime = BMP_STARTUPTIME_REG_DEFAULT;
	for (i=0, bLoop=TRUE; (TRUE==bLoop && i<50); i++)
	{
		dwTotal = GetTickCount() - dwStart;
		if (dwStartupTime < dwTotal)
		{
			if (NULL == FindWindow(_T("Dialog"), WIFI_CARDNAME_TCHAR))
			{
				RETAILMSG(1, (_T("\t BMP_STARTUPTIME : %d\r\n"), dwTotal));
				bLoop = FALSE;
			}
			else
				Sleep(100);
		}
		else
			Sleep(100);
	}
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
		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, APP_UPDATE_REG_STRING, szProgram, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
				APP_UPDATE_REG_STRING, APP_UPDATE_REG_DEFAULT));
			_tcscpy(szProgram, APP_UPDATE_REG_DEFAULT);
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
		else
		{
			RETAILMSG(1, (_T("ERROR : Not Found - %s\r\n"), szProgram));
		}
	}

	HDC hDC = GetDC(HWND_DESKTOP);
	if (TRUE == bDispUpdate)
	{
		BOOL bDirtyRect = (BOOL)ExtEscape(hDC, DRVESC_GET_DIRTYRECT, 0, NULL, 0, NULL);
		for (i=0; (FALSE==bDirtyRect && i<5); i++)
		{
			Sleep(1000);
			bDirtyRect = (BOOL)ExtEscape(hDC, DRVESC_GET_DIRTYRECT, 0, NULL, 0, NULL);
		}
		if (FALSE == bDirtyRect)
		{
			RETAILMSG(1, (_T("ERROR : FALSE == bDirtyRect\r\n")));
			bDispUpdate = FALSE;
		}
	}
	if (FALSE == bDispUpdate)
		DirtyRectUpdate(hDC);
	ReleaseDC(HWND_DESKTOP, hDC);

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

	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("PowerManager/ReloadActivityTimeouts"));
	if (hEvent)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
		RETAILMSG(0, (_T("SetEvent(PowerManager/ReloadActivityTimeouts)\r\n")));
	}

	return 0;
}

