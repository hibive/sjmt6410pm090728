// EBook2Command.cpp : Defines the entry point for the console application.
//

#include <objbase.h>
#include <initguid.h>
#include <imaging.h>
//#pragma comment (lib, "Ole32.lib")
#include "s1d13521.h"


#define SJMT_REG_KEY				_T("Software\\SeojeonMediaTech")
#define IMAGE_SHUTDOWN_REG_STRING	_T("Image_Shutdown")
#define IMAGE_LOWBATTERY_REG_STRING	_T("Image_Lowbattery")


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
	bRet = drawImage(hDC, lpszFileName);
	ExtEscape(hDC, DRVESC_SET_DIRTYRECT, FALSE, NULL, 0, NULL);

	return bRet;
}

static BOOL displayBitmap(HDC hDC, LPCTSTR lpszFileName)
{
	HANDLE hFile = CreateFile(lpszFileName,
		GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return FALSE;

	IMAGEFILES ifs;
	ifs.nCount = (int)GetFileSize(hFile, NULL);
	ifs.x = ifs.y = 0;
	ifs.pBuffer = new BYTE [ifs.nCount];
	if (NULL == ifs.pBuffer)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	DWORD dwNumberOfBytesRead;
	ReadFile(hFile, ifs.pBuffer, ifs.nCount, &dwNumberOfBytesRead, NULL);
	if (ifs.nCount != dwNumberOfBytesRead)
	{
		CloseHandle(hFile);
		delete [] ifs.pBuffer;
		return FALSE;
	}

	int nRet = ExtEscape(hDC, DRVESC_DISPLAY_BITMAP, sizeof(IMAGEFILES), (LPCSTR)&ifs, 0, NULL);
	RETAILMSG(1, (_T("+ DRVESC_DISPLAY_BITMAP %d\r\n"), nRet));

	delete [] ifs.pBuffer;
	CloseHandle(hFile);

	return TRUE;
}
static BOOL displayShutDown(HDC hDC, LPCTSTR lpszFileName)
{
	ExtEscape(hDC, DRVESC_SET_DIRTYRECT, FALSE, NULL, 0, NULL);
	ExtEscape(hDC, DRVESC_SET_DSPUPDSTATE, DSPUPD_FULL, NULL, 0, NULL);
	ExtEscape(hDC, DRVESC_SET_WAVEFORMMODE, WAVEFORM_GC, NULL, 0, NULL);
	return displayBitmap(hDC, lpszFileName);
}








int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
	BOOL bRet = FALSE;
	HDC hDC = NULL;

	if (2 > argc)
		return 0;

	/*for (int i=1; i<argc; i++)
		RETAILMSG(1, (_T("EBook2Command => argv : %s\r\n"), argv[i]));*/

	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
		return FALSE;

	hDC = GetDC(HWND_DESKTOP);
	if (0 == _tcsnicmp(_T("SHUTDOWN"), argv[1], _tcslen(_T("SHUTDOWN"))))
	{
		TCHAR szShutdown[MAX_PATH] = {0,};
		if (FALSE == RegOpenCreateStr(SJMT_REG_KEY, IMAGE_SHUTDOWN_REG_STRING, szShutdown, MAX_PATH, FALSE))
		{
			LPCTSTR lpszDefShutdown = _T("\\Windows\\ebook2_shutdown.bmp");
			RETAILMSG(1, (_T("ERROR : RegOpenCreateStr() : %s\r\n"), IMAGE_SHUTDOWN_REG_STRING));
			_tcscpy_s(szShutdown, _countof(szShutdown), lpszDefShutdown);
		}
		bRet = displayShutDown(hDC, szShutdown);
		RETAILMSG(1, (_T("EBook2Command => SHUTDOWN(%d)\r\n"), bRet));
	}
	else if (0 == _tcsnicmp(_T("LOWBATTERY"), argv[1], _tcslen(_T("LOWBATTERY"))))
	{
		TCHAR szLowbattery[MAX_PATH] = {0,};
		if (FALSE == RegOpenCreateStr(SJMT_REG_KEY, IMAGE_LOWBATTERY_REG_STRING, szLowbattery, MAX_PATH, FALSE))
		{
			LPCTSTR lpszDefLowbattery = _T("\\Windows\\ebook2_lowbattery.bmp");
			RETAILMSG(1, (_T("ERROR : RegOpenCreateStr() : %s\r\n"), IMAGE_LOWBATTERY_REG_STRING));
			_tcscpy_s(szLowbattery, _countof(szLowbattery), lpszDefLowbattery);
		}
		bRet = displayShutDown(hDC, szLowbattery);
		RETAILMSG(1, (_T("EBook2Command => LOWBATTERY(%d)\r\n"), bRet));
	}
	else if (0 == _tcsnicmp(_T("SLEEP"), argv[1], _tcslen(_T("SLEEP"))))
	{
		ExtEscape(hDC, DRVESC_SYSTEM_SLEEP, FALSE, NULL, 0, NULL);
		RETAILMSG(1, (_T("EBook2Command => SLEEP\r\n")));
	}
	else if (0 == _tcsnicmp(_T("WAKEUP"), argv[1], _tcslen(_T("WAKEUP"))))
	{
		ExtEscape(hDC, DRVESC_SYSTEM_WAKEUP, TRUE, NULL, 0, NULL);
		RETAILMSG(1, (_T("EBook2Command => WAKEUP\r\n")));
	}
	else if (0 == _tcsnicmp(_T("DIRTYRECT"), argv[1], _tcslen(_T("DIRTYRECT"))))
	{
		BOOL bSet = -1;
		if (3 <= argc)
		{
			bSet = _ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_DIRTYRECT, bSet, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("EBook2Command => DIRTYRECT(%d)\r\n"), bSet));
	}
	else if (0 == _tcsnicmp(_T("DSPUPDSTATE"), argv[1], _tcslen(_T("DSPUPDSTATE"))))
	{
		DSPUPDSTATE dus = DSPUPD_LAST;
		if (3 <= argc)
		{
			dus = (DSPUPDSTATE)_ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_DSPUPDSTATE, dus, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("EBook2Command => DSPUPDSTATE(%d)\r\n"), dus));
	}
	else if (0 == _tcsnicmp(_T("BORDER"), argv[1], _tcslen(_T("BORDER"))))
	{
		BOOL bSet = -1;
		if (3 <= argc)
		{
			bSet = _ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_BORDER, bSet, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("EBook2Command => BORDER(%d)\r\n"), bSet));
	}
	else if (0 == _tcsnicmp(_T("WAVEFORMMODE"), argv[1], _tcslen(_T("WAVEFORMMODE"))))
	{
		WAVEFORMMODE wfm = WAVEFORM_LAST;
		if (3 <= argc)
		{
			wfm = (WAVEFORMMODE)_ttoi(argv[2]);
			ExtEscape(hDC, DRVESC_SET_WAVEFORMMODE, wfm, NULL, 0, NULL);
		}
		RETAILMSG(1, (_T("EBook2Command => WAVEFORMMODE(%d)\r\n"), wfm));
	}


	// ...
	ReleaseDC(HWND_DESKTOP, hDC);

	CoUninitialize();
	return 0;
}

// _T("\\Windows\\EBook2Command.exe SHUTDOWN");
// _T("\\Windows\\EBook2Command.exe LOWBATTERY");
// _T("\\Windows\\EBook2Command.exe SLEEP");
// _T("\\Windows\\EBook2Command.exe WAKEUP");
// _T("\\Windows\\EBook2Command.exe DIRTYRECT [0(off) or 1(on)]");
// _T("\\Windows\\EBook2Command.exe DSPUPDSTATE [0(full) or 1(part)]");
// _T("\\Windows\\EBook2Command.exe BORDER [0(off) or 1(on)]");
// _T("\\Windows\\EBook2Command.exe WAVEFORMMODE [0(init), 1(du), 2(gu), 3(gc), 4(autodugu)]");

