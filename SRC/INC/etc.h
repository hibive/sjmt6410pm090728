
#ifndef __ETC_H__
#define __ETC_H__


#include <windows.h>


#define ETC_DRIVER_NAME		_T("ETC1:")
#define	ETC_DRIVER_DEVICE	0x8010
#define	ETC_DRIVER_FUNCTION	0x810

#ifndef	METHOD_NEITHER
#define METHOD_NEITHER		3
#endif
#ifndef	FILE_ANY_ACCESS
#define FILE_ANY_ACCESS		0
#endif
#ifndef	CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) (((DeviceType)<<16) | ((Access)<<14) | ((Function)<<2) | (Method))
#endif


#define IOCTL_SET_POWER_WLAN\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0x01, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_POWER_WLAN\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0x02, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SET_POWER_WCDMA\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0x03, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_POWER_WCDMA\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0x04, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_IS_ATTACH_UFN\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xD0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_GET_BOARD_REVISIOIN\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xE0, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_BOOTLOADER_BUILD_DATETIME\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xE1, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_WINCE_BUILD_DATETIME\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xE2, METHOD_NEITHER, FILE_ANY_ACCESS)
typedef struct {
	WORD	BS_wRevsionCode;
	WORD	BS_wProductCode;
	WORD	CMD_wType;				// (little-endian, 0x0000 or 'bs' -> 0x6273)
	BYTE	CMD_bMinor;
	BYTE	CMD_bMajor;
	DWORD	WFM_dwFileSize;
	DWORD	WFM_dwSerialNumber; 	// (little-endian)
	BYTE	WFM_bRunType;			// (0x00=[B]aseline, 0x01=[T]est/trial, 0x02=[P]roduction, 0x03=[Q]ualification)
	BYTE	WFM_bFPLPlatform;		// (0x00=2.0, 0x01=2.1, 0x02=2.3; 0x03=Vizplex 110; other values undefined)
	WORD	WFM_wFPLLot;			// (little-endian)
	BYTE	WFM_bModeVersion;		// (0x01 -> 0 INIT, 1 DU, 2 GC16, 3 GC4)
	BYTE	WFM_bWaveformVersion;	// (BCD)
	BYTE	WFM_bWaveformSubVersion;// (BCD)
	BYTE	WFM_bWaveformType;		// (0x0B=TE, 0x0E=WE; other values undefined)
	BYTE	WFM_bFPLSize;			// (0x32=5", 0x3C=6", 0x50=8", 0x61=9.7")
	BYTE	WFM_bMFGCode;			// (0x01=PVI, 0x02=LGD)
} EPSON_INFO, *PEPSON_INFO;
#define IOCTL_GET_EPSON_INFO\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xE3, METHOD_NEITHER, FILE_ANY_ACCESS)

// +++ Only System Used +++
#define IOCTL_LED_CHECK\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xFB, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_UPDATE_BOOTLOADER\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xFC, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_GET_BOARD_INFO\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xFD, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SET_BOARD_INFO\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xFE, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SET_BOARD_UUID\
	CTL_CODE(ETC_DRIVER_DEVICE, ETC_DRIVER_FUNCTION+0xFF, METHOD_NEITHER, FILE_ANY_ACCESS)
// --- Only System Used ---

#endif //__ETC_H__

