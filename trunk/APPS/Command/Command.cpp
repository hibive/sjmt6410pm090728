// Command.cpp : Defines the entry point for the console application.
//

#include <objbase.h>
#include <initguid.h>
#include <imaging.h>
//#pragma comment (lib, "Ole32.lib")
#include "s1d13521.h"


#define OMNIBOOK_REG_KEY			_T("Software\\Omnibook")

#define BMP_SLEEP_REG_STRING		_T("BmpSleep")

#define BMP_SHUTDOWN_REG_STRING		_T("BmpShutdown")
#define BMP_SHUTDOWN_REG_DEFAULT	_T("\\Windows\\Omnibook_Shutdown.bmp")

#define BMP_LOWBATTERY_REG_STRING	_T("BmpLowbattery")
#define BMP_LOWBATTERY_REG_DEFAULT	_T("\\Windows\\Omnibook_Lowbattery.bmp")

#define BMP_BATCHARGING_REG_STRING	_T("BmpBatCharging")
#define BMP_BATCHARGING_REG_DEFAULT	_T("\\Windows\\Omnibook_BatCharging.bmp")

#define BMP_BATCOMPLETE_REG_STRING	_T("BmpBatComplete")
#define BMP_BATCOMPLETE_REG_DEFAULT	_T("\\Windows\\Omnibook_BatComplete.bmp")


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

#if	0
static BOOL drawImage(HDC hDC, LPCTSTR lpszFileName)
{
	IImagingFactory* pImageFactory = NULL;
	BOOL bRet = FALSE;

	if (SUCCEEDED(CoCreateInstance(CLSID_ImagingFactory,
		0, CLSCTX_INPROC_SERVER, IID_IImagingFactory, (void**)&pImageFactory)))
	{
		IImage* pImage = NULL;
		ImageInfo imageInfo;

		if (SUCCEEDED(pImageFactory->CreateImageFromFile(lpszFileName, &pImage))
			&& SUCCEEDED(pImage->GetImageInfo(&imageInfo)))
		{
			RECT rect = {0, 0, imageInfo.Width, imageInfo.Height};
			pImage->Draw(hDC, &rect, 0);
			pImage->Release();
			bRet = TRUE;
		}

		pImageFactory->Release();
	}

	return bRet;
}
static BOOL drawShutDown(HDC hDC, LPCTSTR lpszFileName)
{
	BOOL bRet = FALSE;

	ExtEscape(hDC, DRVESC_SET_DSPUPDSTATE, DSPUPD_FULL, NULL, 0, NULL);
	ExtEscape(hDC, DRVESC_SET_WAVEFORMMODE, WAVEFORM_GC, NULL, 0, NULL);
	bRet = drawImage(hDC, lpszFileName);
	ExtEscape(hDC, DRVESC_SET_DIRTYRECT, FALSE, NULL, 0, NULL);

	return bRet;
}
#endif
static BOOL dispBitmap(HDC hDC, LPCTSTR lpszFileName)
{
	HANDLE hFile = CreateFile(lpszFileName,
		GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return FALSE;

	DISPBITMAP dispBmp;
	dispBmp.nCount = (int)GetFileSize(hFile, NULL);
	dispBmp.x = dispBmp.y = 0;
	dispBmp.pBuffer = new BYTE [dispBmp.nCount];
	if (NULL == dispBmp.pBuffer)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	DWORD dwNumberOfBytesRead;
	ReadFile(hFile, dispBmp.pBuffer, dispBmp.nCount, &dwNumberOfBytesRead, NULL);
	if (dispBmp.nCount != dwNumberOfBytesRead)
	{
		CloseHandle(hFile);
		delete [] dispBmp.pBuffer;
		return FALSE;
	}

	dispBmp.pUpdate = NULL;
	dispBmp.bIsWait = TRUE;
	int nRet = ExtEscape(hDC, DRVESC_DISP_BITMAP, sizeof(dispBmp), (LPCSTR)&dispBmp, 0, NULL);
	RETAILMSG(0, (_T("+ DRVESC_DISP_BITMAP %d\r\n"), nRet));

	delete [] dispBmp.pBuffer;
	CloseHandle(hFile);

	return TRUE;
}
static BOOL dispShutdown(HDC hDC, LPCTSTR lpszFileName)
{
	ExtEscape(hDC, DRVESC_SET_DIRTYRECT, FALSE, NULL, 0, NULL);
	ExtEscape(hDC, DRVESC_SET_DSPUPDSTATE, DSPUPD_FULL, NULL, 0, NULL);
	ExtEscape(hDC, DRVESC_SET_WAVEFORMMODE, WAVEFORM_GC, NULL, 0, NULL);
	return dispBitmap(hDC, lpszFileName);
}








int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
	BOOL bRet = FALSE;
	HDC hDC = NULL;
	TCHAR szText[MAX_PATH] = {0,};

	if (2 > argc)
		return 0;

#if	0
	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
		return FALSE;
#endif

	hDC = GetDC(HWND_DESKTOP);
	if (0 == _tcsnicmp(_T("SLEEP"), argv[1], _tcslen(_T("SLEEP"))))
	{
		// +++
		ExtEscape(hDC, DRVESC_SYSTEM_SLEEP, 100, NULL, 0, NULL);
		// ---
		if (TRUE == RegOpenCreateStr(OMNIBOOK_REG_KEY, BMP_SLEEP_REG_STRING, szText, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("SLEEP : RegOpenCreateStr(%s, %s)\r\n"), BMP_SLEEP_REG_STRING, szText));
			bRet = dispShutdown(hDC, szText);
		}
		RETAILMSG(1, (_T("App_Command => SLEEP(%d)\r\n"), bRet));
	}
	else if (0 == _tcsnicmp(_T("SHUTDOWN"), argv[1], _tcslen(_T("SHUTDOWN"))))
	{
		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, BMP_SHUTDOWN_REG_STRING, szText, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
				BMP_SHUTDOWN_REG_STRING, BMP_SHUTDOWN_REG_DEFAULT));
			_tcscpy_s(szText, MAX_PATH, BMP_SHUTDOWN_REG_DEFAULT);
		}
		bRet = dispShutdown(hDC, szText);
		if (FALSE == bRet)
			dispShutdown(hDC, BMP_SHUTDOWN_REG_DEFAULT);
		RETAILMSG(1, (_T("App_Command => SHUTDOWN(%d)\r\n"), bRet));
	}
	else if (0 == _tcsnicmp(_T("LOWBATTERY"), argv[1], _tcslen(_T("LOWBATTERY"))))
	{
		// +++
		ExtEscape(hDC, DRVESC_SYSTEM_SLEEP, 100, NULL, 0, NULL);
		// ---
		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, BMP_LOWBATTERY_REG_STRING, szText, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
				BMP_LOWBATTERY_REG_STRING, BMP_LOWBATTERY_REG_DEFAULT));
			_tcscpy_s(szText, MAX_PATH, BMP_LOWBATTERY_REG_DEFAULT);
		}
		bRet = dispShutdown(hDC, szText);
		if (FALSE == bRet)
			dispShutdown(hDC, BMP_LOWBATTERY_REG_DEFAULT);
		RETAILMSG(1, (_T("App_Command => LOWBATTERY(%d)\r\n"), bRet));
	}
	else if (0 == _tcsnicmp(_T("BATCHARGING"), argv[1], _tcslen(_T("BATCHARGING"))))
	{
		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, BMP_BATCHARGING_REG_STRING, szText, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
				BMP_BATCHARGING_REG_STRING, BMP_BATCHARGING_REG_DEFAULT));
			_tcscpy_s(szText, MAX_PATH, BMP_BATCHARGING_REG_DEFAULT);
		}
		bRet = dispShutdown(hDC, szText);
		if (FALSE == bRet)
			dispShutdown(hDC, BMP_BATCHARGING_REG_DEFAULT);
		RETAILMSG(1, (_T("App_Command => BATCHARGING(%d)\r\n"), bRet));
	}
	else if (0 == _tcsnicmp(_T("BATCOMPLETE"), argv[1], _tcslen(_T("BATCOMPLETE"))))
	{
		if (FALSE == RegOpenCreateStr(OMNIBOOK_REG_KEY, BMP_BATCOMPLETE_REG_STRING, szText, MAX_PATH, FALSE))
		{
			RETAILMSG(1, (_T("RegOpenCreateStr(%s), Default(%s)\r\n"),
				BMP_BATCOMPLETE_REG_STRING, BMP_BATCOMPLETE_REG_DEFAULT));
			_tcscpy_s(szText, MAX_PATH, BMP_BATCOMPLETE_REG_DEFAULT);
		}
		bRet = dispShutdown(hDC, szText);
		if (FALSE == bRet)
			dispShutdown(hDC, BMP_BATCOMPLETE_REG_DEFAULT);
		RETAILMSG(1, (_T("App_Command => BATCOMPLETE(%d)\r\n"), bRet));
	}


	else if (0 == _tcsnicmp(_T("DIRTYRECT"), argv[1], _tcslen(_T("DIRTYRECT"))))
	{
		BOOL bSet = -1;
		if (3 <= argc)
		{
			bSet = _ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_DIRTYRECT, bSet, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("App_Command => DIRTYRECT(%d)\r\n"), bSet));
	}
	else if (0 == _tcsnicmp(_T("DSPUPDSTATE"), argv[1], _tcslen(_T("DSPUPDSTATE"))))
	{
		DSPUPDSTATE dus = DSPUPD_LAST;
		if (3 <= argc)
		{
			dus = (DSPUPDSTATE)_ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_DSPUPDSTATE, dus, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("App_Command => DSPUPDSTATE(%d)\r\n"), dus));
	}
	else if (0 == _tcsnicmp(_T("BORDER"), argv[1], _tcslen(_T("BORDER"))))
	{
		BOOL bSet = -1;
		if (3 <= argc)
		{
			bSet = _ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_BORDER, bSet, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("App_Command => BORDER(%d)\r\n"), bSet));
	}
	else if (0 == _tcsnicmp(_T("WAVEFORMMODE"), argv[1], _tcslen(_T("WAVEFORMMODE"))))
	{
		WAVEFORMMODE wfm = WAVEFORM_LAST;
		if (3 <= argc)
		{
			wfm = (WAVEFORMMODE)_ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_WAVEFORMMODE, wfm, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("App_Command => WAVEFORMMODE(%d)\r\n"), wfm));
	}

	// ...
	ReleaseDC(HWND_DESKTOP, hDC);

#if	0
	CoUninitialize();
#endif
	return 0;
}

// _T("\\Windows\\App_Command.exe SLEEP");
// _T("\\Windows\\App_Command.exe SHUTDOWN");
// _T("\\Windows\\App_Command.exe LOWBATTERY");

// _T("\\Windows\\App_Command.exe DIRTYRECT [0(off) or 1(on)]");
// _T("\\Windows\\App_Command.exe DSPUPDSTATE [0(full) or 1(part)]");
// _T("\\Windows\\App_Command.exe BORDER [0(off) or 1(on)]");
// _T("\\Windows\\App_Command.exe WAVEFORMMODE [0(init), 1(du), 2(gu), 3(gc), 4(autodugu)]");

