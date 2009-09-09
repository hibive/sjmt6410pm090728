//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include <windows.h>
#include <bsp.h>
#include <ethdbg.h>
#include <fmd.h>
#include <pcireg.h>
#include <oal_blserial.h>
#include <usbdbgser.h>
#include <usbdbgrndis.h>
#include "loader.h"
#include "usb.h"
#include "fmd_lb.h"
#include "fmd_sb.h"

#include "s3c6410_ldi.h"
#include "s3c6410_display_con.h"
#include "InitialImage_rgb16_320x240.h"


// For USB Download function
extern BOOL InitializeUSB();
extern void InitializeInterrupt();
extern BOOL UbootReadData(DWORD cbData, LPBYTE pbData);
extern void OTGDEV_SetSoftDisconnect();
static void InitializeOTGCLK(void);
// To change Voltage for higher clock.
extern void LTC3714_Init();
extern void LTC3714_VoltageSet(UINT32,UINT32,UINT32);

// For Ethernet Download function.
char *inet_ntoa(DWORD dwIP);
DWORD inet_addr( char *pszDottedD );
BOOL EbootInitEtherTransport(EDBG_ADDR *pEdbgAddr, LPDWORD pdwSubnetMask,
                                    BOOL *pfJumpImg,
                                    DWORD *pdwDHCPLeaseTime,
                                    UCHAR VersionMajor, UCHAR VersionMinor,
                                    char *szPlatformString, char *szDeviceName,
                                    UCHAR CPUId, DWORD dwBootFlags);
BOOL EbootEtherReadData(DWORD cbData, LPBYTE pbData);
EDBG_OS_CONFIG_DATA *EbootWaitForHostConnect (EDBG_ADDR *pDevAddr, EDBG_ADDR *pHostAddr);
void SaveEthernetAddress();

// Eboot Internal static function
static void InitializeDisplay(void);
static void InitializeRTC(void);
static void SpinForever(void);
static USHORT GetIPString(char *);
static USHORT InputNumericalString(CHAR *szCount, UINT32 length);

// Globals
//
DWORD               g_ImageType;
DWORD               g_dwMinImageStart;  // For MultiBin, we will use lowest regions as XIPKERNEL
MultiBINInfo        g_BINRegionInfo;
PBOOT_CFG           g_pBootCfg;
UCHAR               g_TOC[LB_PAGE_SIZE];
const PTOC          g_pTOC = (PTOC)&g_TOC;
DWORD               g_dwTocEntry;
BOOL                g_bBootMediaExist = FALSE;
BOOL                g_bDownloadImage  = TRUE;
BOOL                g_bWaitForConnect = TRUE;
BOOL *              g_bCleanBootFlag;
BOOL *              g_bHiveCleanFlag;
BOOL *              g_bFormatPartitionFlag;
DWORD               g_DefaultBootDevice = BOOT_DEVICE_USB_DNW;

//for KITL Configuration Of Args
OAL_KITL_ARGS       *g_KITLConfig;

//for Device ID Of Args
UCHAR               *g_DevID;


EDBG_ADDR         g_DeviceAddr; // NOTE: global used so it remains in scope throughout download process
                        // since eboot library code keeps a global pointer to the variable provided.

DWORD            wNUM_BLOCKS;

void main(void)
{
    //GPIOTest_Function();

    OTGDEV_SetSoftDisconnect();

    BootloaderMain();

    // Should never get here.
    //
    SpinForever();
}
static void Delay(UINT32 count)
{
    volatile int i, j = 0;
    volatile static int loop = S3C6410_ACLK/100000;
    
    for(;count > 0;count--)
        for(i=0;i < loop; i++) { j++; }
}

static USHORT GetIPString(char *szDottedD)
{
    USHORT InChar = 0;
    USHORT cwNumChars = 0;

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            // If it's a number or a period, add it to the string.
            //
            if (InChar == '.' || (InChar >= '0' && InChar <= '9'))
            {
                if (cwNumChars < 16)        // IP string cannot over 15.  xxx.xxx.xxx.xxx
                {
                    szDottedD[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8)
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }

    return cwNumChars;

}

/*
    @func   void | SetIP | Accepts IP address from user input.
    @rdesc  N/A.
    @comm
    @xref
*/

static void SetIP(PBOOT_CFG pBootCfg)
{
    CHAR   szDottedD[16];   // The string used to collect the dotted decimal IP address.
    USHORT cwNumChars = 0;
    //USHORT InChar = 0;

    EdbgOutputDebugString("\r\nEnter new IP address: ");

    cwNumChars = GetIPString(szDottedD);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars && cwNumChars < 16)
    {
        szDottedD[cwNumChars] = '\0';
        pBootCfg->EdbgAddr.dwIP = inet_addr(szDottedD);
    }
    else
    {
        EdbgOutputDebugString("\r\nIncorrect String");
    }
}

/*
    @func   void | SetMask | Accepts subnet mask from user input.
    @rdesc  N/A.
    @comm
    @xref
*/
static void SetMask(PBOOT_CFG pBootCfg)
{
    CHAR szDottedD[16]; // The string used to collect the dotted masks.
    USHORT cwNumChars = 0;
    //USHORT InChar = 0;

    EdbgOutputDebugString("\r\nEnter new subnet mask: ");

    cwNumChars = GetIPString(szDottedD);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars && cwNumChars < 16)
    {
        szDottedD[cwNumChars] = '\0';
        pBootCfg->SubnetMask = inet_addr(szDottedD);
    }
    else
    {
        EdbgOutputDebugString("\r\nIncorrect String");
    }
    
}


/*
    @func   void | SetDelay | Accepts an autoboot delay value from user input.
    @rdesc  N/A.
    @comm
    @xref
*/
static void SetDelay(PBOOT_CFG pBootCfg)
{
    CHAR szCount[16];
    USHORT cwNumChars = 0;
    USHORT InChar = 0;

    EdbgOutputDebugString("\r\nEnter maximum number of seconds to delay [1-255]: ");

    cwNumChars = InputNumericalString(szCount, 16);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        pBootCfg->BootDelay = atoi(szCount);
        if (pBootCfg->BootDelay > 255)
        {
            pBootCfg->BootDelay = 255;
        }
        else if (pBootCfg->BootDelay < 1)
        {
            pBootCfg->BootDelay = 1;
        }
    }
}


static ULONG mystrtoul(PUCHAR pStr, UCHAR nBase)
{
    UCHAR nPos=0;
    BYTE c;
    ULONG nVal = 0;
    UCHAR nCnt=0;
    ULONG n=0;

    // fulllibc doesn't implement isctype or iswctype, which are needed by
    // strtoul, rather than including coredll code, here's our own simple strtoul.

    if (pStr == NULL)
        return(0);

    for (nPos=0 ; nPos < strlen(pStr) ; nPos++)
    {
        c = tolower(*(pStr + strlen(pStr) - 1 - nPos));
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
        {
            c -= 'a';
            c  = (0xa + c);
        }

        for (nCnt = 0, n = 1 ; nCnt < nPos ; nCnt++)
        {
            n *= nBase;
        }
        nVal += (n * c);
    }

    return(nVal);
}


static void CvtMAC(USHORT MacAddr[3], char *pszDottedD )
{
    DWORD cBytes;
    char *pszLastNum;
    int atoi (const char *s);
    int i=0;
    BYTE *p = (BYTE *)MacAddr;

    // Replace the dots with NULL terminators
    pszLastNum = pszDottedD;
    for(cBytes = 0 ; cBytes < 6 ; cBytes++)
    {
        while(*pszDottedD != '.' && *pszDottedD != '\0')
        {
            pszDottedD++;
        }
        if (pszDottedD == '\0' && cBytes != 5)
        {
            // zero out the rest of MAC address
            while(i++ < 6)
            {
                *p++ = 0;
            }
            break;
        }
        *pszDottedD = '\0';
        *p++ = (BYTE)(mystrtoul(pszLastNum, 16) & 0xFF);
        i++;
        pszLastNum = ++pszDottedD;
    }
}


static void SetCS8900MACAddress(PBOOT_CFG pBootCfg)
{
    CHAR szDottedD[24];
    USHORT cwNumChars = 0;
    USHORT InChar = 0;

    memset(szDottedD, '0', 24);

    EdbgOutputDebugString ( "\r\nEnter new MAC address in hexadecimal (hh.hh.hh.hh.hh.hh): ");

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        InChar = tolower(InChar);
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            // If it's a hex number or a period, add it to the string.
            //
            if (InChar == '.' || (InChar >= '0' && InChar <= '9') || (InChar >= 'a' && InChar <= 'f'))
            {
                if (cwNumChars < 17)
                {
                    szDottedD[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            else if (InChar == 8)       // If it's a backspace, back up.
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }

    EdbgOutputDebugString ( "\r\n");

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szDottedD[cwNumChars] = '\0';
        CvtMAC(pBootCfg->EdbgAddr.wMAC, szDottedD);

        EdbgOutputDebugString("INFO: MAC address set to: %x:%x:%x:%x:%x:%x\r\n",
                  pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, pBootCfg->EdbgAddr.wMAC[0] >> 8,
                  pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, pBootCfg->EdbgAddr.wMAC[1] >> 8,
                  pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, pBootCfg->EdbgAddr.wMAC[2] >> 8);
    }
    else
    {
        EdbgOutputDebugString("WARNING: SetCS8900MACAddress: Invalid MAC address.\r\n");
    }
}

static void MarkReservedBlockWithBadBlock()
{
    DWORD i;
    SectorInfo si;  
    DWORD StartSector;

    // to keep bootpart off of our reserved blocks we must mark it as bad, reserved & read-only
    si.bOEMReserved = ~(OEM_BLOCK_RESERVED | OEM_BLOCK_READONLY);
    si.bBadBlock    = BADBLOCKMARK;
    //                si.bBadBlock    = 0xff;
    si.dwReserved1  = 0xffffffff;
    si.wReserved2   = 0xffff;

    EdbgOutputDebugString("Reserving Blocks [0x%x - 0x%x] ... Wait for completion\r\n", 0, IMAGE_START_BLOCK-1);

#ifdef _IROMBOOT_
    StartSector = (IS_LB)? 64: 32;
#else
    StartSector = 0;
#endif  // !_IROMBOOT_

    for (i = StartSector; i < IMAGE_START_SECTOR; i++) {
        FMD_WriteSector(i, NULL, &si, 1);
    }
    EdbgOutputDebugString("Reserving is completed.\r\n");
}

static void EraseNANDBlockRegion(UINT32 StartBlock, UINT32 EndBlock, const char*RegionName)
{
    UINT32 i;
    EdbgOutputDebugString ("NAND Blocks Erasing for %s [0x%x - 0x%x] ... Wait for completion\r\n", RegionName, StartBlock, EndBlock-1);
    for (i = StartBlock; i < EndBlock; i++) {
        FMD_EraseBlock(i);
    }
    EdbgOutputDebugString ("Erasing is completed.\r\n");
}


static USHORT InputNumericalString(CHAR *szCount, UINT32 length)
{
    USHORT cwNumChars = 0;
    USHORT InChar = 0;
    
    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA) 
        {
            // If it's a number or a period, add it to the string.
            //
            if ((InChar >= '0' && InChar <= '9')) 
            {
                if (cwNumChars < length) 
                {
                    szCount[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8) 
            {
                if (cwNumChars > 0) 
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }
 
    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars) 
    {
        szCount[cwNumChars] = '\0';
    }
    else
    {
        szCount[0] = '\0';
    }
    return cwNumChars;
}

INT32 SetBlockPage(void)
{
    CHAR szCount[16];
    USHORT cwNumChars = 0;
    USHORT InChar = 0;
    UINT32 block=0, page=0;

    EdbgOutputDebugString("\r\nPress only Enter-key to return to menu\r\n");
    EdbgOutputDebugString("Enter Block # : ");

    cwNumChars = InputNumericalString(szCount, 16);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars) 
    {
        block = atoi(szCount);
    }
    else
    {
        return -1;
    }
 
    EdbgOutputDebugString("\r\nEnter Page # : ");

    memset(szCount, 0, sizeof(szCount));
    cwNumChars = InputNumericalString(szCount, 16);

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars) 
    {
        page = atoi(szCount);
    }
    else
    {
        return -1;
    }
  
    if (IS_LB) return ((block<<6)|page);
    else       return ((block<<5)|page);
}
 
void PrintPageData(DWORD nMData, DWORD nSData, UINT8* pMBuf, UINT8* pSBuf)
{
    DWORD i;
 
    EdbgOutputDebugString ("=========================================================");
    for (i=0;i<nMData;i++)
    {
        if ((i%16)==0)
        {
            OALMSG(TRUE,(TEXT("\r\n 0x%03x |"), i));
        }
        OALMSG(TRUE, (TEXT(" %02x"), pMBuf[i]));
        if ((i%512)==511)
        {
            EdbgOutputDebugString ("\r\n ------------------------------------------------------- ");
        }
    }
    for (i=0;i<nSData;i++)
    {
        if ((i%16)==0)
            OALMSG(TRUE, (TEXT("\r\n 0x%03x |"), i));
        OALMSG(TRUE, (TEXT(" %02x"), pSBuf[i]));
    }
    EdbgOutputDebugString ("\r\n=========================================================");
}

char* BootDeviceString[NUM_BOOT_DEVICES] =
{
    "Ethernet", "USB_Serial", "USB_RNDIS", "*USB_DNW"
};

/* 
    @func   BOOL | PrintMainMenu | Print out the Samsung bootloader main menu.
    @rdesc  void return
    @comm
    @xref
*/
static void PrintMainMenu(PBOOT_CFG pBootCfg)
{
    int i=0;
    EdbgOutputDebugString ( "\r\nEthernet Boot Loader Configuration:\r\n\r\n");
    EdbgOutputDebugString ( "----------- Connectivity Settings ------------\r\n");
    EdbgOutputDebugString ( "0) IP address  : [%s]\r\n",inet_ntoa(pBootCfg->EdbgAddr.dwIP));
    EdbgOutputDebugString ( "1) Subnet mask : [%s]\r\n", inet_ntoa(pBootCfg->SubnetMask));
    EdbgOutputDebugString ( "2) DHCP : [%s]\r\n", (pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP)?"*Enabled":"Disabled");
    EdbgOutputDebugString ( "3) Program CS8900 MAC address : [%B:%B:%B:%B:%B:%B]\r\n",
                           pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, pBootCfg->EdbgAddr.wMAC[0] >> 8,
                           pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, pBootCfg->EdbgAddr.wMAC[1] >> 8,
                           pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, pBootCfg->EdbgAddr.wMAC[2] >> 8);

    EdbgOutputDebugString ( "--------- Boot Configuration Section ---------\r\n");
    EdbgOutputDebugString ( "4) Reset to factory default configuration\r\n");    
    EdbgOutputDebugString ( "5) Startup Action after Boot delay : [%s]\r\n", (pBootCfg->ConfigFlags & BOOT_TYPE_DIRECT) ? "Launch Existing OS image from Storage" : "*Download New image");
    EdbgOutputDebugString ( "6) Boot delay: %d seconds\r\n", pBootCfg->BootDelay);    
    EdbgOutputDebugString ( "R) Read Configuration(TOC) \r\n");
    EdbgOutputDebugString ( "W) Write Configuration Data(TOC) Right Now\r\n");

    EdbgOutputDebugString ( "------- Kernel Booting Option Section --------\r\n");
    EdbgOutputDebugString ( "K) KITL Configuration           : [%s]\r\n", (pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL) ? "*Enabled" : "Disabled");    
    EdbgOutputDebugString ( "I) KITL Connection Mode         : [%s]\r\n", (pBootCfg->ConfigFlags & CONFIG_FLAGS_KITLPOLL) ? "Polling" : "*Interrupt");
    EdbgOutputDebugString ( "C) Force Clean Boot Option      : [%s]\r\n", (pBootCfg->ConfigFlags & BOOT_OPTION_CLEAN)?"*True":"False");
    EdbgOutputDebugString ( "H) Hive Clean on Boot-time      : [%s]\r\n",  (pBootCfg->ConfigFlags & BOOT_OPTION_HIVECLEAN)?"True":"*False");
    EdbgOutputDebugString ( "P) Format Partition on Boot-time: [%s]\r\n",  (pBootCfg->ConfigFlags & BOOT_OPTION_FORMATPARTITION)?"True":"*False");

    EdbgOutputDebugString ( "------------- NAND Flash Section -------------\r\n");
    // N.B: we need this option here since BinFS is really a RAM image, where you "format" the media
    // with an MBR. There is no way to parse the image to say it's ment to be BinFS enabled.
    EdbgOutputDebugString ( "A) Erase All Blocks \r\n");
    EdbgOutputDebugString ( "E) Erase Reserved Block(Stepldr+Eboot) \r\n");    
    EdbgOutputDebugString ( "F) Format Boot Media for BINFS with BadBlock Marking to Reserved Block\r\n");
    EdbgOutputDebugString ( "N) Nand Information and Dump NAND Flash\r\n");

    EdbgOutputDebugString ( "--------- Download and Launch Section --------\r\n");
    // This selection option can configure Download connectivity and KITL connectivity
    EdbgOutputDebugString ( "S) Switch Boot Device : [%s]  \r\n", BootDeviceString[pBootCfg->BootDevice]);
    EdbgOutputDebugString ( "        { Options :");
    for (i=0; i<NUM_BOOT_DEVICES; i++)
    {
        EdbgOutputDebugString ( " %s,", BootDeviceString[i]);
    }
    EdbgOutputDebugString ( "\b }\r\n");
    // We did not check TARGET_TYPE_RAMIMAGE
    EdbgOutputDebugString ( "T) Download Target: [%s]\r\n", (pBootCfg->ConfigFlags & TARGET_TYPE_NAND) ? "Write to NAND Storage" : "*Download to RAM");
    EdbgOutputDebugString ( "D) Download or Program image(OS image will be launched)\r\n");
    EdbgOutputDebugString ( "L) LAUNCH existing Boot Media image\r\n");

    EdbgOutputDebugString ( "\r\nEnter your selection: ");
}

/*
    @func   BOOL | ConfirmProcess | Check if continue or not
    @rdesc  TRUE == Success and FALSE == Failure.
    @comm
    @xref
*/
static BOOL ConfirmProcess(const char *msg)
{
    BYTE KeySelect = 0;

    EdbgOutputDebugString ( msg);            
    while (! ( ( (KeySelect == 'Y') || (KeySelect == 'y') ) ||
               ( (KeySelect == 'N') || (KeySelect == 'n') ) ))
    {
        KeySelect = OEMReadDebugByte();
    }
    
    if(KeySelect == 'Y' || KeySelect == 'y')
    {
        return TRUE;
    }
    return FALSE;
}
/*
    @func   BOOL | MainMenu | Manages the Samsung bootloader main menu.
    @rdesc  TRUE == Success and FALSE == Failure.
    @comm
    @xref
*/
static BOOL MainMenu(PBOOT_CFG pBootCfg)
{
    BYTE KeySelect = 0;
    BOOL bConfigChanged = FALSE;
    BOOLEAN bDownload = TRUE;

    if (pBootCfg->BootDevice > NUM_BOOT_DEVICES)
    {
        pBootCfg->BootDevice = g_DefaultBootDevice;
    }

    while(TRUE)
    {
        PrintMainMenu(pBootCfg);    
        
        KeySelect = 0;
        while (! ( ( (KeySelect >= '0') && (KeySelect <= '6') ) ||
                   ( (KeySelect == 'A') || (KeySelect == 'a') ) ||
                   ( (KeySelect == 'C') || (KeySelect == 'c') ) ||
                   ( (KeySelect == 'D') || (KeySelect == 'd') ) ||
                   ( (KeySelect == 'E') || (KeySelect == 'e') ) ||
                   ( (KeySelect == 'F') || (KeySelect == 'f') ) ||
                   ( (KeySelect == 'H') || (KeySelect == 'h') ) ||
                   ( (KeySelect == 'K') || (KeySelect == 'k') ) ||
                   ( (KeySelect == 'I') || (KeySelect == 'i') ) ||
                   ( (KeySelect == 'L') || (KeySelect == 'l') ) ||
                   ( (KeySelect == 'N') || (KeySelect == 'n') ) ||
                   ( (KeySelect == 'P') || (KeySelect == 'p') ) ||
                   ( (KeySelect == 'R') || (KeySelect == 'r') ) ||
                   ( (KeySelect == 'S') || (KeySelect == 's') ) ||
                   ( (KeySelect == 'T') || (KeySelect == 't') ) ||                   
                   ( (KeySelect == 'W') || (KeySelect == 'w') ) ))
        {
            KeySelect = OEMReadDebugByte();
        }

        EdbgOutputDebugString ( "%c\r\n", KeySelect);

        switch(KeySelect)
        {
        case '0':           // Change IP address.
            SetIP(pBootCfg);
            pBootCfg->ConfigFlags &= ~CONFIG_FLAGS_DHCP;   // clear DHCP flag
            bConfigChanged = TRUE;
            break;
        case '1':           // Change subnet mask.
            SetMask(pBootCfg);
            bConfigChanged = TRUE;
            break;
        case '2':           // Toggle static/DHCP mode.
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ CONFIG_FLAGS_DHCP);
            bConfigChanged = TRUE;
            break;
        case '3':           // Configure Crystal CS8900 MAC address.
            SetCS8900MACAddress(pBootCfg);
            bConfigChanged = TRUE;
            break;
        case '4':           // Reset the bootloader configuration to defaults.
            EdbgOutputDebugString("Resetting default TOC...Wait to complete\r\n");
            TOC_Init(DEFAULT_IMAGE_DESCRIPTOR, (IMAGE_TYPE_RAMIMAGE|IMAGE_TYPE_BINFS), 0, 0, 0);
            if ( !TOC_Write() ) {
                OALMSG(OAL_ERROR, (TEXT("TOC_Write Failed!\r\n")));
            }
            EdbgOutputDebugString("...TOC complete\r\n");
            break;
        case '5':           // Toggle download/launch status.
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ BOOT_TYPE_DIRECT);
            bConfigChanged = TRUE;
            break;
        case '6':           // Change autoboot delay.
            SetDelay(pBootCfg);
            bConfigChanged = TRUE;
            break;
        case 'A':
        case 'a':
            if(ConfirmProcess("CAUTION! This will erase all DATA(Bootloader and OS) in storage!\n" \
                              "Do you really want to erase all? (Yes or No)\r\n"))
            {
                EraseNANDBlockRegion(0, wNUM_BLOCKS, "All Block");
            }
            break;
        case 'C':    // Toggle image storage to Smart Media.
        case 'c':
            pBootCfg->ConfigFlags= (pBootCfg->ConfigFlags ^ BOOT_OPTION_CLEAN);
            bConfigChanged = TRUE;
            break;
        case 'D':           // Download? Yes.
        case 'd':
            bDownload = TRUE;
            goto MENU_DONE;
        case 'E':
        case 'e':
            if(ConfirmProcess("CAUTION! This will erase all BOOTLOADER Data(StepLoader and EBOOT) in storage!\n" \
                              "Do you really want to erase all? (Yes or No)\r\n"))
            {
            
                // This will erase reserved block for steploader and eboot
                if ( !g_bBootMediaExist ) {
                    OALMSG(OAL_ERROR, (TEXT("ERROR: BootMonitor: boot media does not exist.\r\n")));
                    continue;
                } else {
                    EraseNANDBlockRegion(NBOOT_BLOCK, NBOOT_BLOCK+NBOOT_BLOCK_SIZE, "StepLoader");
                    EraseNANDBlockRegion(EBOOT_BLOCK, EBOOT_BLOCK+EBOOT_BLOCK_SIZE, "EBOOT");
                }
            }
            break;
        case 'F':
        case 'f':
            if(ConfirmProcess("CAUTION! This will erase all OS IMAGE in storage!\n" \
                              "Do you really want to erase all? (Yes or No)\r\n"))
            {
    /*            
                // low-level format NAND Erasing            
                // format the boot media for BinFS
                // N.B: this does not destroy our OEM reserved sections (TOC, bootloaders, etc)
                if ( !g_bBootMediaExist ) {
                    OALMSG(OAL_ERROR, (TEXT("ERROR: BootMonitor: boot media does not exist.\r\n")));
                    break;
                }
                
                EraseNANDBlockRegion(IMAGE_START_BLOCK, wNUM_BLOCKS, "OS System");
    */
                MarkReservedBlockWithBadBlock();
                // N.B: this erases images, BinFs, FATFS, user data, etc.
                // N.B: format offset by # of reserved blocks,
                // decrease the ttl # blocks available by that amount.
                if ( !BP_LowLevelFormat( IMAGE_START_BLOCK,
                                         wNUM_BLOCKS - IMAGE_START_BLOCK,
                                         FORMAT_SKIP_BLOCK_CHECK) )
                {
                    OALMSG(OAL_ERROR, (TEXT("ERROR: BootMonitor: Low-level boot media format failed.\r\n")));
                    continue;
                }
            }            
            break;
        case 'H':    // Toggle 
        case 'h':
            pBootCfg->ConfigFlags= (pBootCfg->ConfigFlags ^ BOOT_OPTION_HIVECLEAN);
            bConfigChanged = TRUE;
            break;
        case 'K':           // Toggle Kitl Enable
        case 'k':
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ CONFIG_FLAGS_KITL);
            g_bWaitForConnect = (pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL) ? TRUE : FALSE;
            bConfigChanged = TRUE;
            break;
        case 'I':           // Toggle Kitl Mode
        case 'i':
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ CONFIG_FLAGS_KITLPOLL);
            bConfigChanged = TRUE;
            break;
        case 'L':           // Download? No.
        case 'l':
            bDownload = FALSE;
            goto MENU_DONE;
        case 'N':
        case 'n':
            {
                FlashInfo flashInfo;
                
                #define READ_LB_NAND_M_SIZE 2048
                #define READ_LB_NAND_S_SIZE 64
                #define READ_SB_NAND_M_SIZE 512
                #define READ_SB_NAND_S_SIZE 16
 
                UINT8  pMBuf[READ_LB_NAND_M_SIZE], pSBuf[READ_LB_NAND_S_SIZE];
                UINT32 SectorStartAddress;
                UINT32 nMainloop, nSpareloop;

                FMD_GetInfo(&flashInfo);
                EdbgOutputDebugString ( "%s Flash Info:: Blocks:[%d] x BlockSize:[%d]KB = [%d]MB\r\n",
                                        (flashInfo.flashType == NAND) ? "NAND" : " NOR", wNUM_BLOCKS, flashInfo.dwBytesPerBlock,
                                                (wNUM_BLOCKS*flashInfo.dwBytesPerBlock/1024) );
                EdbgOutputDebugString ( "               :: SectorsPerBlock:[%d], Sector size:[%d]\r\n",
                                                        flashInfo.wSectorsPerBlock, flashInfo.wDataBytesPerSector);
                TOC_Print();
                
 
                SectorStartAddress = SetBlockPage();
                if(SectorStartAddress == -1)
                {
                    break;
                }
                EdbgOutputDebugString("\r\n Sector Start Address : 0x%x\r\n", SectorStartAddress);
                memset(pMBuf, 0xff, READ_LB_NAND_M_SIZE);
                memset(pSBuf, 0xff, READ_LB_NAND_S_SIZE);
 
                if (IS_LB)
                {
                    nMainloop  
                        = READ_LB_NAND_M_SIZE;
                    nSpareloop = READ_LB_NAND_S_SIZE;
 
                    RAW_LB_ReadSector(SectorStartAddress, pMBuf, pSBuf);
                }
                else
                {
                    nMainloop  = READ_SB_NAND_M_SIZE;
                    nSpareloop = READ_SB_NAND_S_SIZE;
 
                    RAW_SB_ReadSector(SectorStartAddress, pMBuf, pSBuf);
                }
 
                PrintPageData(nMainloop, nSpareloop, pMBuf, pSBuf);
            }
            break;            
        case 'P':    // Toggle image storage to Smart Media.
        case 'p':
            pBootCfg->ConfigFlags= (pBootCfg->ConfigFlags ^ BOOT_OPTION_FORMATPARTITION);
            bConfigChanged = TRUE;
            break;
            
        case 'R':
        case 'r':
            TOC_Read();
            TOC_Print();
            // TODO
            break;
        case 'S':           // Switch Boot Device
        case 's':
            pBootCfg->BootDevice++;
            if (pBootCfg->BootDevice > NUM_BOOT_DEVICES - 1)
            {
                pBootCfg->BootDevice = 0;
            }
            bConfigChanged = TRUE;
            break;
        case 'T':           // Toggle image storage to Smart Media.
        case 't':                 
            pBootCfg->ConfigFlags = (pBootCfg->ConfigFlags ^ TARGET_TYPE_NAND);
            bConfigChanged = TRUE;
            break;
        case 'W':           // Configuration Write
        case 'w':
            if (!TOC_Write())
            {
                OALMSG(OAL_WARN, (TEXT("WARNING: MainMenu: Failed to store updated eboot configuration in flash.\r\n")));
            }
            else
            {
                OALMSG(OAL_INFO, (TEXT("Successfully Written\r\n")));
                bConfigChanged = FALSE;
            }
            break;
        default:
            break;
        }
    }

MENU_DONE:

    // If eboot settings were changed by user, save them to flash.
    //
    if (bConfigChanged && !TOC_Write())
    {
        OALMSG(OAL_WARN, (TEXT("WARNING: MainMenu: Failed to store updated bootloader configuration to flash.\r\n")));
    }

    return(bDownload);
}

static UINT8 WaitForInitialSelection()
{
    UINT8 KeySelect;
    ULONG BootDelay;
    UINT32 dwStartTime, dwPrevTime, dwCurrTime;

    BootDelay = g_pBootCfg->BootDelay;

    if (g_pBootCfg->ConfigFlags & BOOT_TYPE_DIRECT)
    {
        EdbgOutputDebugString("\r\nPress [ENTER] to launch image stored on boot media, or [SPACE] to enter boot monitor.\r\n");
        EdbgOutputDebugString("\r\nInitiating image launch in %d seconds. ",BootDelay--);
    }
    else
    {
        EdbgOutputDebugString("\r\nPress [ENTER] to download image, or [SPACE] to enter boot monitor.\r\n");
        EdbgOutputDebugString("\r\nInitiating image download in %d seconds. ",BootDelay--);
    }
    
    dwStartTime = OEMEthGetSecs();
    dwPrevTime  = dwStartTime;
    dwCurrTime  = dwStartTime;
    KeySelect   = 0;

    // Allow the user to break into the bootloader menu.
    while((dwCurrTime - dwStartTime) < g_pBootCfg->BootDelay)
    {
        KeySelect = OEMReadDebugByte();

        if ((KeySelect == 0x20) || (KeySelect == 0x0d))
        {
            break;
        }

        dwCurrTime = OEMEthGetSecs();

        if (dwCurrTime > dwPrevTime)
        {
            int i, j;

            // 1 Second has elapsed - update the countdown timer.
            dwPrevTime = dwCurrTime;

            // for text alignment
            if (BootDelay < 9)
            {
                i = 11;
            }
            else if (BootDelay < 99)
            {
                i = 12;
            }
            else if (BootDelay < 999)
            {
                i = 13;
            }
            else 
            {
                i = 14;     //< we don't care about this value when BootDelay over 1000 (1000 seconds)
            }
                

            for(j = 0; j < i; j++)
            {
                OEMWriteDebugByte((BYTE)0x08); // print back space
            }

            EdbgOutputDebugString ( "%d seconds. ", BootDelay--);
        }
    }

    EdbgOutputDebugString("\r\n");
    return KeySelect;
}


/*
    @func   BOOL | OEMPlatformInit | Initialize the Samsung SMD6410 platform hardware.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
void SetKITLConfigAndBootOptions()
{
    //Update  Argument Area Value(KITL, Clean Option)
    if(g_pBootCfg->ConfigFlags &  BOOT_OPTION_CLEAN)
    {
        *g_bCleanBootFlag =TRUE;
    }
    else
    {
        *g_bCleanBootFlag =FALSE;
    }
    if(g_pBootCfg->ConfigFlags &  BOOT_OPTION_HIVECLEAN)
    {
        *g_bHiveCleanFlag =TRUE;
    }
    else
    {
        *g_bHiveCleanFlag =FALSE;
    }
    if(g_pBootCfg->ConfigFlags &  BOOT_OPTION_FORMATPARTITION)
    {
        *g_bFormatPartitionFlag = TRUE;
    }
    else
    {
        *g_bFormatPartitionFlag =FALSE;
    }

    if(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL)
    {
        g_KITLConfig->flags=OAL_KITL_FLAGS_ENABLED;
    }
    else
    {
        g_KITLConfig->flags&=~OAL_KITL_FLAGS_ENABLED;
    }
    if(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_KITLPOLL)
    {
        g_KITLConfig->flags|= OAL_KITL_FLAGS_POLL;
    }
    else
    {
        g_KITLConfig->flags&=~OAL_KITL_FLAGS_POLL;
    }
    

    g_KITLConfig->ipAddress = g_pBootCfg->EdbgAddr.dwIP;
    g_KITLConfig->ipMask    = g_pBootCfg->SubnetMask;

    memcpy(g_KITLConfig->mac, g_pBootCfg->EdbgAddr.wMAC, 6);

    OALKitlCreateName(BSP_DEVICE_PREFIX, g_KITLConfig->mac, g_DevID);

    switch(g_pBootCfg->BootDevice)
    {
        case BOOT_DEVICE_ETHERNET:
            {
                // Configure Ethernet controller.
                g_KITLConfig->devLoc.IfcType    = Internal;
                g_KITLConfig->devLoc.BusNumber  = 0;
                g_KITLConfig->devLoc.LogicalLoc = BSP_BASE_REG_PA_CS8900A_IOBASE;
                g_KITLConfig->flags |= OAL_KITL_FLAGS_VMINI;
            }
            break;
        case BOOT_DEVICE_USB_SERIAL:
            {
                g_KITLConfig->devLoc.IfcType    = Internal;
                g_KITLConfig->devLoc.BusNumber  = 0;
                g_KITLConfig->devLoc.LogicalLoc = S3C6410_BASE_REG_PA_USBOTG_LINK;
            }
            break;

        case BOOT_DEVICE_USB_RNDIS:
            {
                g_KITLConfig->devLoc.IfcType    = InterfaceTypeUndefined; // Using InterfaceTypeUndefined will differentiate between USB RNDIS and USB Serial
                g_KITLConfig->devLoc.BusNumber  = 0;
                g_KITLConfig->devLoc.LogicalLoc = S3C6410_BASE_REG_PA_USBOTG_LINK;
                g_KITLConfig->devLoc.Pin        = IRQ_OTG;
                g_KITLConfig->flags |= OAL_KITL_FLAGS_VMINI;
            }
            break;
        case BOOT_DEVICE_USB_DNW:
            // Use USB Serial transport for KITL in this case
            g_KITLConfig->devLoc.IfcType    = Internal;
            g_KITLConfig->devLoc.BusNumber  = 0;
            g_KITLConfig->devLoc.LogicalLoc = S3C6410_BASE_REG_PA_USBOTG_LINK;
            break;
        default: 
            EdbgOutputDebugString("%s: ERROR: unknown Boot device: 0x%x \r\n", __FUNCTION__, g_pBootCfg->BootDevice);            
            g_KITLConfig->devLoc.IfcType    = InterfaceTypeUndefined;
            g_KITLConfig->devLoc.BusNumber  = 0;
            g_KITLConfig->devLoc.LogicalLoc = 0;
            break;
    }
                
}

/*
    @func   BOOL | OEMPlatformInit | Initialize the Samsung SMD6410 platform hardware.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL OEMPlatformInit(void)
{
    UINT8 KeySelect;
    BOOL bResult = FALSE;
    FlashInfo flashInfo;
    // This is actually not PCI bus, but we use this structure to share FMD library code with FMD driver
    PCI_REG_INFO    RegInfo;    

    OALMSG(OAL_FUNC, (TEXT("+OEMPlatformInit.\r\n")));

    // Check if Current ARM speed is not matched to Target Arm speed
    // then To get speed up, set Voltage
#if (APLL_CLK == CLK_1332MHz)
    LTC3714_Init();
    LTC3714_VoltageSet(1,1200,100);     // ARM
    LTC3714_VoltageSet(2,1300,100);     // INT
#endif
#if (TARGET_ARM_CLK == CLK_800MHz)
    LTC3714_Init();
    LTC3714_VoltageSet(1,1300,100);     // ARM
    LTC3714_VoltageSet(2,1200,100);     // INT
#endif

    EdbgOutputDebugString("Microsoft Windows CE Bootloader for the Samsung SMDK6410 Version %d.%d Built %s\r\n\r\n",
    EBOOT_VERSION_MAJOR, EBOOT_VERSION_MINOR, __DATE__);

    // Set OTG Device's Phy clock
//    InitializeOTGCLK();
               
    // Initialize the display.
    InitializeDisplay();

    // Initialize BCD registers in RTC to known values
    InitializeRTC();

    // Initialize the BSP args structure.
    OALArgsInit(pBSPArgs);

    g_bCleanBootFlag = (BOOL*)OALArgsQuery(BSP_ARGS_QUERY_CLEANBOOT) ;
    g_bHiveCleanFlag = (BOOL*)OALArgsQuery(BSP_ARGS_QUERY_HIVECLEAN);
    g_bFormatPartitionFlag = (BOOL*)OALArgsQuery(BSP_ARGS_QUERY_FORMATPART);
    g_KITLConfig = (OAL_KITL_ARGS *)OALArgsQuery(OAL_ARGS_QUERY_KITL);
    g_DevID = (UCHAR *)OALArgsQuery( OAL_ARGS_QUERY_DEVID);

//    InitializeInterrupt();

    // Try to initialize the boot media block driver and BinFS partition.
    //
    OALMSG(OAL_INFO, (TEXT("BP_Init\r\n")));

    memset(&RegInfo, 0, sizeof(PCI_REG_INFO));
    RegInfo.MemBase.Num = 2;
    RegInfo.MemBase.Reg[0] = (DWORD)OALPAtoVA(S3C6410_BASE_REG_PA_NFCON, FALSE);
    RegInfo.MemBase.Reg[1] = (DWORD)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    if (!BP_Init((LPBYTE)BINFS_RAM_START, BINFS_RAM_LENGTH, NULL, &RegInfo, NULL) )
    {
        OALMSG(OAL_WARN, (TEXT("WARNING: OEMPlatformInit failed to initialize Boot Media.\r\n")));
        g_bBootMediaExist = FALSE;
    }
    else
    {
        g_bBootMediaExist = TRUE;
    }

    // Get flash info
    if (!FMD_GetInfo(&flashInfo))
    {
        OALMSG(OAL_ERROR, (L"ERROR: BLFlashDownload: FMD_GetInfo call failed\r\n"));
    }

    wNUM_BLOCKS = flashInfo.dwNumBlocks;
    OALMSG(OAL_INFO, (TEXT("wNUM_BLOCKS : %d(0x%x) \r\n"), wNUM_BLOCKS, wNUM_BLOCKS));
    stDeviceInfo = GetNandInfo();

    // Try to retrieve TOC (and Boot config) from boot media
    if ( !TOC_Read( ) )
    {
        // use default settings
        TOC_Init(DEFAULT_IMAGE_DESCRIPTOR, (IMAGE_TYPE_RAMIMAGE), 0, 0, 0);
    }

    // Display boot message - user can halt the autoboot by pressing any key on the serial terminal emulator.    
    KeySelect = WaitForInitialSelection();

    // Boot or enter bootloader menu.
    //
    switch(KeySelect)
    {
    case 0x20: // Boot menu.
//        g_pBootCfg->ConfigFlags &= ~BOOT_OPTION_CLEAN;        // Always clear CleanBoot Flags before Menu
        g_bDownloadImage = MainMenu(g_pBootCfg);
        break;
    case 0x00: // Fall through if no keys were pressed -or-
    case 0x0d: // the user cancelled the countdown.
    default:
        if (g_pBootCfg->ConfigFlags & BOOT_TYPE_DIRECT)
        {
            EdbgOutputDebugString("\r\nLaunching image from boot media ... \r\n");
            g_bDownloadImage = FALSE;
        }
        else
        {
            EdbgOutputDebugString("\r\nStarting auto-download ... \r\n");
            g_bDownloadImage = TRUE;
        }
        break;
    }

    SetKITLConfigAndBootOptions();

    if ( !g_bDownloadImage )
    {
        // User doesn't want to download image - load it from the boot media.
        // We could read an entire nk.bin or nk.nb0 into ram and jump.
        if ( !VALID_TOC(g_pTOC) )
        {
            EdbgOutputDebugString("OEMPlatformInit: ERROR_INVALID_TOC, can not autoboot.\r\n");
            return FALSE;
        }

        switch (g_ImageType)
        {
        case IMAGE_TYPE_STEPLDR:
            EdbgOutputDebugString("Don't support launch STEPLDR.bin\r\n");
            break;
        case IMAGE_TYPE_LOADER:
            EdbgOutputDebugString("Don't support launch EBOOT.bin\r\n");
            break;
        case IMAGE_TYPE_RAMIMAGE:
            OTGDEV_SetSoftDisconnect();
            OALMSG(TRUE, (TEXT("OEMPlatformInit: IMAGE_TYPE_RAMIMAGE\r\n")));
            if ( !ReadOSImageFromBootMedia( ) )
            {
                EdbgOutputDebugString("OEMPlatformInit ERROR: Failed to load kernel region into RAM.\r\n");
                return FALSE;
            }
            break;

        default:
            EdbgOutputDebugString("OEMPlatformInit ERROR: unknown image type: 0x%x \r\n", g_ImageType);
            return FALSE;
        }
    }
    else // if ( g_bDownloadImage )
    {
        switch(g_pBootCfg->BootDevice)
        {
            case BOOT_DEVICE_ETHERNET:
                // Configure Ethernet controller.
                if (!InitEthDevice(g_pBootCfg))
                {
                    EdbgOutputDebugString("ERROR: OEMPlatformInit: Failed to initialize Ethernet controller.\r\n");
                    goto CleanUp;
                }
                break;

            case BOOT_DEVICE_USB_SERIAL:
                {
                    // Configure Serial USB Download
                    KITL_SERIAL_INFO SerInfo;
                    SerInfo.pAddress = (UINT8 *)S3C6410_BASE_REG_PA_USBOTG_LINK;

                    EdbgOutputDebugString("OEMPlatformInit: BootDevice - USB Serial.\r\n");
                    EdbgOutputDebugString("Waiting for Platform Builder to connect...\r\n");

                    // Disconnect the device from the bus
                    OTGDEV_SetSoftDisconnect();
                    if (!InitializeUSB())
                    {
                        EdbgOutputDebugString("OEMPlatformInit: Failed to initialize USB.\r\n");
                        return(FALSE);
                    }
                    if (!Serial_Init(&SerInfo))
                    {
                        EdbgOutputDebugString("OEMPlatformInit: Failed to initialize USB for serial download.\r\n");
                        return(FALSE);
                    }
                    EdbgOutputDebugString("OEMPlatformInit: Initialized USB for serial download.\r\n");
                }
                break;

            case BOOT_DEVICE_USB_RNDIS:
                EdbgOutputDebugString("OEMPlatformInit: BootDevice - USB RNDIS.\r\n");
                // Disconnect the device from the bus
                OTGDEV_SetSoftDisconnect();
                if (!InitializeUSB())
                {
                    EdbgOutputDebugString("OEMPlatformInit: Failed to initialize USB.\r\n");
                    return(FALSE);
                }
                if (!Rndis_Init((UINT8 *)S3C6410_BASE_REG_PA_USBOTG_LINK, 0, g_KITLConfig->mac))
                {
                    EdbgOutputDebugString("OEMPlatformInit: ERROR: Failed to initialize USB RNDIS for Download.\r\n");
                    return(FALSE);
                }
                EdbgOutputDebugString("OEMPlatformInit: Initialized USB for RNDIS download.\r\n");
                break;
            case BOOT_DEVICE_USB_DNW:
                // Configure USB Download
                InitializeInterrupt();
                // Disconnect the device from the bus
                OTGDEV_SetSoftDisconnect();
                if (!InitializeUSB())
                {
                    EdbgOutputDebugString("OEMPlatformInit: Failed to initialize USB.\r\n");
                    return(FALSE);
                }
                break;
            default:
                EdbgOutputDebugString("OEMPlatformInit: ERROR: unknown Boot device: 0x%x \r\n", g_pBootCfg->BootDevice);
                return FALSE;
        }
    }

    bResult = TRUE;

    CleanUp:

    OALMSG(OAL_FUNC, (TEXT("_OEMPlatformInit.\r\n")));

    return(bResult);
}


/*
    @func   DWORD | OEMPreDownload | Complete pre-download tasks - get IP address, initialize TFTP, etc.
    @rdesc  BL_DOWNLOAD = Platform Builder is asking us to download an image, BL_JUMP = Platform Builder is requesting we jump to an existing image, BL_ERROR = Failure.
    @comm
    @xref
*/
DWORD OEMPreDownload(void)
{
    BOOL  bGotJump = FALSE;
    DWORD dwDHCPLeaseTime = 0;
    PDWORD pdwDHCPLeaseTime = &dwDHCPLeaseTime;
    DWORD dwBootFlags = 0;

    OALMSG(OAL_FUNC, (TEXT("+OEMPreDownload.\r\n")));

    // Create device name based on Ethernet address (this is how Platform Builder identifies this device).
    //
    OALKitlCreateName(BSP_DEVICE_PREFIX, pBSPArgs->kitl.mac, pBSPArgs->deviceId);
    EdbgOutputDebugString("INFO: *** Device Name '%s' ***\r\n", pBSPArgs->deviceId);

    // We will skip download process, we want to jump to Kernel Address
    if ( !g_bDownloadImage)
    {
        return(BL_JUMP);
    }    

    switch(g_pBootCfg->BootDevice)
    {
        case BOOT_DEVICE_ETHERNET:
        case BOOT_DEVICE_USB_RNDIS:
            // If the user wants to use a static IP address, don't request an address
            // from a DHCP server.  This is done by passing in a NULL for the DHCP
            // lease time variable.  If user specified a static IP address, use it (don't use DHCP).
            //
            if (!(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP))
            {
                // Static IP address.
                pBSPArgs->kitl.ipAddress  = g_pBootCfg->EdbgAddr.dwIP;
                pBSPArgs->kitl.ipMask     = g_pBootCfg->SubnetMask;
                pBSPArgs->kitl.flags     &= ~OAL_KITL_FLAGS_DHCP;
                pdwDHCPLeaseTime = NULL;
                OALMSG(OAL_INFO, (TEXT("INFO: Using static IP address %s.\r\n"), inet_ntoa(pBSPArgs->kitl.ipAddress)));
                OALMSG(OAL_INFO, (TEXT("INFO: Using subnet mask %s.\r\n"),       inet_ntoa(pBSPArgs->kitl.ipMask)));
            }
            else
            {
                pBSPArgs->kitl.ipAddress = 0;
                pBSPArgs->kitl.ipMask    = 0;
            }

            // Initialize the the TFTP transport.
            //
            g_DeviceAddr.dwIP = pBSPArgs->kitl.ipAddress;
            memcpy(g_DeviceAddr.wMAC, pBSPArgs->kitl.mac, (3 * sizeof(UINT16)));
            g_DeviceAddr.wPort = 0;

            if (!EbootInitEtherTransport(&g_DeviceAddr,
                                         &pBSPArgs->kitl.ipMask,
                                         &bGotJump,
                                         pdwDHCPLeaseTime,
                                         EBOOT_VERSION_MAJOR,
                                         EBOOT_VERSION_MINOR,
                                         BSP_DEVICE_PREFIX,
                                         pBSPArgs->deviceId,
                                         EDBG_CPU_ARM720,
                                         dwBootFlags))
            {
                EdbgOutputDebugString("ERROR: OEMPreDownload: Failed to initialize Ethernet connection.\r\n");
                return(BL_ERROR);
            }


            // If the user wanted a DHCP address, we presumably have it now - save it for the OS to use.
            //
            if (g_pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP)
            {
                // DHCP address.
                pBSPArgs->kitl.ipAddress  = g_DeviceAddr.dwIP;
                pBSPArgs->kitl.flags     |= OAL_KITL_FLAGS_DHCP;
            }
            break;

        case BOOT_DEVICE_USB_SERIAL:
            // Send boot requests indefinitely
            do
            {
                OALMSG(TRUE, (TEXT("Sending boot request...\r\n")));
                if(!SerialSendBootRequest(BSP_DEVICE_PREFIX))
                {
                    OALMSG(TRUE, (TEXT("Failed to send boot request\r\n")));
                    return BL_ERROR;
                }
            }
            while(!SerialWaitForBootAck(&bGotJump));
            
            // Ack block zero to start the download
            SerialSendBlockAck(0);

            if( bGotJump )
            {
                OALMSG(TRUE, (TEXT("Received boot request ack... jumping to image\r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT("Received boot request ack... starting download\r\n")));
            }

            break;

        case BOOT_DEVICE_USB_DNW:
            EdbgOutputDebugString("Please send the Image through USB.\r\n");
            break;

    }

    return(bGotJump ? BL_JUMP : BL_DOWNLOAD);
}


/*
    @func   BOOL | OEMReadData | Generically read download data (abstracts actual transport read call).
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL OEMReadData(DWORD dwData, PUCHAR pData)
{
    BOOL ret;
    OALMSG(OAL_FUNC, (TEXT("+OEMReadData.\r\n")));

    switch(g_pBootCfg->BootDevice)
    {
        case BOOT_DEVICE_ETHERNET:
        case BOOT_DEVICE_USB_RNDIS:
            ret = EbootEtherReadData(dwData, pData);
            break;

        case BOOT_DEVICE_USB_SERIAL:
            ret = SerialReadData(dwData, pData);
            break;

        case BOOT_DEVICE_USB_DNW:
            ret = UbootReadData(dwData, pData);
            break;
    } 

    return(ret);
}

void OEMReadDebugString(CHAR * szString)
{
    USHORT cwNumChars = 0;
    USHORT InChar = 0;

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            if ((InChar >= 'a' && InChar <='z') || (InChar >= 'A' && InChar <= 'Z') || (InChar >= '0' && InChar <= '9'))
            {
                if (cwNumChars < 16)
                {
                    szString[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8)
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }

    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars)
    {
        szString[cwNumChars] = '\0';
        EdbgOutputDebugString("\r\n");
    }
}


/*
    @func   void | OEMShowProgress | Displays download progress for the user.
    @rdesc  N/A.
    @comm
    @xref
*/
void OEMShowProgress(DWORD dwPacketNum)
{
    // If user select USB_DNW(download thourhg USB using DNW) 
    // and program to NAND storage, This will be Programming Progress
    // If not this is download progress
    OALMSG(OAL_INFO&&OAL_FUNC, (TEXT("%d.\r\n"), dwPacketNum));
}


/*
    @func   void | OEMLaunch | Executes the stored/downloaded image.
    @rdesc  N/A.
    @comm
    @xref
*/

void OEMLaunch( DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr, const ROMHDR *pRomHdr )
{
    DWORD dwPhysLaunchAddr;
    EDBG_ADDR EshellHostAddr;
    EDBG_OS_CONFIG_DATA *pCfgData;
    UINT32 i;

    OALMSG(OAL_FUNC, (TEXT("+OEMLaunch.\r\n")));


    // If the user requested that a disk image (stored in RAM now) be written to the SmartMedia card, so it now.
    //
    if (g_bDownloadImage && (g_pBootCfg->ConfigFlags & TARGET_TYPE_NAND))
    {
        // Since this platform only supports RAM images, the image cache address is the same as the image RAM address.
        //

        switch (g_ImageType)
        {
            case IMAGE_TYPE_STEPLDR:
                if (!WriteRawImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
                {
                    EdbgOutputDebugString("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n");
                    goto CleanUp;
                }
                EdbgOutputDebugString("INFO: Step loader image stored to Smart Media.  Please Reboot.  Halting...\r\n");
                SpinForever();
                break;

            case IMAGE_TYPE_LOADER:
                g_pTOC->id[0].dwLoadAddress = dwImageStart;
                g_pTOC->id[0].dwTtlSectors = FILE_TO_SECTOR_SIZE(dwImageLength);
                if (!WriteRawImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
                {
                    EdbgOutputDebugString("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n");
                    goto CleanUp;
                }
                if (dwLaunchAddr && (g_pTOC->id[0].dwJumpAddress != dwLaunchAddr))
                {
                    g_pTOC->id[0].dwJumpAddress = dwLaunchAddr;
                    if ( !TOC_Write() ) {
                        EdbgOutputDebugString("*** OEMLaunch ERROR: TOC_Write failed! Next boot may not load from disk *** \r\n");
                    }
                    TOC_Print();
                }
                EdbgOutputDebugString("INFO: Eboot image stored to Smart Media.  Please Reboot.  Halting...\r\n");
                SpinForever();

                break;

            case IMAGE_TYPE_RAMIMAGE:
                g_pTOC->id[g_dwTocEntry].dwLoadAddress = dwImageStart;
                g_pTOC->id[g_dwTocEntry].dwTtlSectors = FILE_TO_SECTOR_SIZE(dwImageLength);
                if (!WriteOSImageToBootMedia(dwImageStart, dwImageLength, dwLaunchAddr))
                {
                    EdbgOutputDebugString("ERROR: OEMLaunch: Failed to store image to Smart Media.\r\n");
                    goto CleanUp;
                }

                if (dwLaunchAddr && (g_pTOC->id[g_dwTocEntry].dwJumpAddress != dwLaunchAddr))
                {
                    g_pTOC->id[g_dwTocEntry].dwJumpAddress = dwLaunchAddr;
                    if ( !TOC_Write() ) {
                        EdbgOutputDebugString("*** OEMLaunch ERROR: TOC_Write failed! Next boot may not load from disk *** \r\n");
                    }
                    TOC_Print();
                }
                else
                {
                    dwLaunchAddr= g_pTOC->id[g_dwTocEntry].dwJumpAddress;
                    EdbgOutputDebugString("INFO: using TOC[%d] dwJumpAddress: 0x%x\r\n", g_dwTocEntry, dwLaunchAddr);
                }

                break;
        }
    }
    else if(g_bDownloadImage)
    {
        switch (g_ImageType)
        {
            case IMAGE_TYPE_STEPLDR:
                EdbgOutputDebugString("Stepldr image can't launch from ram.\r\n");
                EdbgOutputDebugString("You should program it into flash.\r\n");
                SpinForever();
                break;
            case IMAGE_TYPE_LOADER:
                EdbgOutputDebugString("Eboot image can't launch from ram.\r\n");
                EdbgOutputDebugString("You should program it into flash.\r\n");
                SpinForever();
                break;
            default:
                break;
        }
    }

    // Wait for Platform Builder to connect after the download and send us IP and port settings for service
    // connections - also sends us KITL flags.  This information is used later by the OS (KITL).
    //
    if (g_bDownloadImage & g_bWaitForConnect)
    {
        EdbgOutputDebugString("Wait For Connect...\r\n");
        switch(g_pBootCfg->BootDevice)
        {
            case BOOT_DEVICE_ETHERNET:
            case BOOT_DEVICE_USB_RNDIS:
                memset(&EshellHostAddr, 0, sizeof(EDBG_ADDR));

                g_DeviceAddr.dwIP  = pBSPArgs->kitl.ipAddress;
                memcpy(g_DeviceAddr.wMAC, pBSPArgs->kitl.mac, (3 * sizeof(UINT16)));
                g_DeviceAddr.wPort = 0;

                if (!(pCfgData = EbootWaitForHostConnect(&g_DeviceAddr, &EshellHostAddr)))
                {
                    EdbgOutputDebugString("ERROR: OEMLaunch: EbootWaitForHostConnect failed.\r\n");
                    goto CleanUp;
                }

                // If the user selected "passive" KITL (i.e., don't connect to the target at boot time), set the
                // flag in the args structure so the OS image can honor it when it boots.
                //
                if (pCfgData->KitlTransport & KTS_PASSIVE_MODE)
                {
                    pBSPArgs->kitl.flags |= OAL_KITL_FLAGS_PASSIVE;
                }

                // save ethernet address for ethernet kitl // added by jjg 06.09.18
                SaveEthernetAddress();
                break;

            case BOOT_DEVICE_USB_SERIAL:
                {
                    DWORD dwKitlTransport;
                    dwKitlTransport = SerialWaitForJump();

                    if ((dwKitlTransport & KTS_PASSIVE_MODE) != 0) 
                    {
                        pBSPArgs->kitl.flags |= OAL_KITL_FLAGS_PASSIVE;
                    }
                }
                break;

            case BOOT_DEVICE_USB_DNW:
                break;
        } 
    }
    

    // If a launch address was provided, we must have downloaded the image, save the address in case we
    // want to jump to this image next time.  If no launch address was provided, retrieve the last one.
    //
    if (dwLaunchAddr && (g_pTOC->id[g_dwTocEntry].dwJumpAddress != dwLaunchAddr))
    {
        g_pTOC->id[g_dwTocEntry].dwJumpAddress = dwLaunchAddr;
    }
    else
    {
        dwLaunchAddr= g_pTOC->id[g_dwTocEntry].dwJumpAddress;
        OALMSG(OAL_INFO, (TEXT("INFO: using TOC[%d] dwJumpAddress: 0x%x\r\n"), g_dwTocEntry, dwLaunchAddr));
    }
    OALMSG(OAL_VERBOSE, (TEXT("pBSPArgs :0x%x\r\n"), pBSPArgs));
    for(i=0;i<sizeof(BSP_ARGS); i++)
    {
        OALMSG(OAL_VERBOSE, (TEXT("0x%02x "),*((UINT8*)pBSPArgs+i)));
    }    

    // Jump to downloaded image (use the physical address since we'll be turning the MMU off)...
    //
    dwPhysLaunchAddr = (DWORD)OALVAtoPA((void *)dwLaunchAddr);
    EdbgOutputDebugString("INFO: OEMLaunch: Jumping to Physical Address 0x%Xh (Virtual Address 0x%Xh)...\r\n\r\n\r\n", dwPhysLaunchAddr, dwLaunchAddr);

    // Jump...
    //
    Launch(dwPhysLaunchAddr);


CleanUp:

    EdbgOutputDebugString("ERROR: OEMLaunch: Halting...\r\n");
    SpinForever();
}


//------------------------------------------------------------------------------
//
//  Function Name:  OEMVerifyMemory( DWORD dwStartAddr, DWORD dwLength )
//  Description..:  This function verifies the passed address range lies
//                  within a valid region of memory. Additionally this function
//                  sets the g_ImageType if the image is a boot loader.
//  Inputs.......:  DWORD           Memory start address
//                  DWORD           Memory length
//  Outputs......:  BOOL - true if verified, false otherwise
//
//------------------------------------------------------------------------------

BOOL OEMVerifyMemory( DWORD dwStartAddr, DWORD dwLength )
{

    OALMSG(OAL_FUNC, (TEXT("+OEMVerifyMemory.\r\n")));
    OALMSG(OAL_INFO, (TEXT("dwStartAddr:0x%x, dwLength:0x%x\r\n"), dwStartAddr, dwLength));    

    // Is the image being downloaded the stepldr?
    if ((dwStartAddr >= STEPLDR_RAM_IMAGE_BASE) &&
        ((dwStartAddr + dwLength - 1) < (STEPLDR_RAM_IMAGE_BASE + STEPLDR_RAM_IMAGE_SIZE - STEPLDR_BIN_HEAD_CUT_OFFSET)))
    {
        EdbgOutputDebugString("OEMVerifyMemory: Stepldr image\r\n");
        g_ImageType = IMAGE_TYPE_STEPLDR;     // Stepldr image.
        return TRUE;
    }
    // Is the image being downloaded the bootloader?
    else if ((dwStartAddr >= EBOOT_STORE_ADDRESS) &&
        ((dwStartAddr + dwLength - 1) < (EBOOT_STORE_ADDRESS + EBOOT_STORE_MAX_LENGTH)))
    {
        EdbgOutputDebugString("OEMVerifyMemory: Eboot image\r\n");
        g_ImageType = IMAGE_TYPE_LOADER;     // Eboot image.
        return TRUE;
    }

    // Is it a ram image?
    else if ((dwStartAddr >= ROM_RAMIMAGE_START) &&
        ((dwStartAddr + dwLength - 1) < (ROM_RAMIMAGE_START + ROM_RAMIMAGE_SIZE)))
    {
        EdbgOutputDebugString("OEMVerifyMemory: RAM image\r\n");
        g_ImageType = IMAGE_TYPE_RAMIMAGE;
        return TRUE;
    }
    else if (!dwStartAddr && !dwLength)
    {
        EdbgOutputDebugString("OEMVerifyMemory: Don't support raw image\r\n");
        g_ImageType = IMAGE_TYPE_RAWBIN;
        return FALSE;
    }

    // HACKHACK: get around MXIP images with funky addresses
    OALMSG(TRUE, (TEXT("BIN image type is unknown\r\n")));

    OALMSG(OAL_FUNC, (TEXT("_OEMVerifyMemory.\r\n")));

    return FALSE;
}

/*
    @func   void | OEMMultiBINNotify | Called by blcommon to nofity the OEM code of the number, size, and location of one or more BIN regions,
                                       this routine collects the information and uses it when temporarily caching a flash image in RAM prior to final storage.
    @rdesc  N/A.
    @comm
    @xref
*/
void OEMMultiBINNotify(const PMultiBINInfo pInfo)
{
    BYTE nCount;

    OALMSG(OAL_FUNC, (TEXT("+OEMMultiBINNotify.\r\n")));

    if (!pInfo || !pInfo->dwNumRegions)
    {
        EdbgOutputDebugString("WARNING: OEMMultiBINNotify: Invalid BIN region descriptor(s).\r\n");
        return;
    }

    if (!pInfo->Region[0].dwRegionStart && !pInfo->Region[0].dwRegionLength)
    {
        return;
    }

    g_dwMinImageStart = pInfo->Region[0].dwRegionStart;

    EdbgOutputDebugString("\r\nDownload BIN file information:\r\n");
    EdbgOutputDebugString("-----------------------------------------------------\r\n");
    for (nCount = 0 ; nCount < pInfo->dwNumRegions ; nCount++)
    {
        EdbgOutputDebugString("[%d]: Base Address=0x%x  Length=0x%x\r\n",
            nCount, pInfo->Region[nCount].dwRegionStart, pInfo->Region[nCount].dwRegionLength);
        if (pInfo->Region[nCount].dwRegionStart < g_dwMinImageStart)
        {
            g_dwMinImageStart = pInfo->Region[nCount].dwRegionStart;
            if (g_dwMinImageStart == 0)
            {
                EdbgOutputDebugString("WARNING: OEMMultiBINNotify: Bad start address for region (%d).\r\n", nCount);
                return;
            }
        }
    }

    memcpy((LPBYTE)&g_BINRegionInfo, (LPBYTE)pInfo, sizeof(MultiBINInfo));

    EdbgOutputDebugString("-----------------------------------------------------\r\n");
    OALMSG(OAL_FUNC, (TEXT("_OEMMultiBINNotify.\r\n")));
}

 
/*
    @func   PVOID | GetKernelExtPointer | Locates the kernel region's extension area pointer.
    @rdesc  Pointer to the kernel's extension area.
    @comm
    @xref
*/
PVOID GetKernelExtPointer(DWORD dwRegionStart, DWORD dwRegionLength)
{
    DWORD dwCacheAddress = 0;
    ROMHDR *pROMHeader;
    DWORD dwNumModules = 0;
    TOCentry *pTOC;

    if (dwRegionStart == 0 || dwRegionLength == 0)
        return(NULL);

    if (*(LPDWORD) OEMMapMemAddr (dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET) != ROM_SIGNATURE)
        return NULL;

    // A pointer to the ROMHDR structure lives just past the ROM_SIGNATURE (which is a longword value).  Note that
    // this pointer is remapped since it might be a flash address (image destined for flash), but is actually cached
    // in RAM.
    //
    dwCacheAddress = *(LPDWORD) OEMMapMemAddr (dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET + sizeof(ULONG));
    pROMHeader     = (ROMHDR *) OEMMapMemAddr (dwRegionStart, dwCacheAddress);

    // Make sure sure are some modules in the table of contents.
    //
    if ((dwNumModules = pROMHeader->nummods) == 0)
        return NULL;

    // Locate the table of contents and search for the kernel executable and the TOC immediately follows the ROMHDR.
    //
    pTOC = (TOCentry *)(pROMHeader + 1);

    while(dwNumModules--) {
        LPBYTE pFileName = OEMMapMemAddr(dwRegionStart, (DWORD)pTOC->lpszFileName);
        EdbgOutputDebugString("[%d] Module Name: %s\r\n", dwNumModules, pFileName);
        if (!strcmp((const char *)pFileName, "nk.exe")) {
            return ((PVOID)(pROMHeader->pExtensions));
        }
        ++pTOC;
    }
    return NULL;
}


/*
    @func   BOOL | OEMDebugInit | Initializes the serial port for debug output message.
    @rdesc  TRUE == Success and FALSE == Failure.
    @comm
    @xref
*/
BOOL OEMDebugInit(void)
{

    // Set up function callbacks used by blcommon.
    //
    g_pOEMVerifyMemory   = OEMVerifyMemory;      // Verify RAM.
    g_pOEMMultiBINNotify = OEMMultiBINNotify;

    // Call serial initialization routine (shared with the OAL).
    //
    OEMInitDebugSerial();

    return(TRUE);
}

/*
    @func   void | SaveEthernetAddress | Save Ethernet Address on IMAGE_SHARE_ARGS_UA_START for Ethernet KITL
    @rdesc
    @comm
    @xref
*/
void    SaveEthernetAddress()
{
    memcpy(pBSPArgs->kitl.mac, g_pBootCfg->EdbgAddr.wMAC, 6);
    if (!(g_pBootCfg->ConfigFlags & CONFIG_FLAGS_DHCP))
    {
        // Static IP address.
        pBSPArgs->kitl.ipAddress  = g_pBootCfg->EdbgAddr.dwIP;
        pBSPArgs->kitl.ipMask     = g_pBootCfg->SubnetMask;
        pBSPArgs->kitl.flags     &= ~OAL_KITL_FLAGS_DHCP;
    }
    else
    {
        pBSPArgs->kitl.ipAddress  = g_DeviceAddr.dwIP;
        pBSPArgs->kitl.flags     |= OAL_KITL_FLAGS_DHCP;
    }
}
    //Set PWM GPIO to control Back-light  Regulator  Shotdown Pin (GPF[15])
#define BACKLIGHT_ON(gpioreg)  {  (gpioreg)##->GPFDAT |= (1<<15); \
                                    (gpioreg)##->GPFCON = ((gpioreg)##->GPFCON & ~(0x3<<30)) | (1<<30); }        // set GPF[15] as Output

#define BACKLIGHT_OFF(gpioreg)  {  (gpioreg)##->GPFDAT &= ~(1<<15);  \
                                    (gpioreg)##->GPDCON = ((gpioreg)##->GPDCON & ~(0x3<<30)) | (1<<30); }

static void InitializeDisplay(void)
{
    tDevInfo RGBDevInfo;

    volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    volatile S3C6410_DISPLAY_REG *pDispReg = (S3C6410_DISPLAY_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_DISPLAY, FALSE);
    volatile S3C6410_SPI_REG *pSPIReg = (S3C6410_SPI_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SPI0, FALSE);
    volatile S3C6410_MSMIF_REG *pMSMIFReg = (S3C6410_MSMIF_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_MSMIF_SFR, FALSE);

    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    EdbgOutputDebugString("[Eboot] ++InitializeDisplay()\r\n");

    // Initialize Display Power Gating
    if(!(pSysConReg->BLK_PWR_STAT & (1<<4))) {
        pSysConReg->NORMAL_CFG |= (1<<14);
        while(!(pSysConReg->BLK_PWR_STAT & (1<<4)));
        }

    // Initialize Virtual Address
    LDI_initialize_register_address((void *)pSPIReg, (void *)pDispReg, (void *)pGPIOReg);
    Disp_initialize_register_address((void *)pDispReg, (void *)pMSMIFReg, (void *)pGPIOReg);

    // Set LCD Module Type
#if        (SMDK6410_LCD_MODULE == LCD_MODULE_LTS222)
    LDI_set_LCD_module_type(LDI_LTS222QV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTV350)
    LDI_set_LCD_module_type(LDI_LTV350QV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTE480)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_D1)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_QV)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_PQV)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_ML)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMUL48_MP)
    LDI_set_LCD_module_type(LDI_LTE480WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTP700)
    LDI_set_LCD_module_type(LDI_LTP700WV_RGB);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_LTM030DK)
    LDI_set_LCD_module_type(LDI_LTM030DK_RGB);

#else
    EdbgOutputDebugString("[Eboot:ERR] InitializeDisplay() : Unknown Module Type [%d]\r\n", SMDK6410_LCD_MODULE);
#endif

    // Get RGB Interface Information from LDI Library
    LDI_fill_output_device_information(&RGBDevInfo);

    // Setup Output Device Information
    Disp_set_output_device_information(&RGBDevInfo);

    // Initialize Display Controller
    Disp_initialize_output_interface(DISP_VIDOUT_RGBIF);

#if        (LCD_BPP == 16)
    Disp_set_window_mode(DISP_WIN1_DMA, DISP_16BPP_565, LCD_WIDTH, LCD_HEIGHT, 0, 0);
#elif    (LCD_BPP == 32)    // XRGB format (RGB888)
    Disp_set_window_mode(DISP_WIN1_DMA, DISP_24BPP_888, LCD_WIDTH, LCD_HEIGHT, 0, 0);
#else
    EdbgOutputDebugString("[Eboot:ERR] InitializeDisplay() : Unknown Color Depth %d bpp\r\n", LCD_BPP);
#endif

    Disp_set_framebuffer(DISP_WIN1, EBOOT_FRAMEBUFFER_PA_START);
    Disp_window_onfoff(DISP_WIN1, DISP_WINDOW_ON);

#if    (SMDK6410_LCD_MODULE == LCD_MODULE_LTS222)
    // This type of LCD need MSM I/F Bypass Mode to be Disabled
    pMSMIFReg->MIFPCON &= ~(0x1<<3);    // SEL_BYPASS -> Normal Mode
#endif

#if (SMDK6410_LCD_MODULE == LCD_MODULE_LTM030DK)
    // This type of LCD need to initialize Backlight using IIC


    LDI_LTM030DK_port_initialize();
//    I2c_eboot_Init();
//    LCD_SetALC();
    LDI_LTM030DK_RGB_initialize();

#else

    // Initialize LCD Module
    LDI_initialize_LCD_module();
#endif

    // LCD Clock Source as MPLL_Dout
    pSysConReg->CLK_SRC = (pSysConReg->CLK_SRC & ~(0xFFFFFFF0))
                            |(0<<31)    // TV27_SEL    -> 27MHz
                            |(0<<30)    // DAC27        -> 27MHz
                            |(0<<28)    // SCALER_SEL    -> MOUT_EPLL
                            |(1<<26)    // LCD_SEL    -> Dout_MPLL
                            |(0<<24)    // IRDA_SEL    -> MOUT_EPLL
                            |(0<<22)    // MMC2_SEL    -> MOUT_EPLL
                            |(0<<20)    // MMC1_SEL    -> MOUT_EPLL
                            |(0<<18)    // MMC0_SEL    -> MOUT_EPLL
                            |(0<<16)    // SPI1_SEL    -> MOUT_EPLL
                            |(0<<14)    // SPI0_SEL    -> MOUT_EPLL
                            |(0<<13)    // UART_SEL    -> MOUT_EPLL
                            |(0<<10)    // AUDIO1_SEL    -> MOUT_EPLL
                            |(0<<7)        // AUDIO0_SEL    -> MOUT_EPLL
                            |(0<<5)        // UHOST_SEL    -> 48MHz
                            |(0<<4);        // MFCCLK_SEL    -> HCLKx2 (0:HCLKx2, 1:MoutEPLL)

    // Video Output Enable
    Disp_envid_onoff(DISP_ENVID_ON);

    // Fill Framebuffer
#if    (SMDK6410_LCD_MODULE == LCD_MODULE_LTV350)
    memcpy((void *)EBOOT_FRAMEBUFFER_UA_START, (void *)InitialImage_rgb16_320x240, 320*240*2);
#elif    (SMDK6410_LCD_MODULE == LCD_MODULE_EMULQV)
    memcpy((void *)EBOOT_FRAMEBUFFER_UA_START, (void *)InitialImage_rgb16_320x240, 320*240*2);
#elif    (LCD_BPP == 16)
    {
        int i;
        unsigned short *pFB;
        pFB = (unsigned short *)EBOOT_FRAMEBUFFER_UA_START;

        for (i=0; i<LCD_WIDTH*LCD_HEIGHT; i++)
            *pFB++ = 0x001F;        // Blue
    }
#elif    (LCD_BPP == 32)
    {
        int i;
        unsigned int *pFB;
        pFB = (unsigned int *)EBOOT_FRAMEBUFFER_UA_START;

        for (i=0; i<LCD_WIDTH*LCD_HEIGHT; i++)
            *pFB++ = 0x000000FF;        // Blue
    }
#endif
    // Backlight Power On
    BACKLIGHT_ON(pGPIOReg);

    EdbgOutputDebugString("[Eboot] --InitializeDisplay()\r\n");
}

static void SpinForever(void)
{
    EdbgOutputDebugString("SpinForever...\r\n");

    while(1)
    {
        ;
    }
}


BOOL OEMSerialSendRaw(LPBYTE pbFrame, USHORT cbFrame)
{
    UINT16 byteSent;

    while (cbFrame) {
        byteSent = Serial_Send(pbFrame, cbFrame);
        cbFrame -= byteSent;
        pbFrame += byteSent;
    }
    return TRUE;
}


#define RETRY_COUNT             100000

BOOL OEMSerialRecvRaw(LPBYTE pbFrame, PUSHORT pcbFrame, BOOLEAN bWaitInfinite)
{
    USHORT byteToRecv = *pcbFrame;
    UINT16 byteRecv;
    UINT Retries = 0;

    while (byteToRecv) {
        byteRecv = Serial_Recv(pbFrame, byteToRecv);

        if (!bWaitInfinite) {
            // check retry count is we don't want to wait infinite
            // we only return false if we have not receive any data and retry more than RETRY_COUNT times
            if ((byteRecv == 0) && (byteToRecv == *pcbFrame)) {
                Retries++;
                if (Retries > RETRY_COUNT) {
                    *pcbFrame = 0;
                    return FALSE;
                }
            }
        }
        byteToRecv -= byteRecv;
        pbFrame += byteRecv;
    }
    return TRUE;
}


//--------------------------------------------------------------------
//48MHz clock source for usb host1.1, IrDA, hsmmc, spi is shared with otg phy clock.
//So, initialization and reset of otg phy shoud be done on initial booting time.
//--------------------------------------------------------------------
static void InitializeOTGCLK(void)
{
    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
    volatile OTG_PHY_REG *pOtgPhyReg = (OTG_PHY_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_USBOTG_PHY, FALSE);

    OTGDEV_SetSoftDisconnect();

    pSysConReg->HCLK_GATE |= (1<<20);

    pSysConReg->OTHERS |= (1<<16);

    pOtgPhyReg->OPHYPWR = 0x0;  // OTG block, & Analog bock in PHY2.0 power up, normal operation
    pOtgPhyReg->OPHYCLK = 0x20; // Externel clock/oscillator, 48MHz reference clock for PLL
    pOtgPhyReg->ORSTCON = 0x1;
    Delay(100);    
    pOtgPhyReg->ORSTCON = 0x0;
    Delay(100);    

    pSysConReg->HCLK_GATE &= ~(1<<20);
}


static void InitializeRTC(void)
{
    volatile S3C6410_RTC_REG *pRTCReg = (S3C6410_RTC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_RTC, FALSE);

    // As per the S3C6410 User Manual, the RTC clock divider should be reset for exact RTC operation.

    // Enable RTC control first
    pRTCReg->RTCCON |= (1<<0);

    // Pulse the RTC clock divider reset
    pRTCReg->RTCCON |= (1<<3);
    pRTCReg->RTCCON &= ~(1<<3);

    // The value of BCD registers in the RTC are undefined at reset. Set them to a known value
    pRTCReg->BCDSEC  = 0;
    pRTCReg->BCDMIN  = 0;
    pRTCReg->BCDHOUR = 0;
    pRTCReg->BCDDATE = 1;
    pRTCReg->BCDDAY  = 1;
    pRTCReg->BCDMON  = 1;
    pRTCReg->BCDYEAR = 0;

    // Disable RTC control.
    pRTCReg->RTCCON &= ~(1<<0);
}
