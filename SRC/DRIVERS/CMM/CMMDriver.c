//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include <bsp.h>
#include "CMMDriver.h"
#include "CMMMisc.h"

#if (_WIN32_WCE >= 600)
#define E2E_CMM
#endif
#include <pm.h>
#include <pmplatform.h>
#include <windows.h>
#include <nkintr.h>

BOOL            instanceNo[MAX_INSTANCE_NUM];
ALLOC_MEM_T    *AllocMemHead;
ALLOC_MEM_T    *AllocMemTail;
FREE_MEM_T        *FreeMemHead;
FREE_MEM_T        *FreeMemTail;
UINT8            *CachedVirAddr;

#if DEBUG
#define ZONE_INIT              DEBUGZONE(0)

DBGPARAM dpCurSettings =                \
{                                       \
    TEXT("CMM_Driver"),                 \
    {                                   \
        TEXT("Init"),       /* 0  */    \
    },                                  \
    (0x0001)                            \
};
#endif

/*----------------------------------------------------------------------------
*Function: CMM_Init

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize JPEG Hardware 
-----------------------------------------------------------------------------*/
DWORD
CMM_Init(
    DWORD dwContext
    )
{
    //CODEC_MEM_CTX *CodecMem;
    HANDLE      h_Mutex=NULL;
    DWORD       ret;
    FREE_MEM_T *node = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    printD("\n[CMM_Init]\n");

    
    // Mutex initialization
    h_Mutex = CreateCMMmutex();
    if (h_Mutex == NULL) 
    {
        RETAILMSG(1, (TEXT("[CMM_Init] CMM Mutex Initialize error : %d \r\n"),GetLastError()));    
        return FALSE;
    }

    ret = LockCMMMutex();
    if(!ret){
        RETAILMSG(1, (TEXT("[CMM_Init] CMM Mutex Lock Fail\r\n")));
        return FALSE;
    }

    ioPhysicalBase.LowPart = CODEC_MEM_START;
    CachedVirAddr = (UINT8 *)MmMapIoSpace(ioPhysicalBase, CODEC_MEM_SIZE, TRUE);

    // init alloc list, if(AllocMemHead == AllocMemTail) then, the list is NULL
    node = (ALLOC_MEM_T *)malloc(sizeof(ALLOC_MEM_T));
    if(node == NULL)
    {
        RETAILMSG(1, (TEXT("[CMM_Init] Cannot allocate node for Alloc List\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }

    memset(node, 0x00, sizeof(ALLOC_MEM_T));
    node->next = node;
    node->prev = node;
    AllocMemHead = node;
    AllocMemTail = AllocMemHead;

    // init free list, if(FreeMemHead == FreeMemTail) then, the list is NULL
    node = (FREE_MEM_T *)malloc(sizeof(FREE_MEM_T));

    if(node == NULL)
    {
        RETAILMSG(1, (TEXT("[CMM_Init] Cannot allocate node for Free List\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }

    memset(node, 0x00, sizeof(FREE_MEM_T));
    node->next = node;
    node->prev = node;
    FreeMemHead = node;
    FreeMemTail = FreeMemHead;
    
    node = (FREE_MEM_T *)malloc(sizeof(FREE_MEM_T));

    if(node == NULL)
    {
        RETAILMSG(1, (TEXT("[CMM_Init] Cannot allocate node for Free List first item\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }

    memset(node, 0x00, sizeof(FREE_MEM_T));
    node->startAddr = CODEC_MEM_START;
    node->size = CODEC_MEM_SIZE;
    InsertNodeToFreeList(node, -1);

    UnlockCMMMutex();
    
    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: CMM_DeInit

*Parameters:         InitHandle        :
*Return Value:        True/False
*Implementation Notes: Deinitialize JPEG Hardware 
-----------------------------------------------------------------------------*/
BOOL
CMM_Deinit(
    DWORD InitHandle
    )
{
    CODEC_MEM_CTX *CodecMem;

    printD("[CMM_DeInit] CMM_Deinit\n");
    CodecMem = (CODEC_MEM_CTX *)InitHandle;

    if(!CodecMem){
        RETAILMSG(1, (TEXT("[CMM_DeInit] CMM Invalid Input Handle\r\n")));
        return FALSE;
    }

    DeleteCMMMutex();    
    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: CMM_Open

*Parameters:         InitHandle        :Handle to JPEG  context
                    dwAccess        :
                    dwShareMode        :File share mode of JPEG
*Return Value:        This function returns a handle that identifies the 
                    open context of JPEG  to the calling application.
*Implementation Notes: Opens JPEG CODEC device for reading, writing, or both 
-----------------------------------------------------------------------------*/
DWORD
CMM_Open(
    DWORD InitHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    CODEC_MEM_CTX *CodecMem;
    DWORD    ret;
    UINT8    inst_no;


    ret = LockCMMMutex();
    if(!ret){
        RETAILMSG(1, (TEXT("[CMM_Open] CMM Mutex Lock Fail\r\n")));
        return FALSE;
    }

    // check the number of instance 
    if((inst_no = GetInstanceNo()) < 0){
        RETAILMSG(1, (TEXT("[CMM_Open] Instance Number error-too many instance\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }

    CodecMem = (CODEC_MEM_CTX *)malloc(sizeof(CODEC_MEM_CTX));
    if(CodecMem == NULL){
        RETAILMSG(1, (TEXT("[CMM_Init] CodecMem allocatopn failed\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }
    
    memset(CodecMem, 0x00, sizeof(CODEC_MEM_CTX));

    CodecMem->inst_no = inst_no;
    printD("\n*****************************\n[CMM_Open] instanceNo : %d\n*****************************\n", CodecMem->inst_no);
    PrintList();
    UnlockCMMMutex();
    return (DWORD)CodecMem;
}


/*----------------------------------------------------------------------------
*Function: CMM_Close

*Parameters:         OpenHandle        :
*Return Value:        True/False
*Implementation Notes: This function closes the device context identified by
                        OpenHandle 
-----------------------------------------------------------------------------*/
BOOL
CMM_Close(
    DWORD OpenHandle
    )
{
    CODEC_MEM_CTX *CodecMem;
    DWORD    ret;
    ALLOC_MEM_T *node, *tmp_node;
    int        count=0;

    ret = LockCMMMutex();
    if(!ret){
        RETAILMSG(1, (TEXT("[CMM_Close] CMM Mutex Lock Fail\r\n")));
        return FALSE;
    }

    CodecMem = (CODEC_MEM_CTX *)OpenHandle;
    
    if(!CodecMem){
        RETAILMSG(1, (TEXT("[CMM_Close] CMM Invalid Input Handle\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }

    printD("[CMM_Close] CodecMem->inst_no : %d CodecMem->callerProcess : 0x%x\n", CodecMem->inst_no, CodecMem->callerProcess);

    __try
    {
        // release u_addr and v_addr accoring to inst_no
        for(node = AllocMemHead; node != AllocMemTail; node = node->next){
            if(node->inst_no == CodecMem->inst_no){
                tmp_node = node;
                node = node->prev;
                ReleaseAllocMem(tmp_node, CodecMem);
            }
        }
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
       RETAILMSG( 1, ( _T("CMM_Close:Exception in releasing memory\n")) );
        return FALSE;
    }
    
    printD("[%d][CMM Close] MergeFragmentation\n", CodecMem->inst_no);
    MergeFragmentation(CodecMem->inst_no);

    ReturnInstanceNo(CodecMem->inst_no);

    free(CodecMem);
    UnlockCMMMutex();


    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: CMM_IOControl

*Parameters:         OpenHandle        :
                    dwIoControlCode    :
*Return Value:        True/False
*Implementation Notes: JPEG_IOControl sends commands to initiate different
*                       operations like Init,Decode and Deinit.The test 
*                       application uses the DeviceIOControl function to 
*                       specify an operation to perform 
-----------------------------------------------------------------------------*/
BOOL
CMM_IOControl(
    DWORD OpenHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    )
{
    CODEC_MEM_CTX       *CodecMem;
    BOOL                result = TRUE;
    DWORD               ret;
    UINT8               *u_addr;
    ALLOC_MEM_T         *node;
    CMM_ALLOC_PRAM_T    *allocParam;
    PVOID pDestMarshalledIn = NULL;
    PVOID pDestMarshalledOut = NULL;
    
    ret = LockCMMMutex();
    if(!ret){
        RETAILMSG(1, (TEXT("[CMM_IOControl] CMM Mutex Lock Fail\r\n")));
        return FALSE;
    }

    CodecMem = (CODEC_MEM_CTX *)OpenHandle;

    if(!CodecMem){
        RETAILMSG(1, (TEXT("[CMM_IOControl] CMM Invalid Input Handle\r\n")));
        UnlockCMMMutex();
        return FALSE;
    }

    if ((pInBuf == NULL) || (nInBufSize == 0)){
        RETAILMSG(1, (TEXT("[CMM_IOControl] Invalid Input buffer or size\r\n")));
        return FALSE;
    }


    switch ( dwIoControlCode ) {

    case IOCTL_CODEC_MEM_ALLOC:
        if(FAILED(CeOpenCallerBuffer(&pDestMarshalledOut, pOutBuf, nOutBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_GET_LINE_BUF_ADDR.\r\n")));
            return FALSE;
        }
        
        printD("\n[%d][CMM_IOControl] IOCTL_CODEC_MEM_ALLOC\n", CodecMem->inst_no);

        allocParam = (CMM_ALLOC_PRAM_T *)pInBuf;
        printD("[IOCTL_CODEC_MEM_ALLOC] buffSize : %ld\n", allocParam->size);

    // Moon
    // For 4K align
    if((allocParam->size) & (0xFFF))
        allocParam->size = (allocParam->size & 0xFFFFF000) + 0x1000;    
    
        printD("[IOCTL_CODEC_MEM_ALLOC] buffSize : %ld\n", allocParam->size);

        if((node = GetCodecVirAddr(CodecMem->inst_no, allocParam)) == NULL){
            result = FALSE;
            break;
        }

        printD("[IOCTL_CODEC_MEM_ALLOC] node->u_addr : 0x%x\n", node->u_addr);
        printD("[IOCTL_CODEC_MEM_ALLOC] node->v_addr : 0x%x\n", node->v_addr);
        printD("[IOCTL_CODEC_MEM_ALLOC] node->size : %d\n", node->size);

#if    (_WIN32_WCE >= 600)
    #ifdef E2E_CMM
    {
        CodecMem->callerProcess = (HANDLE) GetDirectCallerProcessId();
        node->u_addr = (PBYTE) VirtualAllocEx(CodecMem->callerProcess, NULL, node->size, MEM_RESERVE ,PAGE_NOACCESS);       // HANDLE hSrcProc                                                        
        if (node->u_addr== FALSE)
        {
            RETAILMSG(1, (TEXT("DD:: error : %d \r\n"),GetLastError()));    
            RETAILMSG(1, (L"DD::CMM VirtualAllocEx(node->u_addr) returns FALSE.\n"));
            break;
        }        
        printD("[IOCTL_CODEC_MEM_ALLOC] node->u_addr : 0x%x\n", node->u_addr);
        printD("[IOCTL_CODEC_MEM_ALLOC] node->cached_p_addr : 0x%x\n", node->cached_p_addr);
        if (allocParam->cacheFlag)
        {
            result = VirtualCopyEx(CodecMem->callerProcess,         // HANDLE hDstProc
                                node->u_addr,
                                (HANDLE) GetCurrentProcessId(),     // HANDLE hSrcProc
                                (PVOID)(node->cached_p_addr >> 8),
                                node->size,
                                PAGE_PHYSICAL |PAGE_READWRITE );        
        }
        else
        {        
        result = VirtualCopyEx(CodecMem->callerProcess,             // HANDLE hDstProc
                                node->u_addr,
                                (HANDLE) GetCurrentProcessId(),     // HANDLE hSrcProc
                                (PVOID)((node->uncached_p_addr).LowPart >> 8),
                                node->size,
                                PAGE_PHYSICAL | PAGE_READWRITE |PAGE_NOCACHE );
        }
        if (result== FALSE)
        {
            RETAILMSG(1, (TEXT("DD:: error : %d \r\n"),GetLastError()));    
            RETAILMSG(1, (L"DD::CMM VirtualCopyEx(node->u_addr) returns FALSE.\n"));
            break;
       }
       printD("[IOCTL_CODEC_MEM_ALLOC] u_addr:0x%x  CallerProcessID : 0x%x\n",  node->u_addr, CodecMem->callerProcess);
       *((UINT *)pDestMarshalledOut) = (UINT) node->u_addr;
    }
    #else
    {
        CodecMem->callerProcess = (HANDLE) GetDirectCallerProcessId();
        node->u_addr = (PBYTE) VirtualAllocCopyEx((HANDLE) GetCurrentProcessId(),       // HANDLE hSrcProc                                                        
                                CodecMem->callerProcess,                                // HANDLE hDstProc
                                node->v_addr,
                                node->size,                
                                PAGE_READWRITE);
        printD("[IOCTL_CODEC_MEM_ALLOC] u_addr:0x%x  CallerProcessID : 0x%x\n",  node->u_addr, CodecMem->callerProcess);
        *((UINT *)pOutBuf) = (UINT) node->u_addr;
    }
    #endif
#else
        *((UINT *)pOutBuf) = (UINT) node->v_addr;
#endif

        if(FAILED(CeCloseCallerBuffer(pDestMarshalledOut, pOutBuf, nOutBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_GET_LINE_BUF_ADDR.\r\n")));
            return FALSE;
        }
        
        break;

    case IOCTL_CODEC_MEM_FREE:
        printD("\n[%d][CMM_IOControl] IOCTL_CODEC_MEM_FREE\n", CodecMem->inst_no);
        u_addr = (UINT8 *)pInBuf;
        printD("[CMM_IOControl] free adder : 0x%x \n", u_addr);

        for(node = AllocMemHead; node != AllocMemTail; node = node->next)
#if    (_WIN32_WCE >= 600)    
            if(node->u_addr == u_addr)
#else
            if(node->v_addr == u_addr)
#endif
                break;

        if(node  == AllocMemTail)
        {
            RETAILMSG(1, (TEXT("[CMM_IOControl] invalid virtual address(0x%x)\r\n"), u_addr));
            result = FALSE;
            break;
        }
        // free alloc node
        ReleaseAllocMem(node, CodecMem);
        break;

    case IOCTL_CODEC_CACHE_INVALIDATE:
        printD("\n[CMM_IOControl] IOCTL_CODEC_CACHE_INVALIDATE\n");
        u_addr = (UINT8 *)pInBuf;
        printD("[CMM_IOControl] flush adder : 0x%x \n", u_addr);

        for(node = AllocMemHead; node != AllocMemTail; node = node->next)
#if    (_WIN32_WCE >= 600)    
            if(node->u_addr == u_addr)
#else
            if(node->v_addr == u_addr)
#endif    
                break;

        if(node  == AllocMemTail){
            RETAILMSG(1, (TEXT("[%d][CMM_IOControl] invalid virtual address(0x%x)\r\n"), CodecMem->inst_no, u_addr));
            result = FALSE;
            break;
        }

        InvalidateCacheRange((PBYTE) node->v_addr, 
            (PBYTE) node->v_addr + node->size);
        break;

    case IOCTL_CODEC_CACHE_CLEAN:
        printD("\n[CMM_IOControl] IOCTL_CODEC_CACHE_CLEAN\n");
        u_addr = (UINT8 *)pInBuf;
        printD("[CMM_IOControl] flush adder : 0x%x \n", u_addr);

        for(node = AllocMemHead; node != AllocMemTail; node = node->next)
#if    (_WIN32_WCE >= 600)    
            if(node->u_addr == u_addr)
#else
            if(node->v_addr == u_addr)
#endif    
                break;

        if(node  == AllocMemTail){
            RETAILMSG(1, (TEXT("[%d][CMM_IOControl] invalid virtual address(0x%x)\r\n"), CodecMem->inst_no, u_addr));
            result = FALSE;
            break;
        }

        CleanCacheRange((PBYTE) node->v_addr, 
            (PBYTE) node->v_addr + node->size);
        break;


        // IOCTL_CODEC_CACHE_FLUSH is same as IOCTL_CODEC_CACHE_CLEAN_INVALIDATE.
        // This is remained for backward capability
    case IOCTL_CODEC_CACHE_FLUSH: 
    case IOCTL_CODEC_CACHE_CLEAN_INVALIDATE:
        printD("\n[CMM_IOControl] IOCTL_CODEC_CACHE_CLEAN_INVALIDATE\n");
        u_addr = (UINT8 *)pInBuf;
        printD("[CMM_IOControl] flush adder : 0x%x \n", u_addr);

        for(node = AllocMemHead; node != AllocMemTail; node = node->next)
#if    (_WIN32_WCE >= 600)    
            if(node->u_addr == u_addr)
#else
            if(node->v_addr == u_addr)
#endif    
                break;

        if(node  == AllocMemTail){
            RETAILMSG(1, (TEXT("[%d][CMM_IOControl] invalid virtual address(0x%x)\r\n"), CodecMem->inst_no, u_addr));
            result = FALSE;
            break;
        }

        CleanInvalidateCacheRange((PBYTE) node->v_addr, 
            (PBYTE) node->v_addr + node->size);
        break;


    case IOCTL_CODEC_GET_PHY_ADDR:
        u_addr  = (UINT8 *)pInBuf;
        //printD("\n[CMM_IOControl] IOCTL_CODEC_GET_PHY_ADDR(0x%x)\n", u_addr);
        for(node = AllocMemHead; node != AllocMemTail; node = node->next)
#if    (_WIN32_WCE >= 600)    
            if(node->u_addr == u_addr)
#else
            if(node->v_addr == u_addr)
#endif    
                break;

        if(node  == AllocMemTail){
            RETAILMSG(1, (TEXT("[CMM_IOControl] invalid virtual address(0x%x)\r\n"), u_addr));
            result = FALSE;
            break;
        }

        if(FAILED(CeOpenCallerBuffer(&pDestMarshalledOut, pOutBuf, nOutBufSize, ARG_O_PTR, TRUE)))    {
            printD("[IOCTL_CODEC_GET_PHY_ADDR] Access check failed on output buffer\n");
            return FALSE;
        }

        *((UINT *)pDestMarshalledOut) = (UINT) node->cached_p_addr;

        //printD("\n[CMM_IOControl] p_addr (0x%x)\n", node->cached_p_addr);
        if(FAILED(CeCloseCallerBuffer(pDestMarshalledOut, pOutBuf, nOutBufSize, ARG_O_PTR)))    {
            printD("[IOCTL_CODEC_GET_PHY_ADDR] Access check failed on output buffer\n");
            return FALSE;
        }
        break;


    default : RETAILMSG(1, (TEXT("[CMM_IOControl] CMM Invalid IOControl\r\n")));
    }


    UnlockCMMMutex();
    return result;
}

/*----------------------------------------------------------------------------
*Function: CMM_DllMain

*Parameters:         DllInstance        :
                    Reason            : 
                    Reserved        :
*Return Value:        True/False
*Implementation Notes: Entry point for CMM.dll 
-----------------------------------------------------------------------------*/
BOOL WINAPI
CMM_DllMain(HINSTANCE DllInstance, DWORD Reason, LPVOID Reserved)
{
    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER(DllInstance);
            break;
    }
    return TRUE;
}   

// insert node ahead of AllocMemHead
static void InsertNodeToAllocList(ALLOC_MEM_T *node, UINT8 inst_no)
{
    printD("[%d]InsertNodeToAllocList(p_addr : 0x%08x size:%ld cacheflag : %d)\n", inst_no, node->cached_p_addr, node->size, node->cacheFlag);
    node->next = AllocMemHead;
    node->prev = AllocMemHead->prev;
    AllocMemHead->prev->next = node;
    AllocMemHead->prev = node;
    AllocMemHead = node;
    printD("end InsertNodeToAllocList\n");
}

// insert node ahead of FreeMemHead
static void InsertNodeToFreeList(FREE_MEM_T *node,  UINT8 inst_no)
{
    printD("[%d]InsertNodeToFreeList(startAddr : 0x%08x size:%ld)\n", inst_no, node->startAddr, node->size);
    node->next = FreeMemHead;
    node->prev = FreeMemHead->prev;
    FreeMemHead->prev->next = node;
    FreeMemHead->prev = node;
    FreeMemHead = node;

    PrintList();
}

static void DeleteNodeFromAllocList(ALLOC_MEM_T *node, UINT8 inst_no)
{
    printD("[%d]DeleteNodeFromAllocList(p_addr : 0x%08x size:%ld cacheflag : %d)\n", inst_no, node->cached_p_addr, node->size, node->cacheFlag);
    __try
    {
        if(node == AllocMemTail){
            RETAILMSG(1, (TEXT("[CMM] DeleteNodeFromAllocList :: InValid node\n")));
            return;
        }

        if(node == AllocMemHead)
            AllocMemHead = node->next;

        node->prev->next = node->next;
        node->next->prev = node->prev;

        free(node);
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        RETAILMSG( 1, ( _T("CMM DeleteNodeFromAllocList exception occurred\n")) );
    }

    PrintList();
}

static void DeleteNodeFromFreeList( FREE_MEM_T *node, UINT8 inst_no)
{
    printD("[%d]DeleteNodeFromFreeList(startAddr : 0x%08x size:%ld)\n", inst_no, node->startAddr, node->size);
    __try
    {
        if(node == FreeMemTail){
            RETAILMSG(1, (TEXT("[CMM] DeleteNodeFromFreeList :: InValid node\n")));
            return;
        }

        if(node == FreeMemHead)
            FreeMemHead = node->next;

        node->prev->next = node->next;
        node->next->prev = node->prev;

        free(node);
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        RETAILMSG( 1, ( _T("CMM DeleteNodeFromFreeList exception occurred\n")) );
    }
}


static void ReleaseAllocMem(ALLOC_MEM_T *node, CODEC_MEM_CTX *CodecMem)
{
    FREE_MEM_T *free_node;
    BOOL        r;

    __try
    {
#if    (_WIN32_WCE >= 600)    
    #ifdef E2E_CMM
    {
        printD("decommit CodecAddr\n");
        r = VirtualFreeEx(CodecMem->callerProcess,    // HANDLE hProcess
            node->u_addr,
            0,
            MEM_RELEASE);
        if (r == FALSE)
        {
            RETAILMSG(1, (L"[%d][CMM_Close] CMM VirtualFreeEx returns FALSE.(u_addr : 0x%08x cacheFlag : %d callerprocess:%ld, Err:0x%x)\n", 
            CodecMem->inst_no, node->u_addr, node->cacheFlag, CodecMem->callerProcess, GetLastError()));
        }
    }
    #else
    {
        printD("decommit CodecAddr\n");
        r = VirtualFreeEx(CodecMem->callerProcess,    // HANDLE hProcess
            node->u_addr,
            node->size,
            MEM_DECOMMIT);
        if (r == FALSE)
        {
            RETAILMSG(1, (L"[%d][CMM_Close] CMM VirtualFreeEx returns FALSE.(u_addr : 0x%08x cacheFlag : %d callerprocess:%ld)\n", 
            CodecMem->inst_no, node->u_addr, node->cacheFlag, CodecMem->callerProcess));
        }
    }
    #endif
#endif

        FreeCodecBuff(node);

        if(node->cacheFlag){
            free_node = (FREE_MEM_T    *)malloc(sizeof(FREE_MEM_T));
            if(free_node != NULL)
            {
                free_node->startAddr = node->cached_p_addr;
                free_node->size = node->size;
                InsertNodeToFreeList(free_node, CodecMem->inst_no);
            }
            else
            {
                RETAILMSG(1, (L"[%d][CMM_Close] Fail to create free node with cacheFlag\n"));
            }
        }

        // Delete from AllocMem list
        DeleteNodeFromAllocList(node, CodecMem->inst_no);
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        RETAILMSG( 1, ( _T("CMM ReleaseAllocMem exception occurred\n")) );
    }
}



// Remove Fragmentation in FreeMemList
static void MergeFragmentation(UINT8 inst_no)
{
    FREE_MEM_T *node1, *node2;

    node1 = FreeMemHead;

    while(node1 != FreeMemTail){
        node2 = FreeMemHead;
        while(node2 != FreeMemTail){
            if(node1->startAddr + node1->size == node2->startAddr){
                node1->size += node2->size;
                printD("find merge area !! ( node1->startAddr + node1->size == node2->startAddr)\n");
                DeleteNodeFromFreeList(node2, inst_no);
                break;
            }
            else if(node1->startAddr == node2->startAddr + node2->size){
                printD("find merge area !! ( node1->startAddr == node2->startAddr + node2->size)\n");
                node1->startAddr = node2->startAddr;
                node1->size += node2->size;
                DeleteNodeFromFreeList(node2, inst_no);
                break;
            }
            node2 = node2->next;
        }
        node1 = node1->next;
    }
}
static DWORD GetMemArea(UINT32 allocSize, UINT8 inst_no)
{
    FREE_MEM_T *node = NULL;
    FREE_MEM_T *match_node = NULL;
    DWORD       allocAddr = 0;
    int         i = 0;

    printD("request Size : %ld\n", allocSize);
    
    if(FreeMemHead == FreeMemTail){
        RETAILMSG(1, (TEXT("All memory is gone\r\n")));
        return(allocAddr);
    }

    // find best chunk of memory
    for(node = FreeMemHead; node != FreeMemTail; node = node->next){

        if(match_node != NULL)
        {
            if((node->size >= allocSize) && (node->size < match_node->size))
            {
                match_node = node;
            }
            else
            {
            //???
            }
        }
        else
        {
            if(node->size >= allocSize)
            {
                match_node = node;
            }
            else
            {
                // ???
            }
        }                

    }
    printD("match : startAddr(0x%08x) size(%ld)\n", (match_node == NULL) ? 0 : match_node->startAddr, (match_node == NULL) ? 0 : match_node->size);

    // rearange FreeMemArea
    if(match_node != NULL){
        allocAddr = match_node->startAddr;
        match_node->startAddr += allocSize;
        match_node->size -= allocSize;
        
        if(match_node->size == 0)          // delete match_node.
             DeleteNodeFromFreeList(match_node, inst_no);

        return(allocAddr);
    }
    else RETAILMSG(1, (TEXT("there is no suitable chunk\r\n")));

    return(allocAddr);
}


static ALLOC_MEM_T * GetCodecVirAddr(UINT8 inst_no, CMM_ALLOC_PRAM_T *in_param)
{
    DWORD                    p_startAddr;
    ALLOC_MEM_T             *p_allocMem;
    DMA_ADAPTER_OBJECT     Adapter;
    UINT8                    *v_addr;
    PHYSICAL_ADDRESS        p_addr;


    //
    // if user request cachable area, allocate from reserved area
    // if user request uncachable area, allocate dynamically
    //

    printD("GetCodecVirAddr \n");

    __try
    {
        if(in_param->cacheFlag){
            p_startAddr = GetMemArea((UINT32)in_param->size, inst_no);

            if(!p_startAddr){
                RETAILMSG(1, (L"[CMM:ERR] There is no more memory\n\r"));
                return NULL;
            }

            p_allocMem = (ALLOC_MEM_T *)malloc(sizeof(ALLOC_MEM_T));
            if(p_allocMem == NULL)
            {
                RETAILMSG(1, (L"[CMM:ERR] There is no more memory\n\r"));
                return NULL;
            }
            memset(p_allocMem, 0x00, sizeof(ALLOC_MEM_T));

            p_allocMem->cached_p_addr = p_startAddr;
            p_allocMem->v_addr =  CachedVirAddr + (p_allocMem->cached_p_addr - CODEC_MEM_START);

            if (p_allocMem->v_addr == NULL)
            {
                RETAILMSG(1, (L"[CMM:ERR] Phy2Vir_AddrMapping() : Mapping Failed [PA:0x%08x]\n\r", p_allocMem->cached_p_addr));
                free(p_allocMem);
                return NULL;
            }
            printD("v_addr : 0x%x p_addr : 0x%x\n", p_allocMem->v_addr, p_allocMem->cached_p_addr);
        }
        else{
            memset(&Adapter, 0, sizeof(DMA_ADAPTER_OBJECT));
            Adapter.InterfaceType = Internal;
            Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

            v_addr = (PBYTE)HalAllocateCommonBuffer(&Adapter, (UINT32)in_param->size, &p_addr, FALSE);
            if(v_addr == NULL){
                RETAILMSG(1, (TEXT("[CMM] dynamic allocation error\r\n")));
                return NULL;
            }

            p_allocMem = (ALLOC_MEM_T *)malloc(sizeof(ALLOC_MEM_T));
            if(p_allocMem == NULL)
            {
                RETAILMSG(1, (L"[CMM:ERR] There is no more memory\n\r"));
                return NULL;
            }
            memset(p_allocMem, 0x00, sizeof(ALLOC_MEM_T));

            p_allocMem->uncached_p_addr = p_addr;
            p_allocMem->v_addr = v_addr;

            printD("v_addr : 0x%x p_addr : 0x%x\n", p_allocMem->v_addr, p_allocMem->uncached_p_addr.LowPart);
        }

        p_allocMem->size = (UINT32)in_param->size;
        p_allocMem->inst_no = inst_no;
        p_allocMem->cacheFlag = in_param->cacheFlag;

        InsertNodeToAllocList(p_allocMem, inst_no);
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        RETAILMSG( 1, ( _T("CMM GetCodecVirAddr Exception occurred.\n")) );
        return NULL;
    }

    return(p_allocMem);
}

static void FreeCodecBuff(ALLOC_MEM_T *node)
{
    DMA_ADAPTER_OBJECT Adapter;

    printD("FreeCodecBuff \n");

    __try
    {
        if(!node->cacheFlag){
            memset(&Adapter, 0, sizeof(DMA_ADAPTER_OBJECT));
            Adapter.InterfaceType = Internal;
            Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

            HalFreeCommonBuffer(&Adapter, 0, node->uncached_p_addr, (PVOID)node->v_addr, FALSE);     
        }
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        RETAILMSG( 1, ( _T("CMM FreeCodecBuff Exception occurred\n")) );
    }
}

static UINT8 GetInstanceNo()
{
    UINT8    i;

    for(i = 0; i < MAX_INSTANCE_NUM; i++)
    {
        if(instanceNo[i] == FALSE)
        {
            instanceNo[i] = TRUE;
            return i;
        }
    }
    // Instance is all created, there is no room
    return -1;
}

static void ReturnInstanceNo(UINT8 inst_no)
{
    instanceNo[inst_no] = FALSE;

}


static void PrintList()
{
    ALLOC_MEM_T    *node1;
    FREE_MEM_T        *node2;
    int                 count = 0;
    DWORD            p_addr;

    for(node1 = AllocMemHead; node1 != AllocMemTail; node1 = node1->next){
        if(node1->cacheFlag)
            p_addr = node1->cached_p_addr;
        else
            p_addr = (DWORD)node1->uncached_p_addr.LowPart;
        
        printD("     [AllocList][%d] inst_no : %d p_addr : 0x%08x v_addr:0x%08x size:%ld cacheflag : %d\n", 
            count++, node1->inst_no,  p_addr, node1->v_addr, node1->size, node1->cacheFlag);

    }
                
    count = 0;
    for(node2 = FreeMemHead; node2 != FreeMemTail; node2 = node2->next){
            printD("     [FreeList][%d] startAddr : 0x%08x size:%ld\n", count++, node2->startAddr , node2->size);

    }
    
    
}
