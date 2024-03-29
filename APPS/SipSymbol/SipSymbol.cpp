
#include <windows.h>	// For all that Windows stuff
#include "resource.h"
#include "SipSymbol.h"	// Program-specific stuff
#include "ETC.h"
#include "s1d13521.h"


//----------------------------------------------------------------------
// Global data
HINSTANCE	g_hInstance;	// Program instance handle
HWND		g_hWndMain = NULL;

HINSTANCE	g_hCoreDll = NULL;
HHOOK		g_hKeyHook = NULL;
SETWINDOWSHOOKEX	g_fnSetWindowsHookEx;
UNHOOKWINDOWSHOOKEX	g_fnUnhookWindowsHookEx;
CALLNEXTHOOKEX		g_fnCallNextHookEx;

#define CANDXSIZE		239
#define CANDYSIZE		30
#define	CANDPAGESIZE	9
const static RECT rcCandCli = { 0, 0, 239, 29 };
const static RECT rcLArrow = { 2, 4, 14, 25 }, rcRArrow = { 225, 4, 236, 25 };
const static RECT rcBtn[9] = {
	{  17, 4,  38, 25 }, {  40, 4,  61, 25 },
	{  63, 4,  84, 25 }, {  86, 4, 107, 25 },
	{ 109, 4, 130, 25 }, { 132, 4, 153, 25 },
	{ 155, 4, 176, 25 }, { 178, 4, 199, 25 },
	{ 201, 4, 222, 25 }
};
HBITMAP	g_hBMCand, g_hBMCandNum, g_hBMCandArr[2];

BOOL	g_bWifiOnOff = FALSE;
BOOL	g_bSysVolCtrl = FALSE;

static struct _CANDLIST {
	TCHAR	szKey[3];
	TCHAR	chVKey;
	BOOL	bShift;
} CandList[] = {
	{_T("!"),	_T('1'),		TRUE},
	{_T("@"),	_T('2'),		TRUE},
	{_T("#"),	_T('3'),		TRUE},
	{_T("$"),	_T('4'),		TRUE},
	{_T("%"),	_T('5'),		TRUE},
	{_T("^"),	_T('6'),		TRUE},
	{_T("&&"),	_T('7'),		TRUE},
	{_T("*"),	_T('8'),		TRUE},
	{_T("("),	_T('9'),		TRUE},

	{_T(")"),	_T('0'),		TRUE},
	{_T("-"),	VK_SUBTRACT,	FALSE},
	{_T("_"),	VK_HYPHEN,		TRUE},
	{_T("="),	VK_EQUAL,		FALSE},
	{_T("+"),	VK_EQUAL,		TRUE},
	{_T("["),	VK_LBRACKET,	FALSE},
	{_T("{"),	VK_LBRACKET,	TRUE},
	{_T("}"),	VK_RBRACKET,	TRUE},
	{_T("]"),	VK_RBRACKET,	FALSE},

	{_T("\\"),	VK_BACKSLASH,	FALSE},
	{_T("|"),	VK_BACKSLASH,	TRUE},
	{_T(";"),	VK_SEMICOLON,	FALSE},
	{_T(":"),	VK_SEMICOLON,	TRUE},
	{_T("\'"),	VK_APOSTROPHE,	FALSE},
	{_T("\""),	VK_APOSTROPHE,	TRUE},
	{_T("<"),	VK_COMMA,		TRUE},
	{_T(">"),	VK_COMMA,		FALSE},
	{_T("?"),	VK_SLASH,		TRUE},

	{_T("`"),	VK_BACKQUOTE,	FALSE},
	{_T("~"),	VK_BACKQUOTE,	TRUE},
	{_T(","),	VK_PERIOD,		TRUE},
};
const static DWORD CANDLISTCOUNT = dim(CandList);
static DWORD g_dwCLSelect;


UINT g_uMsgUfn = RegisterWindowMessage(_T("OMNIBOOK_MESSAGE_UFN"));
UINT g_uMsgBatState = RegisterWindowMessage(_T("OMNIBOOK_MESSAGE_BATSTATE"));
int g_nModeUfn = -1;
BOOL g_bIsAttachUfn = FALSE;

extern int ufnGetCurrentClientName(void);
extern BOOL ufnChangeCurrentClient(int nMode);
extern int ufnGetCurrentMassStorage(void);
extern BOOL ufnChangeCurrentMassStorage(int nType);


//======================================================================
// Program entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	int rc = 0;
	HWND hwndMain;

	if (IsAlreadyExist(lpCmdLine))
		return 0x01;

	// Initialize this instance.
	hwndMain = InitInstance(hInstance, lpCmdLine, nCmdShow);
	if (hwndMain == 0)
		return 0x10;

	g_nModeUfn = ufnGetCurrentClientName();

	// Application message loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Instance cleanup
	return TermInstance(hInstance, msg.wParam);
}

BOOL IsAlreadyExist(LPWSTR lpCmdLine)
{
	HWND hWnd = FindWindow(CLASS_NAME, NULL);
	if (hWnd)
	{
		if (0 == wcsncmp(lpCmdLine, L"EXIT", 4))
		{
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		else
		{
			SetForegroundWindow(hWnd);
		}
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------
// InitInstance - Instance initialization
HWND InitInstance(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wc;
	HWND hWnd;

	// Save program instance handle in global variable.
	g_hInstance = hInstance;

	g_hCoreDll = LoadLibrary(_T("coredll.dll"));
	if (NULL == g_hCoreDll)
		return 0;
	g_fnSetWindowsHookEx = (SETWINDOWSHOOKEX)GetProcAddress(g_hCoreDll, L"SetWindowsHookExW");
	g_fnUnhookWindowsHookEx = (UNHOOKWINDOWSHOOKEX)GetProcAddress(g_hCoreDll, L"UnhookWindowsHookEx");
	g_fnCallNextHookEx = (CALLNEXTHOOKEX)GetProcAddress(g_hCoreDll, L"CallNextHookEx");

	g_hKeyHook = g_fnSetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyHookProc, hInstance, 0);

	// Register application main window class.
	wc.style = CS_VREDRAW | CS_HREDRAW | CS_IME;//0;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//0
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASS_NAME;
	if (RegisterClass(&wc) == 0)
		return 0;

	// Create main window.
	hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		CLASS_NAME,
		_T("SipSymbol"),
		WS_POPUP,
		600-CANDXSIZE, 800-CANDYSIZE, CANDXSIZE, CANDYSIZE,
		NULL,
		NULL,
		hInstance,
		NULL);
	if (!IsWindow(hWnd))
		return 0;

	// Standard show and update calls
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	return hWnd;
}

//----------------------------------------------------------------------
// TermInstance - Program cleanup
int TermInstance(HINSTANCE hInstance, int nDefRC)
{
	if (g_hKeyHook)
	{
		g_fnUnhookWindowsHookEx(g_hKeyHook);
		g_hKeyHook = NULL;
	}

	if (g_hCoreDll)
	{
		FreeLibrary(g_hCoreDll);
		g_hCoreDll = NULL;
	}

	return nDefRC;
}

void DrawBitmap(HDC hDC, long xStart, long yStart, HBITMAP hBitmap)
{
	HDC     hMemDC;
	HBITMAP hBMOld;
	BITMAP  bm;
	POINT   pt;

	hMemDC = CreateCompatibleDC(hDC);
	hBMOld = (HBITMAP) SelectObject(hMemDC, hBitmap);
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);

	pt.x = bm.bmWidth;
	pt.y = bm.bmHeight;
	BitBlt(hDC, xStart, yStart, pt.x, pt.y, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hMemDC, hBMOld);
	DeleteDC(hMemDC);

	return;
}

BOOL CheckOmnibookRegistry(LPCTSTR lpValueName)
{
	BOOL bRet = FALSE;
	HKEY hKey;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Omnibook"), 0, KEY_ALL_ACCESS, &hKey))
	{
		DWORD dwType = REG_DWORD, cbData = sizeof(bRet);
		RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE)&bRet, &cbData);

		RegCloseKey(hKey);
	}

	return bRet;
}

//======================================================================
// Message handling procedures for main window
//----------------------------------------------------------------------
LRESULT DoUfnMain(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	// wParam : UFN_DETACH(0), UFN_ATTACH(1)
	// lParam : ...
	RETAILMSG(0, (_T("DoUfnMain(%d, %d)\r\n"), wParam, lParam));

	if (0 == g_nModeUfn)		// Serial_Class
	{
		if (0 == wParam)		// UFN_DETACH
			g_bIsAttachUfn = FALSE;
		else if (1 == wParam)	// UFN_ATTACH
			g_bIsAttachUfn = TRUE;
	}
	else if (1 == g_nModeUfn)	// Mass_Storage_Class
	{
		if (0 == wParam)		// UFN_DETACH
		{
			if (TRUE == g_bIsAttachUfn)
			{
				HDC hDC = GetDC(HWND_DESKTOP);

				ExtEscape(hDC, DRVESC_SET_DSPUPDSTATE, DSPUPD_PART, NULL, 0, NULL);
				ExtEscape(hDC, DRVESC_SET_WAVEFORMMODE, WAVEFORM_GC, NULL, 0, NULL);
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

				ReleaseDC(HWND_DESKTOP, hDC);
			}

			g_bIsAttachUfn = FALSE;
		}
		else if (1 == wParam)	// UFN_ATTACH
		{
			g_bIsAttachUfn = TRUE;

			SYSTEM_POWER_STATUS_EX2	sps = {0,};
			GetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);
			BYTE fAcOn = !((1<<0) & sps.BatteryAverageCurrent);
			BYTE fChgDone = !((1<<2) & sps.BatteryAverageCurrent);

			//if (fAcOn)
			{
				TCHAR szCmdLine[MAX_PATH] = {0,};
				_stprintf_s(szCmdLine, MAX_PATH, fChgDone ? _T("BATCOMPLETE") : _T("BATCHARGING"));

				LPCTSTR lpszPathName = _T("\\Windows\\Omnibook_Command.exe");
				PROCESS_INFORMATION pi;
				ZeroMemory(&pi, sizeof(pi));
				if (CreateProcess(lpszPathName, szCmdLine, 0, 0, 0, 0, 0, 0, 0, &pi))
				{
					WaitForSingleObject(pi.hThread, 5000);
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
				}
			}
		}
	}

	return 0;
}
LRESULT DoBatStateMain(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	BYTE fBatteryState = (BYTE)wParam;
	BYTE fAcOn = !((1<<0) & fBatteryState);
	BYTE fChgDone = !((1<<2) & fBatteryState);
	RETAILMSG(0, (_T("DoBatStateMain(%d, %d, %d)\r\n"), fAcOn, fChgDone, g_bIsAttachUfn));

	if (0 == g_nModeUfn)		// Serial_Class
	{
	}
	else if (1 == g_nModeUfn)	// Mass_Storage_Class
	{
		if (g_bIsAttachUfn && fAcOn)
		{
			TCHAR szCmdLine[MAX_PATH] = {0,};
			_stprintf_s(szCmdLine, MAX_PATH, fChgDone ? _T("BATCOMPLETE") : _T("BATCHARGING"));

			LPCTSTR lpszPathName = _T("\\Windows\\Omnibook_Command.exe");
			PROCESS_INFORMATION pi;
			ZeroMemory(&pi, sizeof(pi));
			if (CreateProcess(lpszPathName, szCmdLine, 0, 0, 0, 0, 0, 0, 0, &pi))
			{
				WaitForSingleObject(pi.hThread, 5000);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
		}
	}

	return 0;
}
LRESULT DoPaintMain(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	DWORD iLoop, iStart;
	int iSaveBkMode;
	RECT rect;

	hDC = BeginPaint(hWnd, &ps);
	if (1)
	{
		DrawBitmap(hDC, 0, 0, g_hBMCand);
		iSaveBkMode = SetBkMode(hDC, TRANSPARENT);
		iStart = (g_dwCLSelect / CANDPAGESIZE) * CANDPAGESIZE;
		for (iLoop=0; iLoop<CANDPAGESIZE && iStart+iLoop<CANDLISTCOUNT; iLoop++)
		{
			rect.left = rcBtn[iLoop].left + 2;
			rect.right = rcBtn[iLoop].right + 2;
			rect.top = rcBtn[iLoop].top;
			rect.bottom = rcBtn[iLoop].bottom;
			DrawText(hDC, CandList[iStart + iLoop].szKey,
				(_T('&') == CandList[iStart + iLoop].szKey[0]) ? 2 : 1,
				&rect, DT_CENTER | DT_VCENTER);
		}
		SetBkMode(hDC, iSaveBkMode);
		for (; iLoop < 9; iLoop++)
			DrawBitmap(hDC, rcBtn[iLoop].left + 1, rcBtn[iLoop].top + 6, g_hBMCandNum);
		if (iStart)
			DrawBitmap(hDC, 6, 8, g_hBMCandArr[0]);
		if (iStart + 9 < CANDLISTCOUNT)
			DrawBitmap(hDC, 228, 8, g_hBMCandArr[1]);
	}
	EndPaint(hWnd, &ps);
	return 0;
}
LRESULT DoLbuttonDown(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	DestroyWindow(hWnd);
	return 0;
}
LRESULT DoDestroy(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	DeleteObject(g_hBMCand);
	DeleteObject(g_hBMCandNum);
	DeleteObject(g_hBMCandArr[0]);
	DeleteObject(g_hBMCandArr[1]);
	PostQuitMessage(0);
	return 0;
}
LRESULT DoCreate(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
	g_hWndMain = hWnd;
	g_hBMCand       = LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_CANDIDATE));
	g_hBMCandNum    = LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_CANDNUMBER));
	g_hBMCandArr[0] = LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_CANDARROW1));
	g_hBMCandArr[1] = LoadBitmap(lpcs->hInstance, MAKEINTRESOURCE(IDB_CANDARROW2));
	g_bWifiOnOff	= CheckOmnibookRegistry(_T("CfgWifiOnOff"));
	g_bSysVolCtrl	= CheckOmnibookRegistry(_T("CfgSysVolCtrl"));
	RETAILMSG(1, (_T("g_bWifiOnOff(%d), g_bSysVolCtrl(%d)\r\n"), g_bWifiOnOff, g_bSysVolCtrl));
	return 0;
}
//----------------------------------------------------------------------
// MainWndProc - Callback function for application window
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	// Message dispatch table for MainWindowProc
	const struct decodeUINT MainMessages[] = {
		g_uMsgUfn,			DoUfnMain,
		g_uMsgBatState,		DoBatStateMain,
		WM_PAINT,			DoPaintMain,
		WM_LBUTTONDOWN,		DoLbuttonDown,
		WM_DESTROY,			DoDestroy,
		WM_CREATE,			DoCreate,
	};

	// Search message list to see if we need to handle this message.  If in list, call procedure.
	for (INT i=0; i<dim(MainMessages); i++)
	{
		if (wMsg == MainMessages[i].Code)
			return (*MainMessages[i].Fxn)(hWnd, wMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}


LRESULT CALLBACK KeyHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static BOOL static_bIsNumSkip = FALSE;

	if (g_bIsAttachUfn && (1 == g_nModeUfn))	// Mass_Storage_Class
	{
		RETAILMSG(0, (_T("KeyHookProc() : g_bIsAttachUfn = %d\r\n"), g_bIsAttachUfn));
		return 1;
	}

	if (HC_ACTION == nCode && (WM_KEYDOWN == wParam || WM_KEYUP == wParam))
	{
		PKBDLLHOOKSTRUCT key = (PKBDLLHOOKSTRUCT)lParam;
		BOOL bIsKeyUp = (WM_KEYUP == wParam) ? TRUE : FALSE;
		BOOL bIsVisible = IsWindowVisible(g_hWndMain);
		BOOL bIsRet = TRUE;

		if (VK_F21 == key->vkCode)	// SYM
		{
			if (bIsKeyUp)
			{
				g_dwCLSelect = 0;
				ShowWindow(g_hWndMain, bIsVisible ? SW_HIDE : SW_SHOWNOACTIVATE);
			}
		}
		else if (VK_F22 == key->vkCode)	// Test Program...
		{
			if (bIsKeyUp)
			{
				LPCTSTR lpszPathName = _T("\\Storage Card\\testOmnibook\\testOmnibook.exe");
				PROCESS_INFORMATION pi;
				ZeroMemory(&pi, sizeof(pi));
				if (CreateProcess(lpszPathName, NULL, 0, 0, 0, 0, 0, 0, 0, &pi))
				{
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
				}
			}
			bIsRet = FALSE;
		}
		else if (g_bWifiOnOff
			&& VK_BROWSER_REFRESH == key->vkCode)
		{
			if (bIsKeyUp)
			{
				HANDLE hEtc = CreateFile(ETC_DRIVER_NAME, GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,	NULL, OPEN_EXISTING, 0, 0);
				if (INVALID_HANDLE_VALUE != hEtc)
				{
					BOOL bIsWifi = DeviceIoControl(hEtc, IOCTL_GET_POWER_WLAN, NULL, 0, NULL, 0, NULL, NULL);
					RETAILMSG(0, (_T("IOCTL_GET_POWER_WLAN = %d\r\n"), bIsWifi));
					bIsWifi = DeviceIoControl(hEtc, IOCTL_SET_POWER_WLAN, NULL, !bIsWifi, NULL, 0, NULL, NULL);
					RETAILMSG(0, (_T("IOCTL_SET_POWER_WLAN = %d\r\n"), bIsWifi));
					CloseHandle(hEtc);
				}
			}
		}
		else if (g_bSysVolCtrl
			&& (VK_BROWSER_BACK == key->vkCode || VK_BROWSER_FORWARD == key->vkCode))	// System Volume Down or Up
		{
			if (bIsKeyUp)
			{
				DWORD dwVolume;
				int nSoundLevel;

				waveOutGetVolume(NULL, &dwVolume);
				nSoundLevel = dwVolume / 0x11111111;
				RETAILMSG(0, (_T("waveOutGetVolume = %d, %X\r\n"), nSoundLevel, dwVolume));
				if (VK_BROWSER_BACK == key->vkCode)
					nSoundLevel--;
				else //if (VK_BROWSER_FORWARD == key->vkCode)
					nSoundLevel++;

				if (15 < nSoundLevel)
					nSoundLevel = 15;
				else if (0 > nSoundLevel)
					nSoundLevel = 0;
				dwVolume = (0x11111111 * nSoundLevel);
				waveOutSetVolume(NULL, dwVolume);
				RETAILMSG(0, (_T("waveOutSetVolume = %d, %X\r\n"), nSoundLevel, dwVolume));
			}
		}
		else if (bIsVisible)
		{
			if (VK_LEFT == key->vkCode)
			{
				if (bIsKeyUp && (CANDPAGESIZE-1) < g_dwCLSelect)
				{
					g_dwCLSelect -= CANDPAGESIZE;
					InvalidateRect(g_hWndMain, NULL, FALSE);
				}
			}
			else if (VK_RIGHT == key->vkCode)
			{
				if (bIsKeyUp && (CANDLISTCOUNT-CANDPAGESIZE) > g_dwCLSelect)
				{
					g_dwCLSelect += CANDPAGESIZE;
					InvalidateRect(g_hWndMain, NULL, FALSE);
				}
			}
			else if (_T('1') <= key->vkCode && _T('9') >= key->vkCode)
			{
				if (TRUE == static_bIsNumSkip)
				{
					if (bIsKeyUp)
						static_bIsNumSkip = FALSE;
					return g_fnCallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
				}

				if (bIsKeyUp)
				{
					int nIdx = key->vkCode - _T('1') + g_dwCLSelect;
					if (_T('1') <= CandList[nIdx].chVKey && _T('9') >= CandList[nIdx].chVKey)
						static_bIsNumSkip = TRUE;

					if (CandList[nIdx].bShift)
						keybd_event(VK_SHIFT, 0, 0, 0);
					keybd_event((BYTE)CandList[nIdx].chVKey, 0, 0, 0);
					keybd_event((BYTE)CandList[nIdx].chVKey, 0, KEYEVENTF_KEYUP, 0);
					if (CandList[nIdx].bShift)
						keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);

					//ShowWindow(g_hWndMain, SW_HIDE);
					RETAILMSG(0, (_T("CandList - szKey(%s), chVKey(0x%X), bShift(%d)\r\n"),
						CandList[nIdx].szKey, CandList[nIdx].chVKey, CandList[nIdx].bShift));
				}
			}
			else if (VK_PROCESSKEY == key->vkCode)	// ???
			{
				bIsRet = FALSE;
			}
			else
			{
				if (bIsKeyUp)
				{
					ShowWindow(g_hWndMain, SW_HIDE);

					if (_T('U') == key->vkCode)
					{
						g_nModeUfn = ufnGetCurrentClientName();
						if (0 == g_nModeUfn)		// Serial_Class
						{
							g_nModeUfn = 1;			// Mass_Storage_Class
							ufnChangeCurrentClient(g_nModeUfn);
							RETAILMSG(1, (_T("USB Mode Change : <<< Mass_Storage_Class >>>\r\n")));
						}
						else if (1 == g_nModeUfn)	// Mass_Storage_Class
						{
							g_nModeUfn = 0;			// Serial_Class
							ufnChangeCurrentClient(g_nModeUfn);
							RETAILMSG(1, (_T("USB Mode Change : <<< Serial_Class >>>\r\n")));
						}
						else
						{
							// ...
						}
					}
				}
				bIsRet = FALSE;
			}
		}
		else
			bIsRet = FALSE;

		RETAILMSG(0, (_T("KeyHookProc => message(0x%x), vkey(0x%x)\r\n"), wParam, key->vkCode));
		if (bIsRet)
			return 1;
	}

	return g_fnCallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
}

