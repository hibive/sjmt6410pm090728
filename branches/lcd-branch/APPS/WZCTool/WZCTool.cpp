//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

    wzctool.cpp

Abstract:  

    Program to test WZC (wireless zero config) service.
    wzctool.exe uses WZC APIs.


Environment:


Revision History:

    05/04/2001 -  Initial version
    06/21/2002 -  Added -c option
    03/23/2004 -  Correcting broken features, adding peap, and other general options
                  re-wrote for WINDOWS CE.
   
--*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <eapol.h>
#include <wzcsapi.h>
#include <ntddndis.h>

// utility macro to convert a hexa digit into its value
#define HEX(c)  ((c)<='9'?(c)-'0':(c)<='F'?(c)-'A'+0xA:(c)-'a'+0xA)


WCHAR* g_szAuthenticationMode[] =
{
    L"Ndis802_11AuthModeOpen",
    L"Ndis802_11AuthModeShared",
    L"Ndis802_11AuthModeAutoSwitch",
    L"Ndis802_11AuthModeWPA",
    L"Ndis802_11AuthModeWPAPSK",
    L"Ndis802_11AuthModeWPANone",
    L"Ndis802_11AuthModeWPA2",
    L"Ndis802_11AuthModeWPA2PSK"
};

WCHAR* g_szcPrivacyMode[] =
{
    TEXT("Ndis802_11WEPEnabled"),                                                         
    TEXT("Ndis802_11WEPDisabled"),
    TEXT("Ndis802_11WEPKeyAbsent"),
    TEXT("Ndis802_11WEPNotSupported"),
    TEXT("Ndis802_11Encryption2Enabled"),
    TEXT("Ndis802_11Encryption2KeyAbsent"),
    TEXT("Ndis802_11Encryption3Enabled"),
    TEXT("Ndis802_11Encryption3KeyAbsent")
};


WCHAR g_WirelessCard1[MAX_PATH] = L"";    // 1st wireless card found by WZC query




int
WasOption
// look for argument like '-t' or '/t'.
// returns option index
// returns index of argv[] found, -1 if not found
(
    IN int argc,       // number of args
    IN WCHAR* argv[],  // arg array
    IN WCHAR* szOption // to find ('t')
)
{
    for(int i=0; i<argc; i++)
    {
        if( ((*argv[i] == L'-') || (*argv[i] == L'/')) &&
            !wcscmp(argv[i]+1, szOption))
            return i;
    }
    return -1;
}   // WasOption()



int
GetOption
// look for argument like '-t 100' or '/t 100'.
// returns index of '100' if option ('t') is found
// returns -1 if not found
(
    int argc,                // number of args
    IN WCHAR* argv[],        // arg array
    IN WCHAR* pszOption,     // to find ('n')
    OUT WCHAR** ppszArgument // option value ('100')
)
{
    if(!ppszArgument)
        return -1;

    int i = WasOption(argc, argv, pszOption);
    if((i < 0) || ((i+1) >= argc))
    {
        *ppszArgument = NULL;
        return -1;
    }

    *ppszArgument = argv[i+1];
    return i+1;
}   // GetOption()



void
ShowTextMessages
(
    IN WCHAR **szMsgBlock
)
{
    for(WCHAR **sz1=szMsgBlock; *sz1; sz1++)
        wprintf(L"%s", *sz1);
}




void
EnumWirelessNetworkCard
// enumerate wireless network cards detected by WZC
(
   // arg none
)
{
    INTFS_KEY_TABLE IntfsTable;
    IntfsTable.dwNumIntfs = 0;
    IntfsTable.pIntfs = NULL;

    DWORD dwStatus = WZCEnumInterfaces(NULL, &IntfsTable);

    if(dwStatus != ERROR_SUCCESS)
    {
        wprintf(L"WZCEnumInterfaces() error 0x%08X\n", dwStatus);
        return;        
    }

    // print the GUIDs
    // note that in CE the GUIDs are simply the device instance name
    // i.e XWIFI11B1, CISCO1, ISLP21, ...
    //

    if(!IntfsTable.dwNumIntfs)
    {
        wprintf(L"system has no wireless card.\n");
        return;
    }

    for(unsigned int i=0; i < IntfsTable.dwNumIntfs; i++)
        wprintf(L"wifi-card [%d] = %s\n", i, IntfsTable.pIntfs[i].wszGuid);

    LocalFree(IntfsTable.pIntfs); // need to free memory allocated by WZC for us.
}   // EnumWirelessNetworkCard





void
GetFirstWirelessNetworkCard
// find the first wireless network cards
// found card name goes to g_WirelessCard1
(
   // arg none
)
{
    g_WirelessCard1[0] = L'\0';

    INTFS_KEY_TABLE IntfsTable;
    IntfsTable.dwNumIntfs = 0;
    IntfsTable.pIntfs = NULL;

    DWORD dwStatus = WZCEnumInterfaces(NULL, &IntfsTable);

    if(dwStatus != ERROR_SUCCESS)
    {
        wprintf(L"WZCEnumInterfaces() error 0x%08X\n", dwStatus);
        return;        
    }

    // print the GUIDs
    // note that in CE the GUIDs are simply the device instance name
    // i.e XWIFI11B1, CISCO1, ISLP2, ...
    //

    if(!IntfsTable.dwNumIntfs)
    {
        wprintf(L"system has no wireless card.\n");
        return;
    }

    wcsncpy(g_WirelessCard1, IntfsTable.pIntfs[0].wszGuid, MAX_PATH-1);
    wprintf(L"wireless card found: %s\n", g_WirelessCard1);

    // need to free memory allocated by WZC for us.
    LocalFree(IntfsTable.pIntfs);
}   // GetFirstWirelessNetworkCard




void
PrintMacAddress
// some RAW_DATA is a MAC ADDRESS, this function is for printing MAC ADDRESS
(
    IN PRAW_DATA prdMAC
)
{
    if (prdMAC == NULL || prdMAC->dwDataLen == 0)
        wprintf(L"<NULL>");
    else if(prdMAC->dwDataLen != 6)
        wprintf(L"<INVLID MAC>");
    else
    {
        wprintf(L"%02X:%02X:%02X:%02X:%02X:%02X",
            prdMAC->pData[0],
            prdMAC->pData[1],
            prdMAC->pData[2],
            prdMAC->pData[3],
            prdMAC->pData[4],
            prdMAC->pData[5]);
    }
}   // PrintMacAddress()




void 
PrintSSID
// some RAW_DATA is a SSID, this function is for printing SSID
(
    PRAW_DATA prdSSID   // RAW SSID data
)
{
    if (prdSSID == NULL || prdSSID->dwDataLen == 0)
        wprintf(L"<NULL>");
    else
    {
        WCHAR szSsid[33];
        for (UINT i = 0; i < prdSSID->dwDataLen; i++)
            szSsid[i] = prdSSID->pData[i];
        szSsid[i] = L'\0';
        wprintf(L"%s", szSsid);
    }

}	//	PrintSSID()




WCHAR g_szSupportedRate[32];// = { L"1", L"2", L"5.5", L"11", L"" };   // Mbit/s

WCHAR*
SupportedRate
// rate values in WCHAR string
(
    IN BYTE ucbRawValue
)
{
    double fRate = ((double)(ucbRawValue & 0x7F)) * 0.5;
    swprintf(g_szSupportedRate, L"%.1f", fRate);
    return g_szSupportedRate;
}   // SupportedRate()




UINT
ChannelNumber
//
// calculate 802.11b channel number for given frequency
// return 1-14 based on the given ulFrequency_kHz
// return 0 for invalid frequency range
//
// 2412 MHz = ch-1
// 2417 MHz = ch-2
// 2422 MHz = ch-3
// 2427 MHz = ch-4
// 2432 MHz = ch-5
// 2437 MHz = ch-6
// 2442 MHz = ch-7
// 2447 MHz = ch-8
// 2452 MHz = ch-9
// 2457 MHz = ch-10
// 2462 MHz = ch-11
// 2467 MHz = ch-12
// 2472 MHz = ch-13
// 2484 MHz = ch-14
//
(
    IN ULONG ulFrequency_kHz // frequency in kHz
)
{
#ifdef	OMNIBOOK_VER
	ULONG ulFrequency_MHz = ulFrequency_kHz;
#else	//!OMNIBOOK_VER
    ULONG ulFrequency_MHz = ulFrequency_kHz/1000;
#endif	OMNIBOOK_VER
    if((2412<=ulFrequency_MHz) && (ulFrequency_MHz<2484))
        return ((ulFrequency_MHz-2412)/5)+1;
    else if(ulFrequency_MHz==2484)
        return 14;
    return 0;   // invalid channel number
}   // ChannelNumber()



void
PrintConfigList
// print WZC configuration list
// used when printing [Available Networks] and [Preferred Networks]
(
    PRAW_DATA prdBSSIDList
)
{
    if (prdBSSIDList == NULL || prdBSSIDList->dwDataLen == 0)
    {
        wprintf(L"<NULL> entry.");
    }
    else
    {
        PWZC_802_11_CONFIG_LIST pConfigList = (PWZC_802_11_CONFIG_LIST)prdBSSIDList->pData;

        wprintf(L"[%d] entries.\n\n", pConfigList->NumberOfItems);

        for (UINT i = 0; i < pConfigList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pConfig = &(pConfigList->Config[i]);

            wprintf(L"******** List Entry Number [%d] ********\n", i);
            wprintf(L"   Length                  = %d bytes.\n", pConfig->Length);
            wprintf(L"   dwCtlFlags              = 0x%08X\n", pConfig->dwCtlFlags);
            wprintf(L"   MacAddress              = %02X:%02X:%02X:%02X:%02X:%02X\n",
                            pConfig->MacAddress[0],
                            pConfig->MacAddress[1],
                            pConfig->MacAddress[2],
                            pConfig->MacAddress[3],
                            pConfig->MacAddress[4],
                            pConfig->MacAddress[5]);
            wprintf(L"   SSID                    = ");
            RAW_DATA rdBuffer;
            rdBuffer.dwDataLen = pConfig->Ssid.SsidLength;
            rdBuffer.pData     = pConfig->Ssid.Ssid;
            PrintSSID(&rdBuffer);
            wprintf(L"\n");

            wprintf(L"   Privacy                 = %d  ", pConfig->Privacy);
            if(pConfig->Privacy == 1)
                wprintf(L"Privacy disabled (wireless data is not encrypted)\n");
            else
                wprintf(L"Privacy enabled (encrypted with [%s])\n", g_szcPrivacyMode[pConfig->Privacy]);

            wprintf(L"   RSSI                    = %d dBm (0=excellent, -100=weak signal)\n", pConfig->Rssi);

            wprintf(L"   NetworkTypeInUse        = %s\n",
                (pConfig->NetworkTypeInUse == Ndis802_11FH) ? L"NDIS802_11FH" :
                (pConfig->NetworkTypeInUse == Ndis802_11DS) ?
                L"NDIS802_11DS" : L"<UNKNOWN! SHOULD NOT BE!!>");

            ////////////////////////////////////////////////////////////////////
            wprintf(L"   Configuration:\n");
            wprintf(L"      Struct Length    = %d\n", pConfig->Configuration.Length);
            wprintf(L"      BeaconPeriod     = %d kusec\n", pConfig->Configuration.BeaconPeriod);
            wprintf(L"      ATIMWindow       = %d kusec\n", pConfig->Configuration.ATIMWindow);
            wprintf(L"      DSConfig         = %d kHz (ch-%d)\n", pConfig->Configuration.DSConfig, ChannelNumber(pConfig->Configuration.DSConfig));
            wprintf(L"      FHConfig:\n");
            wprintf(L"         Struct Length = %d\n" ,pConfig->Configuration.FHConfig.Length);
            wprintf(L"         HopPattern    = %d\n", pConfig->Configuration.FHConfig.HopPattern);
            wprintf(L"         HopSet        = %d\n", pConfig->Configuration.FHConfig.HopSet);
            wprintf(L"         DwellTime     = %d\n", pConfig->Configuration.FHConfig.DwellTime);

            wprintf(L"   Infrastructure          = %s\n",
                        (pConfig->InfrastructureMode == Ndis802_11IBSS) ? L"NDIS802_11IBSS" :
                        (pConfig->InfrastructureMode == Ndis802_11Infrastructure) ? L"Ndis802_11Infrastructure" :
                        (pConfig->InfrastructureMode == Ndis802_11AutoUnknown) ? L"Ndis802_11AutoUnknown" :
                        L"<UNKNOWN! SHOULD NOT BE!!>");

            wprintf(L"   SupportedRates          = ");
            for(int j=0; j<8; j++)
            {
                if(pConfig->SupportedRates[j])
                    wprintf(L"%s,", SupportedRate(pConfig->SupportedRates[j]));
            }
            wprintf(L" (Mbit/s)\n");

            wprintf(L"   KeyIndex                = <not available> (beaconing packets don't have this info)\n"); // pConfig->KeyIndex
            wprintf(L"   KeyLength               = <not available> (beaconing packets don't have this info)\n"); // pConfig->KeyLength
            wprintf(L"   KeyMaterial             = <not available> (beaconing packets don't have this info)\n");

            ////////////////////////////////////////////////////////////////////

            wprintf(L"   Authentication          = %d  ", pConfig->AuthenticationMode);
            if(pConfig->AuthenticationMode < Ndis802_11AuthModeMax)
                wprintf(L"%s\n", g_szAuthenticationMode[pConfig->AuthenticationMode]);
            else
                wprintf(L"<unknown>\n");

            ////////////////////////////////////////////////////////////////////

            wprintf(L"   rdUserData length       = %d bytes.\n",  pConfig->rdUserData.dwDataLen);
			
        }
    }
}   // PrintConfigList()

#ifdef UNDER_CE

void
DoQueryCache
// Query the WiFi card PMK Cache.
// wzctool -cache cisco1
//     query cisco1 adapter parameters.
// wzctool -cache
//     find wireless card, and query for that adapter.
(
	WCHAR *szWiFiCard
)
{
    INTF_ENTRY_EX Intf;
    DWORD dwOutFlags;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;
    
    DWORD dwStatus = WZCQueryInterfaceEx(NULL, INTF_PMKCACHE, &Intf, &dwOutFlags);
    if (dwStatus != ERROR_SUCCESS)
    {
        wprintf(L"WZCQueryInterfaceEx() error 0x%08X\n", dwStatus);
        return;
    }
	DWORD Flags = Intf.PMKCacheFlags;
	wprintf(L"PMKCache: Enabled=%hs Opportunistic=%hs Preauth=%hs\n", 
		(Flags & INTF_ENTRY_PMKCACHE_FLAG_ENABLE) ? "ON" : "OFF",
		(Flags & INTF_ENTRY_PMKCACHE_FLAG_ENABLE_OPPORTUNISTIC) ? "ON" : "OFF",
		(Flags & INTF_ENTRY_PMKCACHE_FLAG_ENABLE_PREAUTH) ? "ON" : "OFF"
	);

	if (Intf.rdPMKCache.dwDataLen)
	{
		PNDIS_802_11_PMKID	pCache = (PNDIS_802_11_PMKID)Intf.rdPMKCache.pData;
        DWORD               i;
	
		wprintf (L"   BSSIDInfoCount           : [%u]\n", pCache->BSSIDInfoCount);

        for (i = 0 ; i < pCache->BSSIDInfoCount; i++)
        {
			PBYTE pMac = &pCache->BSSIDInfo[i].BSSID[0];
			PBYTE pId  = &pCache->BSSIDInfo[i].PMKID[0];
            wprintf (L"      BSSID=%02X%02X%02X%02X%02X%02X  PMKID=%02X%02X..%02X%02X\n", 
				pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5],
				pId[0], pId[1], pId[14], pId[15]);
        }        
	}

	//
    // Free memory block that WZC allocated.
    // WZCQueryInterfaceEx() always should be followed by this WZCDeleteIntfObj().
    // Note that the wzcGuid is ** NOT ** freed by WZCDeleteIntfObj()
    //	

    WZCDeleteIntfObjEx(&Intf);

}

#endif

void
DoQuery
// Query the WiFi card and print out the info available for the interface.
// wzctool -q cisco1
//     query cisco1 adapter parameters.
// wzctool -q
//     find wireless card, and query for that adapter.
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    WCHAR *szWiFiCard = NULL;
    if(GetOption(argc, argv, L"q", &szWiFiCard) < 0)
    {
        GetFirstWirelessNetworkCard();
        if(!*g_WirelessCard1)    // wifi card not found
            return;
        szWiFiCard = g_WirelessCard1;
    }
        
    INTF_ENTRY_EX Intf;
    DWORD dwOutFlags;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;
    
    DWORD dwStatus = WZCQueryInterfaceEx(
                        NULL,
                        INTF_ALL,
                        &Intf,
                        &dwOutFlags);

    if (dwStatus != ERROR_SUCCESS)
    {
        wprintf(L"WZCQueryInterfaceEx() error 0x%08X\n", dwStatus);
        return;
    }

    wprintf(L"WZCQueryInterfaceEx() for %s\n", szWiFiCard);
    wprintf(L"In flags used       = [0x%08X]\n", INTF_ALL);
    wprintf(L"Returned out flags  = [0x%08X]\n", dwOutFlags);

    // GUID (in CE, GUID=instance name)
    wprintf(L"wzcGuid             = [%s]\n", Intf.wszGuid);

    //	Description
    wprintf(L"wzcDescr            = [%s]\n", Intf.wszDescr);

    //	Print BSSID. BSSID is the MAC address of the AP I am connected.
    if (dwOutFlags & INTF_BSSID)
    {
        wprintf(L"BSSID = ");
        PrintMacAddress(&Intf.rdBSSID);

        PRAW_DATA prdMAC = &Intf.rdBSSID;
        if (
            prdMAC == NULL ||
            (prdMAC->dwDataLen == 0) ||
            (prdMAC->dwDataLen != 6) ||
            (   !prdMAC->pData[0] &&
                !prdMAC->pData[1] &&
                !prdMAC->pData[2] &&
                !prdMAC->pData[3] &&
                !prdMAC->pData[4] &&
                !prdMAC->pData[5]     )
        )
            wprintf(L" (this wifi card is not associated to any)\n");
        else
            wprintf(L" (this wifi card is associated state)\n");
    }
    else
        wprintf(L"BSSID = <unknown> (not connected)\n");

    //	Media Type
    if (dwOutFlags & INTF_NDISMEDIA)
        wprintf(L"Media Type          = [%d]\n", Intf.ulMediaType);
    else
        wprintf(L"Media Type          = <unknown>\n");

    //	Configuration Mode
    if (dwOutFlags & INTF_ALL_FLAGS)
    {
        wprintf(L"Configuration Mode  = [%08X]\n", Intf.dwCtlFlags);
        if(Intf.dwCtlFlags & INTFCTL_ENABLED)
            wprintf(L"   zero conf enabled for this interface\n");
        if(Intf.dwCtlFlags & INTFCTL_FALLBACK)
            wprintf(L"   attempt to connect to visible non-preferred networks also\n");
        if(Intf.dwCtlFlags & INTFCTL_OIDSSUPP)
            wprintf(L"   802.11 OIDs are supported by the driver/firmware\n");
        if(Intf.dwCtlFlags & INTFCTL_VOLATILE)
            wprintf(L"   the service parameters are volatile\n");
        if(Intf.dwCtlFlags & INTFCTL_POLICY)
            wprintf(L"   the service parameters policy enforced\n");
    }
    else
        wprintf(L"Configuration Mode  = <unknown>\n");

    //	Print Infrastructure Mode
    if (dwOutFlags & INTF_INFRAMODE)
    {
        wprintf(L"Infrastructure Mode = [%d]  ", Intf.nInfraMode);
        if(Intf.nInfraMode == Ndis802_11IBSS)
            wprintf(L"IBSS net (adhoc net)\n");
        else if(Intf.nInfraMode == Ndis802_11Infrastructure)
            wprintf(L"Infrastructure net (connected to an Access Point)\n");
        else
            wprintf(L"Ndis802_11AutoUnknown\n");
    }
    else
        wprintf(L"Infrastructure Mode = <unknown>\n");


    //	Print Authentication Mode
    if (dwOutFlags & INTF_AUTHMODE)
    {
        wprintf(L"Authentication Mode = [%d]  ", Intf.nAuthMode);

        if((DWORD)Intf.nAuthMode < Ndis802_11AuthModeMax)
            wprintf(L"%s\n", g_szAuthenticationMode[Intf.nAuthMode]);
        else
            wprintf(L"<unknown>\n");
    }
    else
        wprintf(L"Authentication Mode = <unknown>\n");

	wprintf(L"rdNicCapabilities   = %d bytes\n", Intf.rdNicCapabilities.dwDataLen);

	if (Intf.rdNicCapabilities.dwDataLen)
	{
		PINTF_80211_CAPABILITY	pCapability = (PINTF_80211_CAPABILITY)Intf.rdNicCapabilities.pData;
        DWORD                   i;
	
		wprintf (L"   dwNumOfPMKIDs            : [%d]\n", pCapability->dwNumOfPMKIDs);
		wprintf (L"   dwNumOfAuthEncryptPairs  : [%d]\n", pCapability->dwNumOfAuthEncryptPairs);

        for (i = 0 ; i < pCapability->dwNumOfAuthEncryptPairs ; i++)
        {
            wprintf (L"      Pair[%d]\n", i+1);

            wprintf (L"         AuthmodeSupported          [%s]\n", 
                g_szAuthenticationMode[pCapability->AuthEncryptPair[i].AuthModeSupported]);
            
            wprintf (L"         EncryptStatusSupported     [%s]\n", 
                g_szcPrivacyMode[pCapability->AuthEncryptPair[i].EncryptStatusSupported]);
        }        
	}		

#ifdef UNDER_CE
	wprintf(L"rdPMKCache   = %u bytes\n", Intf.rdPMKCache.dwDataLen);

	if (Intf.rdPMKCache.dwDataLen)
	{
		PNDIS_802_11_PMKID	pCache = (PNDIS_802_11_PMKID)Intf.rdPMKCache.pData;
        DWORD                   i;
	
		wprintf (L"   BSSIDInfoCount           : [%u]\n", pCache->BSSIDInfoCount);

        for (i = 0 ; i < pCache->BSSIDInfoCount; i++)
        {
			PBYTE pMac = &pCache->BSSIDInfo[i].BSSID[0];
			PBYTE pId  = &pCache->BSSIDInfo[i].PMKID[0];
            wprintf (L"      BSSID=%02X%02X%02X%02X%02X%02X  PMKID=%02X%02X..%02X%02X\n", 
				pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5],
				pId[0], pId[1], pId[14], pId[15]);
        }        
	}		
#endif

    //	Print WEP status
    if (dwOutFlags & INTF_WEPSTATUS)
    {
        wprintf(L"WEP Status          = [%d]  ", Intf.nWepStatus);

        WCHAR* szWepStatus[] =
        {
            L"Ndis802_11WEPEnabled",
            L"Ndis802_11WEPDisabled",
            L"Ndis802_11WEPKeyAbsent",
            L"Ndis802_11WEPNotSupported"
#ifdef	OMNIBOOK_VER
			L"Ndis802_11Encryption2Enabled",
			L"Ndis802_11Encryption2KeyAbsent",
			L"Ndis802_11Encryption3Enabled",
			L"Ndis802_11Encryption3KeyAbsent"
#endif	OMNIBOOK_VER
        };

#ifdef	OMNIBOOK_VER
		if(Intf.nWepStatus < 8)
#else	//!OMNIBOOK_VER
        if(Intf.nWepStatus < 4)
#endif	OMNIBOOK_VER
            wprintf(L"%s\n", szWepStatus[Intf.nWepStatus]);
        else
            wprintf(L"<unknown value>\n");
    }
    else
        wprintf(L"WEP Status          = <unknown>\n");

    //	Print SSID status
    if (dwOutFlags & INTF_SSID)
    {
        wprintf(L"SSID = ");
        PrintSSID(&Intf.rdSSID);
        wprintf(L"\n");
    }
    else
        wprintf(L"SSID = <unknown>\n");


    if (dwOutFlags & INTF_CAPABILITIES)
    {
        wprintf(L"Capabilities =\n");
        if(Intf.dwCapabilities & INTFCAP_SSN)
            wprintf(L"     WPA/TKIP capable\n");
        if(Intf.dwCapabilities & INTFCAP_80211I)
            wprintf(L"     WPA2/AES capable\n");
        
    }

    wprintf(L"\n");
    wprintf(L"[Available Networks] SSID List ");
    PrintConfigList(&Intf.rdBSSIDList);


    wprintf(L"\n");
    wprintf(L"[Preferred Networks] SSID List ");
    PrintConfigList(&Intf.rdStSSIDList);

    wprintf(L"\n");
    wprintf(L"rdCtrlData length   = %d bytes\n", Intf.rdCtrlData.dwDataLen);		

    //
    // Free memory block that WZC allocated.
    // WZCQueryInterfaceEx() always should be followed by this WZCDeleteIntfObj().
    // Note that the wzcGuid is ** NOT ** freed by WZCDeleteIntfObj()
    //	

    WZCDeleteIntfObjEx(&Intf);

    //
    //  Get context information.
    //
    wprintf(L"\n");
    wprintf(L"parameter setting in Zero Config\n");

    WZC_CONTEXT WzcContext;
    dwStatus = WZCQueryContext(NULL, 0x00, &WzcContext, NULL);
    if (dwStatus != ERROR_SUCCESS)
        wprintf(L"!!! Failed WZCQueryContext.  Err = [0x%08X] !!!\n", dwStatus);
    else
    {
        wprintf(L"tmTr = %d mili-seconds (Scan time out)\n", WzcContext.tmTr);
        wprintf(L"tmTp = %d mili-seconds (Association time out)\n", WzcContext.tmTp);
        wprintf(L"tmTc = %d mili-seconds (Periodic scan when connected)\n", WzcContext.tmTc);
        wprintf(L"tmTf = %d mili-seconds (Periodic scan when disconnected)\n", WzcContext.tmTf);
    }
}   // DoQuery()




void
EncryptWepKMaterial
// encrypt WEP key material
// note: this is simply for the security (to protect from memory scanning)
(
    IN OUT WZC_WLAN_CONFIG* pwzcConfig
)
{
    BYTE chFakeKeyMaterial[] = { 0x56, 0x09, 0x08, 0x98, 0x4D, 0x08, 0x11, 0x66, 0x42, 0x03, 0x01, 0x67, 0x66 };
    for (int i = 0; i < WZCCTL_MAX_WEPK_MATERIAL; i++)
        pwzcConfig->KeyMaterial[i] ^= chFakeKeyMaterial[(7*i)%13];
}   // EncryptWepKMaterial()




void
InterpretEncryptionKeyValue
// interpret key value then fill wzcConfig1.KeyLength and KeyMaterial[]
// wzcConfig1.Privacy should be initialized before calling.
// key is interpreted differently based on the wzcConfig1.Privacy
// wzcConfig1.Privacy could be one of these
//      Ndis802_11WEPEnabled = WEP key
//      Ndis802_11Encryption2Enabled = TKIP/WPA key
(
    IN OUT WZC_WLAN_CONFIG& wzcConfig1,
    IN WCHAR *szEncryptionKey,
    IN BOOL& bNeed8021X // this becomes TRUE if szEncryptionKey is "auto"
)
{
    if(wzcConfig1.Privacy == Ndis802_11WEPEnabled)
    {
        // WEP key : valid forms are
        // -key 1/0x1234567890 [index=1,40-bit(10-digit hexa)]
        // -key 4/zxcvb [index=4, 40-bit(5-char)
        // -key 3/0x12345678901234567890123456 [index=3, 104-bit(26-digit hexa)
        // -key 2/abcdefghij123 [index=2, 104-bit(13-char)
        // -key auto    [key comes from successful EAP]
        if(!_wcsicmp(szEncryptionKey, L"auto"))
            bNeed8021X = TRUE;
        else
        {
            if((szEncryptionKey[0] < L'1') || (szEncryptionKey[0] > L'4') || (szEncryptionKey[1]!=L'/'))
            {
                wprintf(L"invalid key index\n");
                return;
            }
            wzcConfig1.KeyIndex = szEncryptionKey[0] - L'1';

            WCHAR *szEncryptionKeyValue = szEncryptionKey + 2;
            wzcConfig1.KeyLength = wcslen(szEncryptionKeyValue);
            if((wzcConfig1.KeyLength==5) || (wzcConfig1.KeyLength==13))
            {
                for(UINT i=0; i<wzcConfig1.KeyLength; i++)
                    wzcConfig1.KeyMaterial[i] = (UCHAR) szEncryptionKeyValue[i];
            }
            else
            {
                if((szEncryptionKeyValue[0]!=L'0') || (szEncryptionKeyValue[1]!=L'x'))
                {
                    wprintf(L"invalid key value\n");
                    return;
                }
                szEncryptionKeyValue += 2;
                wzcConfig1.KeyLength = wcslen(szEncryptionKeyValue);

                if((wzcConfig1.KeyLength!=10) && (wzcConfig1.KeyLength!=26))
                {
                    wprintf(L"invalid key value\n");
                    return;
                }

                wzcConfig1.KeyLength >>= 1;
                for(UINT i=0; i<wzcConfig1.KeyLength; i++)
                    wzcConfig1.KeyMaterial[i] = (HEX(szEncryptionKeyValue[2*i])<<4) | HEX(szEncryptionKeyValue[2*i+1]);
            }
            EncryptWepKMaterial(&wzcConfig1);
            wzcConfig1.dwCtlFlags |= WZCCTL_WEPK_PRESENT;
        }
    }
    else if(wzcConfig1.Privacy == Ndis802_11Encryption2Enabled
	     || wzcConfig1.Privacy == Ndis802_11Encryption3Enabled)
    {
        // TKIP key
        // -key 12345678   [8-char]
        // -key HelloWorld [10-char]
        // -key abcdefghij1234567890abcdefghij1234567890abcdefghij1234567890abc [63-char]
        // -key auto    [key comes from successful EAP]

        if(!_wcsicmp(szEncryptionKey, L"auto"))
            bNeed8021X = TRUE;
        else
        {
            wzcConfig1.KeyLength = wcslen(szEncryptionKey);
            if((wzcConfig1.KeyLength<8) || (wzcConfig1.KeyLength>63))
            {
                wprintf(L"WPA-PSK/TKIP key should be 8-63 char long string\n");
                return;
            }

            // WPA/TKIP pre-shared key takes 256 bit key.
            // Everything else is incorrect format.
            // Translates a user password (8 to 63 ascii chars) into a 256 bit network key.
            // We do this for WPA-PSK and WPA-None.

            char szEncryptionKeyValue8[64]; // longest key is 63
            memset(szEncryptionKeyValue8, 0, sizeof(szEncryptionKeyValue8));
            WideCharToMultiByte(CP_ACP,
                0,
                szEncryptionKey,
                wzcConfig1.KeyLength+1,
                szEncryptionKeyValue8,
                wzcConfig1.KeyLength+1,
                NULL,
                NULL);
            WZCPassword2Key(&wzcConfig1, szEncryptionKeyValue8);
            EncryptWepKMaterial(&wzcConfig1);
            wzcConfig1.dwCtlFlags |= WZCCTL_WEPK_XFORMAT
                    | WZCCTL_WEPK_PRESENT
                    | WZCCTL_ONEX_ENABLED;
        }
        wzcConfig1.EapolParams.dwEapFlags = EAPOL_ENABLED;
        wzcConfig1.EapolParams.dwEapType = DEFAULT_EAP_TYPE;
        wzcConfig1.EapolParams.bEnable8021x = TRUE;
        wzcConfig1.WPAMCastCipher = Ndis802_11Encryption2Enabled;
    }
}   // InterpretEncryptionKeyValue()



void
AddToPreferredNetworkList
// adding to the [Preferred Networks]
// [Preferred Networks] is a list of SSIDs in preference order.
// WZC continuously scans available SSIDs and attempt to connect to the most preferable SSID.
(
    IN WCHAR *szWiFiCard,
    IN WZC_WLAN_CONFIG& wzcConfig1,
    IN WCHAR *szSsidToConnect
)
{
    DWORD dwOutFlags = 0;
    INTF_ENTRY_EX Intf;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;

    DWORD dwStatus = WZCQueryInterfaceEx(
                        NULL, 
                        INTF_ALL,
                        &Intf, 
                        &dwOutFlags);
    if(dwStatus)
    {
        wprintf(L"WZCQueryInterfaceEx() error dwStatus=0x%0X, dwOutFlags=0x%0X", dwStatus, dwOutFlags);
        WZCDeleteIntfObjEx(&Intf);
        return;
    }

    WZC_802_11_CONFIG_LIST *pConfigList = (PWZC_802_11_CONFIG_LIST)Intf.rdStSSIDList.pData;
    if(!pConfigList)   // empty [Preferred Networks] list case
    {
        DWORD dwDataLen = sizeof(WZC_802_11_CONFIG_LIST);
        WZC_802_11_CONFIG_LIST *pNewConfigList = (WZC_802_11_CONFIG_LIST *)LocalAlloc(LPTR, dwDataLen);
        pNewConfigList->NumberOfItems = 1;
        pNewConfigList->Index = 0;
        memcpy(pNewConfigList->Config, &wzcConfig1, sizeof(wzcConfig1));
        Intf.rdStSSIDList.pData = (BYTE*)pNewConfigList;
        Intf.rdStSSIDList.dwDataLen = dwDataLen;
    }
    else
    {
        ULONG uiNumberOfItems = pConfigList->NumberOfItems;
        for(UINT i=0; i<uiNumberOfItems; i++)
        {
            if(memcmp(&wzcConfig1.Ssid, &pConfigList->Config[i].Ssid, sizeof(NDIS_802_11_SSID)) == 0)
            {
                wprintf(L"%s is already in the [Preferred Networks] list", szSsidToConnect);
                WZCDeleteIntfObjEx(&Intf);
                return;
            }
        }
        wprintf(L"SSID List has [%d] entries.\n", uiNumberOfItems);
        wprintf(L"adding %s to the top of [Preferred Networks]\n", szSsidToConnect); // this will be the most preferable SSID

        DWORD dwDataLen = sizeof(WZC_802_11_CONFIG_LIST) + (uiNumberOfItems+1)*sizeof(WZC_WLAN_CONFIG);
        WZC_802_11_CONFIG_LIST *pNewConfigList = (WZC_802_11_CONFIG_LIST *)LocalAlloc(LPTR, dwDataLen);
        pNewConfigList->NumberOfItems = uiNumberOfItems + 1;
        pNewConfigList->Index = 0;

        memcpy(pNewConfigList->Config, &wzcConfig1, sizeof(wzcConfig1));
        if(pConfigList->NumberOfItems)
        {
            pNewConfigList->Index = pConfigList->Index;
            memcpy(pNewConfigList->Config+1, pConfigList->Config, (uiNumberOfItems)*sizeof(WZC_WLAN_CONFIG));
            LocalFree(pConfigList);
            pConfigList = NULL;
        }

        Intf.rdStSSIDList.pData = (BYTE*)pNewConfigList;
        Intf.rdStSSIDList.dwDataLen = dwDataLen;
    }

    dwStatus = WZCSetInterfaceEx(NULL, INTF_PREFLIST, &Intf, &dwOutFlags);
    if(dwStatus)
        wprintf(L"WZCSetInterfaceEx() error dwStatus=0x%0X, dwOutFlags=0x%0X", dwStatus, dwOutFlags);

    WZCDeleteIntfObjEx(&Intf);
}   // AddToPreferredNetworkList()




void
DoConfigureAsRegistry
// configure WZC data as the registry setting
// WZCTOOL registry is under HKEY_CURRENT_USER\Comm\WZCTOOL
// sample:
// [HKEY_CURRENT_USER\Comm\WZCTOOL]
//    "SSID"           = "CE8021X",
//    "authentication" = dword:0  ; open (Ndis802_11AuthModeOpen)
//    "encryption"     = dword:0  ; WEP (Ndis802_11WEPEnabled)
//    "key"            = "auto"   ; key generated automatically by EAP
//    "eap"            = "tls"    ; EAP type is TLS (certificate based authentication)
//    "adhoc"          = dword:0  ; CE8021X is an infrastructure network
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    if(WasOption(argc, argv, L"?")>0)
    {
        WCHAR *szOptionHelp[] = {
            L"registry datail\n",
            L"\n",
            L"registry path  = HKEY_CURRENT_USER\\Comm\\WZCTOOL\n",
            L"SSID           = REG_SZ (max 32 char)\n",
            L"authentication = REG_DWORD\n",
            L"encryption     = REG_DWORD\n",
            L"key            = REG_SZ\n",
            L"eap            = REG_SZ\n",
            L"adhoc          = REG_DWORD\n",
            L"\n\n",
            NULL };
        ShowTextMessages(szOptionHelp);

        wprintf(L"authentication =\n");
        wprintf(L"   %d = open (Ndis802_11AuthModeOpen)\n", Ndis802_11AuthModeOpen);
        wprintf(L"   %d = shared-key (Ndis802_11AuthModeShared)\n", Ndis802_11AuthModeShared);
        wprintf(L"   %d = WPA-PSK (Ndis802_11AuthModeWPAPSK)\n", Ndis802_11AuthModeWPAPSK);
        wprintf(L"   %d = WPA-NONE (Ndis802_11AuthModeWPANone)\n", Ndis802_11AuthModeWPANone);
        wprintf(L"   %d = WPA (Ndis802_11AuthModeWPA)\n", Ndis802_11AuthModeWPA);
        wprintf(L"\n");

        wprintf(L"encryption =\n");
        wprintf(L"   %d = WEP (Ndis802_11WEPEnabled)\n", Ndis802_11WEPEnabled);
        wprintf(L"   %d = no-encrption (Ndis802_11WEPDisabled)\n", Ndis802_11WEPDisabled);
        wprintf(L"   %d = TKIP (Ndis802_11Encryption2Enabled)\n", Ndis802_11Encryption2Enabled);
        wprintf(L"\n");

        wprintf(L"eap =\n");
        wprintf(L"   %d = TLS\n", EAP_TYPE_TLS);
        wprintf(L"   %d = PEAP\n", EAP_TYPE_PEAP);
        wprintf(L"   %d = MD5\n", EAP_TYPE_MD5);
        wprintf(L"\n");

        wprintf(L"adhoc =\n");
        wprintf(L"   1 = adhoc net\n");
        wprintf(L"   if \"adhoc\" value not exists or its value is 0 = inrastructure net\n");
        wprintf(L"\n");

        WCHAR *szRegExample[] = {
            L"registry example:\n",
            L"\n",
            L"HKEY_CURRENT_USER\\Comm\\WZCTOOL\n",
            L"   \"SSID\" = \"CEWEP40\"\n",
            L"   \"authentication\" = dword:0  ;OPEN-authentication\n",
            L"   \"encryption\" = dword:0      ;WEP-encryption\n",
            L"   \"key\" = \"1/0x1234567890    ;key index=1, 40-bit key\"\n",
            L"   \"adhoc\" = dword:0\n",
            L"\n\n",
            NULL };
        ShowTextMessages(szRegExample);

        WCHAR *szNotes[] = {
            L"Note:\n",
            L"Storing encryption key in registry as a clear text form could be a security concern.\n",
            L"somebody could ready keys by scanning registry.\n",
            L"possible way to avoid is encrypt the key values so that only your software can understand.\n",
            L"\n",
            NULL };
        ShowTextMessages(szNotes);

        return;
    }

    HKEY hKey1 = NULL;
    LONG rc = RegOpenKeyEx(HKEY_CURRENT_USER, L"Comm\\WZCTOOL", 0, 0, &hKey1);
    if (rc != ERROR_SUCCESS)
        return;

    WZC_WLAN_CONFIG wzcConfig1;
    memset(&wzcConfig1, 0, sizeof(wzcConfig1));
    wzcConfig1.Length = sizeof(wzcConfig1);
    wzcConfig1.dwCtlFlags = 0;

    GetFirstWirelessNetworkCard();
    if(!*g_WirelessCard1)    // wifi card not found
        return;
    WCHAR *szWiFiCard = g_WirelessCard1;

    BYTE ucbData[MAX_PATH];
    DWORD *pdwData = (DWORD *)ucbData;
    DWORD dwDataSize = sizeof(ucbData);
    DWORD dwType;

    // SSID
    if(
        (ERROR_SUCCESS != RegQueryValueEx(hKey1, L"SSID", NULL, &dwType, ucbData, &dwDataSize)) ||
        (dwType != REG_SZ) )
    {
        wprintf(L"error no SSID is given. Check usage.\n");
        RegCloseKey(hKey1);
        return;
    }
    WCHAR *szSsidToConnect = (WCHAR *)ucbData;
    wzcConfig1.Ssid.SsidLength = wcslen(szSsidToConnect);
    if(wzcConfig1.Ssid.SsidLength >= 32)
    {
        wprintf(L"SSID is too long, truncating to max-32-chars\n");
        wzcConfig1.Ssid.SsidLength = 32;
    }
    for(UINT i=0; i<wzcConfig1.Ssid.SsidLength; i++)
        wzcConfig1.Ssid.Ssid[i] = (char)szSsidToConnect[i];

    // adhoc? or infrastructure net?
    wzcConfig1.InfrastructureMode = Ndis802_11Infrastructure;
    dwDataSize = sizeof(ucbData);
    if(
        (ERROR_SUCCESS == RegQueryValueEx(hKey1, L"adhoc", NULL, &dwType, ucbData, &dwDataSize)) &&
        (dwType == REG_DWORD) &&
        (*pdwData) )
    {
        wzcConfig1.InfrastructureMode = Ndis802_11IBSS;
    }

    // Authentication
    wzcConfig1.AuthenticationMode = Ndis802_11AuthModeOpen;
    dwDataSize = sizeof(ucbData);
    if(
        (ERROR_SUCCESS == RegQueryValueEx(hKey1, L"authentication", NULL, &dwType, ucbData, &dwDataSize)) &&
        (dwType == REG_DWORD) )
    {
        wzcConfig1.AuthenticationMode = (NDIS_802_11_AUTHENTICATION_MODE)(*pdwData);
    }

    // Encryption
    wzcConfig1.Privacy = Ndis802_11WEPDisabled;
    dwDataSize = sizeof(ucbData);
    if(
        (ERROR_SUCCESS == RegQueryValueEx(hKey1, L"encryption", NULL, &dwType, ucbData, &dwDataSize)) &&
        (dwType == REG_DWORD) )
    {
        wzcConfig1.Privacy = *pdwData;
    }

    // Key
    WCHAR *szEncryptionKey = (WCHAR *)ucbData;
    dwDataSize = sizeof(ucbData);
    BOOL bNeed8021X = FALSE;
    if(
        (ERROR_SUCCESS == RegQueryValueEx(hKey1, L"key", NULL, &dwType, ucbData, &dwDataSize)) &&
        (dwType == REG_SZ) )
    {
        InterpretEncryptionKeyValue(wzcConfig1, szEncryptionKey, bNeed8021X);
    }

    if(bNeed8021X)
    {
        WCHAR *szEapType = (WCHAR *)ucbData;
        dwDataSize = sizeof(ucbData);
        if(
            (ERROR_SUCCESS == RegQueryValueEx(hKey1, L"eap", NULL, &dwType, ucbData, &dwDataSize)) &&
            (dwType == REG_SZ) )
        {
            if(!_wcsicmp(szEapType, L"tls"))
                wzcConfig1.EapolParams.dwEapType = EAP_TYPE_TLS;
            else if(!_wcsicmp(szEapType, L"peap"))
                wzcConfig1.EapolParams.dwEapType = EAP_TYPE_PEAP;
            else
                wprintf(L"invalid eap type\n");
            wzcConfig1.EapolParams.dwEapFlags = EAPOL_ENABLED;
            wzcConfig1.EapolParams.bEnable8021x  = TRUE;
            wzcConfig1.EapolParams.dwAuthDataLen = 0;
            wzcConfig1.EapolParams.pbAuthData    = 0;
        }
        else
        {
            wprintf(L"need eap-type for this option\n");
        }
    }
    RegCloseKey(hKey1);

    AddToPreferredNetworkList(szWiFiCard, wzcConfig1, szSsidToConnect);
}   // DoConfigureAsRegistry()




void
DoConnect
// configure WZC as given arguments.
// as a result of this configuration, WZC will atempt to connect given SSID.
//
// options could be
// wzctool -c cisco1 -ssid TEST1
// wzctool -c -ssid TEST1
//     find the first wireless card, and connect this card to 'TEST1'
// wzctool -c cisco1 -ssid TEST1 -auth open -encr disabled
// wzctool -c cisco1 -ssid TEST1 -auth open -encr wep -key 1/0x1234567890
//      WEP key, index=1, 10-digit-hexa=40-bit-key
// wzctool -c cisco1 -ssid TEST1 -auth open -encr wep -key auto -eap tls
// wzctool -c cisco1 -ssid TEST1 -auth wpa-psk -encr tkip -key abcdefgh
// wzctool -c cisco1 -ssid TEST1 -auth wpa -encr tkip -key auto -eap tls
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    if(WasOption(argc, argv, L"?")>0)
    {
        WCHAR *szOptionHelp[] = {
            L"-c options:\n",
            L"-ssid   SSID to connect. SSID is the name of wireless network\n",
            L"-auth   open/shared/wpa-psk/wpa/wpa2-psk/wpa2\n",
            L"        '-authentication' is same.\n",
            L"        default is open-authentication.\n",
            L"-encr   disabled/wep/tkip/aes\n",
            L"        '-encryption' is same.\n",
            L"        'WEP' and 'TKIP' should have '-key' option.\n",
            L"        default is encryption-disabled.\n",
            L"-key    key value.\n",
            L"        for WEP-key, use '#/<key-value>' form.\n",
            L"        '#' is key-index (1-4), '<key-value>' is WEP key value (40-bit or 104-bit).\n",
            L"        40-bit is either '10-digit hexa numbers' (ex: 0x1234567890) or '5-char ASCII string' (ex: zxcvb)\n",
            L"        104-bit is either '26-digit hexa numbers' (ex: 0x12345678901234567890123) or '13-char ASCII string' (ex: abcdefghijklm)\n",
            L"        for TKIP-key, use '<key-value>' form. (no key index)\n",
            L"        TKIP-key can be 8-63 char ASCII string (ex: asdfghjk)\n",
            L"-eap    tls/peap/md5\n",
            L"        this is for 802.1X (EAP). both AP and STA will get keys automatically after the successful EAP.\n",
            L"        so, '-eap' should have '-key auto' option.\n",
            L"        UI dialogs will popup and ask user credentials (like certificate or user-name/password).\n",
            L"-adhoc  connects to an adhoc net.\n",
            L"        if this is not given, by default connecting to an AP (infrastructure net).\n",
            L"-example  shows example usage.\n",
            L"note\n",
            L"1. options are case insensitive. (ie. '-auth'=='-AUTH', 'open'=='OPEN')\n",
            L"2. but the SSID and key-values are case sensitive. (ie. 'TEST1'!='test1', 'abcdefgh'!='ABCDEFGH'\n",
            L"3. giving WEP key in a command line could be a security concern.\n",
            L"   somebody could watch keys over your shoulder.\n",
            NULL };
        ShowTextMessages(szOptionHelp);
        return;
    }
    else if((WasOption(argc, argv, L"ex")>0) || (WasOption(argc, argv, L"example")>0))
    {
        WCHAR *szOptionExamples[] = {
            L"examples:\n",
            L"wzctool -c -ssid CEOPEN -auth open -encr disabled\n",
            L"wzctool -c -ssid CEOPEN    (same as above)\n",
            L"wzctool -c -ssid CESHARED -auth open -encr WEP -key 1/0x1234567890\n",
            L"wzctool -c -ssid CESHARED2 -auth open -encr WEP -key 4/zxcvb\n",
            L"wzctool -c -ssid CESHARED3 -auth shared -encr WEP -key 1/0x1234567890\n",
            L"wzctool -c -ssid CE8021X -auth open -encr wep -key auto -eap tls",
            L"wzctool -c -ssid WPAPSK -auth wpa-psk -encr tkip -key qwertyuiop",
            L"wzctool -c -ssid WPA -auth wpa -encr tkip -key auto -eap peap",
            L"wzctool -c -ssid CEAD1 -adhoc\n",
            L"wzctool -c -ssid CEADWEP104 -adhoc -auth open -encr WEP -key 1/abcdefghijabc\n",
            NULL };
        ShowTextMessages(szOptionExamples);
    }

    WZC_WLAN_CONFIG wzcConfig1;
    memset(&wzcConfig1, 0, sizeof(wzcConfig1));
    wzcConfig1.Length = sizeof(wzcConfig1);
    wzcConfig1.dwCtlFlags = 0;

    WCHAR *szWiFiCard = NULL;
    if(
        (GetOption(argc, argv, L"c", &szWiFiCard) < 0) ||
        (*szWiFiCard == L'-') )
    {
        GetFirstWirelessNetworkCard();
        if(!*g_WirelessCard1)    // wifi card not found
            return;
        szWiFiCard = g_WirelessCard1;
    }

    // SSID
    WCHAR *szSsidToConnect = NULL;
    if(GetOption(argc, argv, L"ssid", &szSsidToConnect) < 0)
    {
        wprintf(L"no SSID is given\n");
        return;
    }
    wzcConfig1.Ssid.SsidLength = wcslen(szSsidToConnect);
    for(UINT i=0; i<wzcConfig1.Ssid.SsidLength; i++)
        wzcConfig1.Ssid.Ssid[i] = (char)szSsidToConnect[i];

    // adhoc? or infrastructure net?
    wzcConfig1.InfrastructureMode =
        (WasOption(argc, argv, L"adhoc")>0) ? Ndis802_11IBSS : Ndis802_11Infrastructure;

    // Authentication
    wzcConfig1.AuthenticationMode = Ndis802_11AuthModeOpen;
    WCHAR *szAuthMode = NULL;
    if((GetOption(argc, argv, L"auth", &szAuthMode) > 0) ||
        (GetOption(argc, argv, L"authentication", &szAuthMode) > 0) )
    {
        if(!_wcsicmp(szAuthMode, L"open"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeOpen;
        else if(!_wcsicmp(szAuthMode, L"shared"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeShared;
        else if(!_wcsicmp(szAuthMode, L"wpa-psk"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeWPAPSK;
        else if(!_wcsicmp(szAuthMode, L"wpa-none"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeWPANone;
        else if(!_wcsicmp(szAuthMode, L"wpa"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeWPA;
        else if(!_wcsicmp(szAuthMode, L"wpa2"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeWPA2;
        else if(!_wcsicmp(szAuthMode, L"wpa2-psk"))
            wzcConfig1.AuthenticationMode = Ndis802_11AuthModeWPA2PSK;
        else
            wprintf(L"unknown auth mode, assuming 'open-auth'\n");
    }

    // Encryption
    wzcConfig1.Privacy = Ndis802_11WEPDisabled;
    WCHAR *szEncryptionMode = NULL;
    if((GetOption(argc, argv, L"encr", &szEncryptionMode ) > 0) ||
        (GetOption(argc, argv, L"encryption", &szEncryptionMode ) > 0) )
    {
        if(!_wcsicmp(szEncryptionMode, L"disabled"))
            wzcConfig1.Privacy = Ndis802_11WEPDisabled;
        else if(!_wcsicmp(szEncryptionMode, L"wep"))
            wzcConfig1.Privacy = Ndis802_11WEPEnabled;
        else if(!_wcsicmp(szEncryptionMode, L"tkip"))
            wzcConfig1.Privacy = Ndis802_11Encryption2Enabled;
        else if(!_wcsicmp(szEncryptionMode, L"aes"))
            wzcConfig1.Privacy = Ndis802_11Encryption3Enabled;
        else
            wprintf(L"unknown encryption mode, assuming 'encryption-disabled'\n");
    }

    // Key
    WCHAR *szEncryptionKey = NULL;
    if(GetOption(argc, argv, L"key", &szEncryptionKey) < 0)
        szEncryptionKey = L"auto";
    BOOL bNeed8021X = FALSE;
    InterpretEncryptionKeyValue(wzcConfig1, szEncryptionKey, bNeed8021X);

    if(bNeed8021X)
    {
        WCHAR *szEapType = NULL;
        if(GetOption(argc, argv, L"eap", &szEapType) < 0)
        {
            wprintf(L"need eap-type for this option\n");
            return;
        }

        if(!_wcsicmp(szEapType, L"tls"))
            wzcConfig1.EapolParams.dwEapType = EAP_TYPE_TLS;
        else if(!_wcsicmp(szEapType, L"peap"))
            wzcConfig1.EapolParams.dwEapType = EAP_TYPE_PEAP;
        else if(!_wcsicmp(szEapType, L"md5"))
            wzcConfig1.EapolParams.dwEapType = EAP_TYPE_MD5;
        else
        {
            wprintf(L"invalid eap type\n");
            return;
        }
        wzcConfig1.EapolParams.dwEapFlags = EAPOL_ENABLED;
        wzcConfig1.EapolParams.bEnable8021x  = TRUE;
        wzcConfig1.EapolParams.dwAuthDataLen = 0;
        wzcConfig1.EapolParams.pbAuthData    = 0;
    }


    AddToPreferredNetworkList(szWiFiCard, wzcConfig1, szSsidToConnect);
}   // DoConnect()

#ifdef UNDER_CE
void
SetAdapterPMKCache
(
     IN WCHAR *szWiFiCard,
	 IN DWORD  Flags,
	 IN DWORD  FlagsToSet
)
//
//  Set the PMK Cache flags specified by the mask "FlagsToSet"
//  to the value specified by the mask "Flags".
//
{
	DWORD dwOutFlags = 0;
    INTF_ENTRY_EX Intf;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;

    DWORD dwStatus = WZCQueryInterfaceEx(NULL, INTF_PMKCACHE, &Intf,  &dwOutFlags);
    if(dwStatus)
    {
        wprintf(L"WZCQueryInterfaceEx() error dwStatus=0x%0X, dwOutFlags=0x%0X", dwStatus, dwOutFlags);
        WZCDeleteIntfObjEx(&Intf);
        return;
    }

	Intf.PMKCacheFlags = (Intf.PMKCacheFlags & ~FlagsToSet) | Flags;
    dwStatus = WZCSetInterfaceEx(NULL, INTF_PMKCACHE, &Intf, &dwOutFlags);
    if(dwStatus)
        wprintf(L"WZCSetInterfaceEx() error dwStatus=0x%0X, dwOutFlags=0x%0X", dwStatus, dwOutFlags);

    WZCDeleteIntfObjEx(&Intf);
}

void
DoCache
//  Do various PMK Cache operations
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
	WZC_WLAN_CONFIG wzcConfig1;
    memset(&wzcConfig1, 0, sizeof(wzcConfig1));
    wzcConfig1.Length = sizeof(wzcConfig1);
    wzcConfig1.dwCtlFlags = 0;

    WCHAR *szWiFiCard = NULL;
    if((GetOption(argc, argv, L"cache", &szWiFiCard) < 0) ||
        (*szWiFiCard == L'-') )
    {
        GetFirstWirelessNetworkCard();
        if(!*g_WirelessCard1)    // wifi card not found
            return;
        szWiFiCard = g_WirelessCard1;
    }
	
	DWORD Flags = 0;
	DWORD FlagsToSet = 0;
	WCHAR *szFlag;
	if ((GetOption(argc, argv, L"enable", &szFlag) > 0))
	{
		FlagsToSet |= INTF_ENTRY_PMKCACHE_FLAG_ENABLE;
		if (0 == wcsicmp(szFlag, L"on"))
			Flags |= INTF_ENTRY_PMKCACHE_FLAG_ENABLE;
	}
	if ((GetOption(argc, argv, L"opportunistic", &szFlag) > 0))
	{
		FlagsToSet |= INTF_ENTRY_PMKCACHE_FLAG_ENABLE_OPPORTUNISTIC;
		if (0 == wcsicmp(szFlag, L"on"))
			Flags |= INTF_ENTRY_PMKCACHE_FLAG_ENABLE_OPPORTUNISTIC;
	}
	if ((GetOption(argc, argv, L"preauth", &szFlag) > 0))
	{
		FlagsToSet |= INTF_ENTRY_PMKCACHE_FLAG_ENABLE_PREAUTH;
		if (0 == wcsicmp(szFlag, L"on"))
			Flags |= INTF_ENTRY_PMKCACHE_FLAG_ENABLE_PREAUTH;
	}
	if ((WasOption(argc, argv, L"flush") > 0))
	{
		FlagsToSet |= INTF_ENTRY_PMKCACHE_FLAG_FLUSH;
		Flags |= INTF_ENTRY_PMKCACHE_FLAG_FLUSH;
	}

	if (FlagsToSet)
	{
		SetAdapterPMKCache(szWiFiCard, Flags, FlagsToSet);
	}
	else
	{
		DoQueryCache(szWiFiCard);
	}
}

#endif



void
ResetPreferredList
// reset the [Preferred Networks], so wireless will be disconnected
// wzctool -reset cisco1
//      reset CISCO1 adapter.
// wzctool -reset
//      reset the first wireless adapter found in the system
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    WCHAR *szWiFiCard = NULL;
    if(
        (GetOption(argc, argv, L"reset", &szWiFiCard) < 0) ||
        (*szWiFiCard == L'-') )
    {
        GetFirstWirelessNetworkCard();
        if(!*g_WirelessCard1)    // wifi card not found
            return;
        szWiFiCard = g_WirelessCard1;
    }

    DWORD dwInFlags = 0;
    INTF_ENTRY_EX Intf;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;
    DWORD dwStatus = WZCSetInterfaceEx(NULL, INTF_PREFLIST, &Intf, &dwInFlags);
    if(dwStatus)
        wprintf(L"WZCSetInterfaceEx() error dwStatus=0x%0X, dwOutFlags=0x%0X", dwStatus, dwInFlags);
    else
        wprintf(L"now, WZC resets [Preferred Networks]\n");
}   // ResetPreferredList()





void
SetWzcParameter
// set WZC parameters.
// WZC has 4 timer parameters; tmTr, tmTp, tmTc, tmTf
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    WZC_CONTEXT WzcContext;
    DWORD dwStatus = WZCQueryContext(NULL, 0x00, &WzcContext, NULL);
    if (dwStatus != ERROR_SUCCESS)
    {
        wprintf(L"!!! Failed WZCQueryContext.  Err = [0x%08X] !!!\n", dwStatus);
        return;
    }

    if(WasOption(argc, argv, L"?")>0)
    {
        WCHAR *szHelp[] = {
            L"tmTr  Scan time out.\n"
            L"      WZC requests BSSID scan to the miniport driver then waits for 'tmTr'\n",
            L"      until wireless miniport finishes scanning.\n",
            L"      default = 3 sec (3000 ms)\n",
            L"tmTp  Association time out.\n",
            L"      WZC requests wifi adapter to associate to the given SSID.\n",
            L"      If wifi adapter does not finish association within time 'tmTp',\n",
            L"      WZC tries next SSID in the [Preferred Networks].\n",
            L"      default = 2 sec (2000 ms)\n",
            L"tmTc  Periodic scan when connected.\n",
            L"      The scanning requires channel switching on wireless card.\n",
            L"      in order to listen beaconing packets on all channels.\n",
            L"      The channel switching is not preferable when STA is in connected-state\n",
            L"      since wireless packets transfer is blocked during the scanning.\n",
            L"      This timer is set to possible maximum value INFINITE by default.\n",
            L"      default = INFINITE (0x70000000=1879048192)\n",
            L"tmTf  Periodic scan when disconnected.\n",
            L"      This is the interval that WZC sends scanning requests\n",
            L"      to find connection candidate SSIDs given by [Preferred Networks].\n",
            L"      default = 1 min (60000 ms)\n",
            L"\n",
            L"usage:\n",
            L"wzctool -set -tmtr 1000\n",
            L"      set 'tmTr' timer to 1000 mili-second\n",
            L"wzctool -set -tmtr 1000 -tmtf 2000\n",
            L"      set 'tmTr' = 1000 mili-second, 'tmTf' = 2000 mili-second\n",
            L"wzctool -set -tmtr 0\n",
            L"      will set back to the default value.\n",
            L"wzctool -set -tmtr -1\n",
            L"      will set to INFINITE number.\n",
            L"\n",
			NULL
        };

        ShowTextMessages(szHelp);
        wprintf(L"current parameter value:\n");
    }
    else
    {
        WCHAR *szVal = NULL;
        int iValue = 0;
        if(GetOption(argc, argv, L"tmtr", &szVal) > 0)
        {
            iValue = _wtoi(szVal);
            WzcContext.tmTr = ((iValue==0) ? TMMS_DEFAULT_TR :
                    ((iValue<0) ? TMMS_INFINITE : iValue));
        }
        if(GetOption(argc, argv, L"tmtp", &szVal) > 0)
        {
            iValue = _wtoi(szVal);
            WzcContext.tmTp = ((iValue==0) ? TMMS_DEFAULT_TP :
                    ((iValue<0) ? TMMS_INFINITE : iValue));
        }
        if(GetOption(argc, argv, L"tmtc", &szVal) > 0)
        {
            iValue = _wtoi(szVal);
            WzcContext.tmTc = ((iValue==0) ? TMMS_DEFAULT_TC :
                    ((iValue<0) ? TMMS_INFINITE : iValue));
        }
        if(GetOption(argc, argv, L"tmtf", &szVal) > 0)
        {
            iValue = _wtoi(szVal);
            WzcContext.tmTf = ((iValue==0) ? TMMS_DEFAULT_TF :
                    ((iValue<0) ? TMMS_INFINITE : iValue));
        }

        dwStatus = WZCSetContext(NULL, 0x00, &WzcContext, NULL);
    }

    if (dwStatus != ERROR_SUCCESS)
        wprintf(L"!!! Failed WZCSetContext. Err = [0x%08X] !!!\n", dwStatus);
    else
    {
        wprintf(L"tmTr = %d mili-seconds (Scan time out)\n", WzcContext.tmTr);
        wprintf(L"tmTp = %d mili-seconds (Association time out)\n", WzcContext.tmTp);
        wprintf(L"tmTc = %d mili-seconds (Periodic scan when connected)\n", WzcContext.tmTc);
        wprintf(L"tmTf = %d mili-seconds (Periodic scan when disconnected)\n", WzcContext.tmTf);
    }
}   // SetWzcParameter()





void
DoRefreshWzc
// refresh WZC
// forces WZC to reconnect [Preferred Networks]
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    WCHAR *szWiFiCard = NULL;
    if(
        (GetOption(argc, argv, L"r", &szWiFiCard) < 0) ||
        (*szWiFiCard == L'-') )
    {
        GetFirstWirelessNetworkCard();
        if(!*g_WirelessCard1)    // wifi card not found
            return;
        szWiFiCard = g_WirelessCard1;
    }

    INTF_ENTRY_EX Intf;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;
    DWORD dwOutFlags;
    DWORD dwStatus = WZCRefreshInterfaceEx(
                        NULL,
                        INTF_ALL,
                        &Intf,
                        &dwOutFlags);
    if (dwStatus != ERROR_SUCCESS)
        wprintf(L"!!! Failed WZCRefreshInterfaceEx. Err = [0x%08X] !!!\n", dwStatus);
    else
        wprintf(L"WZCRefreshInterfaceEx successful\n");
}   // DoRefreshWzc()




void
DoEnableDisableWzcSvc
// enable or disable WZC service
// options
// we can also provide specific adapter to apply this option.
// "-disablewzcsvc cisco1" will disable WZC service on cisco1 card.
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    BOOL bEnable = TRUE;
    WCHAR *szWiFiCard = NULL;
    if(WasOption(argc, argv, L"enablewzcsvc")>0)
    {
        bEnable = TRUE;
        if(
            (GetOption(argc, argv, L"enablewzcsvc", &szWiFiCard) < 0) ||
            (*szWiFiCard == L'-') )
        {
            GetFirstWirelessNetworkCard();
            if(!*g_WirelessCard1)    // wifi card not found
                return;
            szWiFiCard = g_WirelessCard1;
        }
    }
    else if(WasOption(argc, argv, L"disablewzcsvc")>0)
    {
        bEnable = FALSE;
        if(
            (GetOption(argc, argv, L"disablewzcsvc", &szWiFiCard) < 0) ||
            (*szWiFiCard == L'-') )
        {
            GetFirstWirelessNetworkCard();
            if(!*g_WirelessCard1)    // wifi card not found
                return;
            szWiFiCard = g_WirelessCard1;
        }
    }
    else
    {
        wprintf(L"unknown option\n");
        return;
    }

    DWORD dwInFlags = 0;
    INTF_ENTRY_EX Intf;
    memset(&Intf, 0x00, sizeof(INTF_ENTRY_EX));
    Intf.wszGuid = szWiFiCard;
    if(bEnable)
        Intf.dwCtlFlags |= INTFCTL_ENABLED;
    else
        Intf.dwCtlFlags &= ~INTFCTL_ENABLED;

    // work item for INTFCTL_FALLBACK

    DWORD dwStatus = WZCSetInterfaceEx(NULL, INTF_ENABLED, &Intf, &dwInFlags);
    if(dwStatus)
        wprintf(L"WZCSetInterfaceEx() error dwStatus=0x%0X, dwOutFlags=0x%0X", dwStatus, dwInFlags);
    else
        wprintf(L"%s\n", bEnable ? L"enabled" : L"disabled" );
}   // DoEnableDisableWzcSvc




WCHAR *g_szHelp[] =
{
    L"wzctool usage:\n",
    L"options:\n",
    L" -e              Enumerate wireless cards.\n",
    L" -q <Card Name>  Query wireless card.\n",
    L" -c <Card Name> -ssid AP-SSID -auth open -encr wep -key 1/0x1234567890\n",
    L"     connect to AP-SSID with given parameters.",
    L"     Use -c -? for detail.\n",
    L" -reset          Reset WZC configuration data. Wireless card will disconnect if it was connected.\n",
    L" -set <Card Name> <parameter> Set WZC variables.\n",
    L"     Use -set -? for detail.\n",
    L" -refresh        Refresh entries.\n",
    L" -registry       configure as registry.\n",
    L"     Use -registry -? for detail.\n",
    L" -enablewzcsvc   enable WZC service.\n",
    L" -disablewzcsvc  disable WZC service.\n",
    L" -?  shows help message\n",
    L"if no arg is given, wzctool will reads and set as settings in the registry. Use '-registry -?' for detail\n",
    L"if no <Card Name> is given, wzctool will find the first WiFi card and use this card.\n",
    NULL
};


int
wmain
// main function for WCHAR console application
(
    IN int argc,      // number of args
    IN WCHAR* argv[]  // arg array
)
{
    //wprintf(L"wireless configuration tool\n");

    if(argc==1)
    {
        wprintf(L"trying to read registry and configure wireless settings\n");
        wprintf(L"security warning: storing encryption key in clear text is volunerable to attack.\n");
        DoConfigureAsRegistry(argc, argv);
        return 0;
    }

    if(WasOption(argc, argv, L"e")>0)
        EnumWirelessNetworkCard();
    else if(WasOption(argc, argv, L"q")>0)
        DoQuery(argc, argv);
    else if(WasOption(argc, argv, L"c")>0)
        DoConnect(argc, argv);
    else if(WasOption(argc, argv, L"cache")>0)
        DoCache(argc, argv);
    else if(WasOption(argc, argv, L"reset")>0)
        ResetPreferredList(argc, argv);
    else if(WasOption(argc, argv, L"set")>0)
        SetWzcParameter(argc, argv);
    else if(WasOption(argc, argv, L"refresh")>0)
        DoRefreshWzc(argc, argv);
    else if(WasOption(argc, argv, L"registry")>0)
        DoConfigureAsRegistry(argc, argv);
    else if(WasOption(argc, argv, L"enablewzcsvc")>0)
        DoEnableDisableWzcSvc(argc, argv);
    else if(WasOption(argc, argv, L"disablewzcsvc")>0)
        DoEnableDisableWzcSvc(argc, argv);
    else //if(WasOption(argc, argv, L"?")>0)
        ShowTextMessages(g_szHelp);
    return 0;
}   // wmain()

