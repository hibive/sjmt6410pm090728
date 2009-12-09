//
// EBook2Startup.cpp
//

#include <windows.h>
#include "s1d13521.h"


#define OMNIBOOK_REG_KEY		_T("Software\\Omnibook")

#define STARTUP_REG_STRING		_T("AppStartup")
#define DEFAULT_STARTUP			_T("\\Omnibook Store\\Omnibook_MainApp.exe")
#define UPDATE_REG_STRING		_T("AppUpdate")
#define DEFAULT_UPDATE			_T("\\Storage Card\\Omnibook_UpdateApp.exe")
#define SIPSYMBOL_REG_STRING	_T("AppSipSymbol")
#define DEFAULT_SIPSYMBOL		_T("\\Windows\\Omnibook_SipSymbol.exe")


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

static BOOL RunProgram(LPCWSTR lpszImageName)
{
	PROCESS_INFORMATION pi = {0,};
	BOOL bResult = FALSE;

	bResult = CreateProcessW(lpszImageName,
		NULL,	// Command line.
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
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	return bResult;
}

int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPTSTR lpCmdLine,
				   int nCmdShow)
{
	TCHAR szProgram[MAX_PATH] = {0,};

	if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, STARTUP_REG_STRING, szProgram, MAX_PATH, FALSE))
	{
		RETAILMSG(1, (_T("ERROR : RegOpenCreateStr() : %s\r\n"), STARTUP_REG_STRING));
		_tcscpy(szProgram, DEFAULT_STARTUP);
	}
	if (IsProgram(szProgram))
	{
		RunProgram(szProgram);
		RETAILMSG(1, (_T("RunProgram : %s\r\n"), szProgram));
	}
	else
	{
		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, UPDATE_REG_STRING, szProgram, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("ERROR : RegOpenCreateStr() : %s\r\n"), UPDATE_REG_STRING));
			_tcscpy(szProgram, DEFAULT_UPDATE);
		}
		if (IsProgram(szProgram))
		{
			RunProgram(szProgram);
			RETAILMSG(1, (_T("RunProgram : %s\r\n"), szProgram));
		}
		else
		{
			HDC hDC = GetDC(HWND_DESKTOP);
			ExtEscape(hDC, DRVESC_SET_DIRTYRECT, TRUE, NULL, 0, NULL);
			ReleaseDC(HWND_DESKTOP, hDC);
			RETAILMSG(1, (_T("ERROR : Not Found - %s\r\n"), szProgram));
		}
	}

	if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, SIPSYMBOL_REG_STRING, szProgram, MAX_PATH, FALSE))
	{
		RETAILMSG(1, (_T("ERROR : RegOpenCreateStr() : %s\r\n"), SIPSYMBOL_REG_STRING));
		_tcscpy(szProgram, DEFAULT_SIPSYMBOL);
	}
	if (IsProgram(szProgram))
	{
		RunProgram(szProgram);
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

