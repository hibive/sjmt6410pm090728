
#include <windows.h>	// For all that Windows stuff
#include <usbfnioctl.h>
#include <devload.h>


#define	USB_FUN_DEV_NAME	_T("UFN1:")
#define	ARRAY_COUNT(x)		(sizeof(x) / sizeof(x[0]))


LPCTSTR	g_szUsbClassName[] = {
	_T("Serial_Class"),
	_T("Mass_Storage_Class"),
};


int ufnGetCurrentClientName(void)
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

BOOL ufnChangeCurrentClient(int nMode)
{
	HANDLE hUSBFn = INVALID_HANDLE_VALUE;
	UFN_CLIENT_INFO ufnInfo = {0,};
	DWORD dwBytes;

	if (ARRAY_COUNT(g_szUsbClassName) <= nMode)
		return FALSE;

	hUSBFn = CreateFile(USB_FUN_DEV_NAME, DEVACCESS_BUSNAMESPACE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hUSBFn)
	{
		_stprintf_s(ufnInfo.szName, UFN_CLIENT_NAME_MAX_CHARS, _T("%s"), g_szUsbClassName[nMode]);
		DeviceIoControl(hUSBFn,	IOCTL_UFN_CHANGE_CURRENT_CLIENT, ufnInfo.szName, sizeof(ufnInfo.szName), NULL, 0, &dwBytes, NULL);

		RETAILMSG(0, (_T("IOCTL_UFN_CHANGE_CURRENT_CLIENT\r\n")));
		RETAILMSG(0, (_T("    ClientName = %s\r\n"), ufnInfo.szName));
		
		CloseHandle(hUSBFn);

		return TRUE;
	}

	return FALSE;
}

int ufnGetCurrentMassStorage(void)
{
	HKEY hKey;
	TCHAR szDeviceName[16] = {0,};
	DWORD dwType = REG_SZ, cbData = sizeof(szDeviceName);
	int nType;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Drivers\\USB\\FunctionDrivers\\Mass_Storage_Class"), 0, KEY_ALL_ACCESS, &hKey))
		return -1;

	RegQueryValueEx(hKey, _T("DeviceName"), NULL, &dwType, (LPBYTE)&szDeviceName, &cbData);
	RETAILMSG(0, (_T("szDeviceName = %s\r\n"), szDeviceName));
	RegCloseKey(hKey);

	nType = szDeviceName[3] - _T('0');
	if (1 > nType || 3 < nType)	// 1 ~ 3
		return -2;

	return nType;
}

BOOL ufnChangeCurrentMassStorage(int nType)	// 1 ~ 3
{
	HKEY hKey;
	TCHAR szDeviceName[16] = {0,};
	DWORD dwType = REG_SZ, cbData = sizeof(szDeviceName);

	if (1 > nType || 3 < nType)	// 1 ~ 3
		return FALSE;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Drivers\\USB\\FunctionDrivers\\Mass_Storage_Class"), 0, KEY_ALL_ACCESS, &hKey))
		return FALSE;

	_stprintf_s(szDeviceName, 16, _T("DSK%d:"), nType);
	RETAILMSG(1, (_T("szDeviceName = %s\r\n"), szDeviceName));
	RegSetValueEx(hKey, _T("DeviceName"), NULL, dwType, (LPBYTE)&szDeviceName, cbData);
	RegCloseKey(hKey);

	return TRUE;
}

