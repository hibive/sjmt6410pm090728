
#include <windows.h>
#include "dirtyrect.h"
#include <queue>
using namespace std;


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)


typedef queue<RECT> QDIRTYRECT;
typedef struct
{
	BOOL		bIsFirst;
	HANDLE		hEvnet;
	BOOL		bThread;
	HANDLE		hThread;
	HANDLE		hMutex;
	QDIRTYRECT	qDirtyRect;
	CBWORK		cbWork;
} SDIRTYRECT, *PSDIRTYRECT;


SDIRTYRECT g_DirtyRect = {FALSE, };


static int queueSize(HDIRTYRECT hDR)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;
	if (WAIT_OBJECT_0 != WaitForSingleObject(pDR->hMutex, INFINITE))
	{
		MYERR((_T("WAIT_OBJECT_0 != WaitForSingleObject\r\n")));
		return -1;
	}

	int nSize = (int)pDR->qDirtyRect.size();
	ReleaseMutex(pDR->hMutex);
	return nSize;
}
static int queuePush(HDIRTYRECT hDR, RECT rect)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;
	if (WAIT_OBJECT_0 != WaitForSingleObject(pDR->hMutex, INFINITE))
	{
		MYERR((_T("WAIT_OBJECT_0 != WaitForSingleObject\r\n")));
		return 0;
	}

	pDR->qDirtyRect.push(rect);
	int nSize = (int)pDR->qDirtyRect.size();
	ReleaseMutex(pDR->hMutex);
	return nSize;
}
static int queuePop(HDIRTYRECT hDR, PRECT pdr)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;
	if (WAIT_OBJECT_0 != WaitForSingleObject(pDR->hMutex, INFINITE))
	{
		MYERR((_T("WAIT_OBJECT_0 != WaitForSingleObject\r\n")));
		return 0;
	}
	if (pDR->qDirtyRect.size())
	{
		*pdr = pDR->qDirtyRect.front();
		pDR->qDirtyRect.pop();
	}
	int nSize = (int)pDR->qDirtyRect.size();
	ReleaseMutex(pDR->hMutex);
	return nSize;
}

DWORD wqThreadProc(LPVOID lpParameter)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)lpParameter;
	DWORD dwStatus, dwTimeout = INFINITE;
	RECT rectDirty, rect;
	BOOL bIsRect;

	while (pDR->bThread)
	{
		dwStatus = WaitForSingleObject(pDR->hEvnet, dwTimeout);
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

	if (TRUE == pDR->bIsFirst)
	{
		MYERR((_T("TRUE == g_DirtyRect.bIsFirst\r\n")));
		goto goto_err;
	}

	pDR->hEvnet = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == pDR->hEvnet)
	{
		MYERR((_T("NULL == pDR->hEvnet\r\n")));
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

	pDR->hMutex = CreateMutex(NULL, FALSE, NULL);
	if (NULL == pDR->hMutex)
	{
		MYERR((_T("NULL == pDR->hMutex\r\n")));
		goto goto_err;
	}

	pDR->cbWork = cbWork;
	pDR->bIsFirst = TRUE;

	return (HDIRTYRECT)pDR;
goto_err:
	DirtyRect_Destroy(pDR);
	return NULL;
}

void DirtyRect_Add(HDIRTYRECT hDR, RECT rect)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	queuePush(hDR, rect);
	SetEvent(pDR->hEvnet);
}

void DirtyRect_Destroy(HDIRTYRECT hDR)
{
	PSDIRTYRECT pDR = (PSDIRTYRECT)hDR;

	if (pDR->hMutex)
	{
		CloseHandle(pDR->hMutex);
		pDR->hMutex = NULL;
	}

	if (pDR->hEvnet)
	{
		pDR->bThread = FALSE;
		SetEvent(pDR->hEvnet);
	}

	if (pDR->hThread)
	{
		WaitForSingleObject(pDR->hThread, 2000/*INFINITE*/);
		CloseHandle(pDR->hThread);
		pDR->hThread = NULL;
	}

	if (pDR->hEvnet)
	{
		CloseHandle(pDR->hEvnet);
		pDR->hEvnet = NULL;
	}

	pDR->bIsFirst = FALSE;
}

