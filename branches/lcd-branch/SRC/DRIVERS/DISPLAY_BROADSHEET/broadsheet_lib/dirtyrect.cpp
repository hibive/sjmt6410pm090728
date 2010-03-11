
#include <windows.h>
#include "dirtyrect.h"
#include <queue>
using namespace std;


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)


typedef queue<RECT> QDIRTYRECT;
typedef struct
{
	HANDLE		hEvent;
	BOOL		bThread;
	HANDLE		hThread;
	QDIRTYRECT	qDirtyRect;
	CBWORK		cbWork;
} SDIRTYRECT, *PSDIRTYRECT;


static CRITICAL_SECTION	g_CS;
static SDIRTYRECT g_DirtyRect = {FALSE, };


static int queueSize(HDIRTYRECT hDR)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	EnterCriticalSection(&g_CS);
	int nSize = (int)pDR->qDirtyRect.size();
	LeaveCriticalSection(&g_CS);
	return nSize;
}
static void queuePush(HDIRTYRECT hDR, RECT rect)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	EnterCriticalSection(&g_CS);
	pDR->qDirtyRect.push(rect);
	LeaveCriticalSection(&g_CS);
}
static void queuePop(HDIRTYRECT hDR, PRECT pdr)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	EnterCriticalSection(&g_CS);
	if (pDR->qDirtyRect.size())
	{
		*pdr = pDR->qDirtyRect.front();
		pDR->qDirtyRect.pop();
	}
	LeaveCriticalSection(&g_CS);
}

static DWORD wqThreadProc(LPVOID lpParameter)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)lpParameter;
	DWORD dwStatus, dwTimeout = INFINITE;
	RECT rectDirty, rect;
	BOOL bIsRect;

	while (pDR->bThread)
	{
		dwStatus = WaitForSingleObject(pDR->hEvent, dwTimeout);
		switch (dwStatus)
		{
        case WAIT_OBJECT_0:
			rectDirty.left = rectDirty.top = 10000;
			rectDirty.right = rectDirty.bottom = 0;
			bIsRect = FALSE;
			while (0 < queueSize(pDR))
			{
				queuePop(pDR, &rect);
				rectDirty.left = min(rectDirty.left, rect.left);
				rectDirty.top = min(rectDirty.top, rect.top);
				rectDirty.right = max(rectDirty.right, rect.right);
				rectDirty.bottom = max(rectDirty.bottom, rect.bottom);
				bIsRect = TRUE;
			}
			if (bIsRect)
			{
				MYMSG((_T("===> rectDirty(%d,%d - %d,%d)\r\n"),
					rectDirty.left, rectDirty.top,
					rectDirty.right, rectDirty.bottom));
				pDR->cbWork(rectDirty);
			}
			break;
		case WAIT_TIMEOUT:
			break;
		default:
			break;
		}
	}

	return 0;
}


HDIRTYRECT DirtyRect_Init(CBWORK cbWork)
{
	PSDIRTYRECT pDR = &g_DirtyRect;

	pDR->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == pDR->hEvent)
	{
		MYERR((_T("NULL == pDR->hEvent\r\n")));
		goto goto_err;
	}

	pDR->bThread = TRUE;
	pDR->hThread = CreateThread(NULL, 0, wqThreadProc, pDR, 0, NULL);
	if (NULL == pDR->hThread)
	{
		MYERR((_T("NULL == pDR->hThread\r\n")));
		goto goto_err;
	}
	//default(251)//CeSetThreadPriority(pDR->hThread, 200);

	pDR->cbWork = cbWork;

	InitializeCriticalSection(&g_CS);

	return (HDIRTYRECT)pDR;
goto_err:
	DirtyRect_Destroy(pDR);
	return NULL;
}

void DirtyRect_Add(HDIRTYRECT hDR, RECT rect)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	queuePush(hDR, rect);
	SetEvent(pDR->hEvent);
}

void DirtyRect_Destroy(HDIRTYRECT hDR)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	if (pDR->hEvent)
	{
		pDR->bThread = FALSE;
		SetEvent(pDR->hEvent);
	}

	if (pDR->hThread)
	{
		WaitForSingleObject(pDR->hThread, INFINITE);
		CloseHandle(pDR->hThread);
		pDR->hThread = NULL;
	}

	if (pDR->hEvent)
	{
		CloseHandle(pDR->hEvent);
		pDR->hEvent = NULL;
	}
}

