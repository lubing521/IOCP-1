#include "stdafx.h"

#include "GThread.h"
#include "GSocket.h"

/*********************************************************************************
                   默认事件处理
*********************************************************************************/
void OnGThreadBeginDef(const DWORD dwWorkerId)
{
}

void OnGThreadEndDef(const DWORD dwWorkerId)
{
}

/*********************************************************************************
                   变量定义和初始化
*********************************************************************************/
PFN_ON_GTHREAD_EVENT	pfnOnGThreadBegin = OnGThreadBeginDef;
PFN_ON_GTHREAD_EVENT	pfnOnGThreadEnd = OnGThreadBeginDef;

PGTHREAD			pGThreadHead = NULL;
DWORD				dwGThreadCount = 0;
CRITICAL_SECTION	GThreadCS;
BOOL				bGThreadIsActive = FALSE;

/*********************************************************************************
                   默认线程体
*********************************************************************************/
DWORD WINAPI GThreadProc(PGTHREAD pThread)
{
	pfnOnGThreadBegin((DWORD)pThread);
	((PFN_ON_GTHREAD_PROC)(pThread->pfnThreadProc))(pThread);
	pfnOnGThreadEnd((DWORD)pThread);
	SetEvent(pThread->hFinished);
	return(0);
}

/*********************************************************************************
                变量设置与获取
*********************************************************************************/
void GThrd_SetEventProc(PFN_ON_GTHREAD_EVENT pfnOnBeginProc, PFN_ON_GTHREAD_EVENT pfnOnEndProc)
{
	if(bGThreadIsActive)
		return;
	if(pfnOnBeginProc)
		pfnOnGThreadBegin = pfnOnBeginProc;
	if(pfnOnEndProc)
		pfnOnGThreadEnd = pfnOnEndProc;
}

DWORD GThrd_GetThreadCount(void)
{
	return(dwGThreadCount);
}

/*********************************************************************************
                GThread参数获取与设置
*********************************************************************************/
DWORD GThrd_GetThreadId(DWORD dwThreadContext)
{
	return(PGTHREAD(dwThreadContext)->dwThreadId);
}

DWORD GThrd_GetRunCount(DWORD dwThreadContext)
{
	return(PGTHREAD(dwThreadContext)->dwRunCount);
}

DWORD GThrd_GetState(DWORD dwThreadContext)
{
	return(PGTHREAD(dwThreadContext)->dwState);
}

DWORD GThrd_GetType(DWORD dwThreadContext)
{
	return(PGTHREAD(dwThreadContext)->ttType);
}

char* GThrd_GetName(DWORD dwThreadContext)
{
	return(PGTHREAD(dwThreadContext)->pThreadName);
}

void* GThrd_GetData(void)
{
	DWORD dwThreadId;
	PGTHREAD pThread;
	void* Result = NULL;

	EnterCriticalSection(&GThreadCS);

	dwThreadId = GetCurrentThreadId();
	pThread = pGThreadHead;
	while(pThread)
	{
		if(dwThreadId == pThread->dwThreadId)
		{
			Result = pThread->pData;
			break;
		}
		pThread = pThread->pNext;
	}

	LeaveCriticalSection(&GThreadCS);

	return(Result);
}

void GThrd_SetData(DWORD dwThreadContext, void* pData)
{
	PGTHREAD(dwThreadContext)->pData = pData;
}

void GThrd_TraversalThread(DWORD dwParam, PFN_ON_GSOCK_TRAVERSAL pfnOnProc)
{
	if(!bGThreadIsActive)
		return;

	EnterCriticalSection(&GThreadCS);

	PGTHREAD pThrd;
	DWORD dwIndex = 0;

	pThrd = pGThreadHead;
	while(pThrd)
	{
		pfnOnProc(dwParam, dwIndex, (DWORD)pThrd);
		dwIndex++;
		pThrd = pThrd->pNext;
	}

	LeaveCriticalSection(&GThreadCS);
}

/*********************************************************************************
                   GThread操作
*********************************************************************************/
void GThrd_CreateThread(PGTHREAD pThread, GTHREAD_TYPE ttType, char* szThreadName, PFN_ON_GTHREAD_PROC pfnThreadProc)
{
	ZeroMemory(pThread, sizeof(GTHREAD));
	pThread->ttType = ttType;
	pThread->pThreadName = szThreadName;
	pThread->pfnThreadProc = pfnThreadProc;

	EnterCriticalSection(&GThreadCS);
	pThread->pNext = pGThreadHead;
	pGThreadHead = pThread;
	dwGThreadCount++;
	LeaveCriticalSection(&GThreadCS);

	pThread->hFinished = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&GThreadProc, pThread, 0, &pThread->dwThreadId);
}

void GThrd_DestroyThread(PGTHREAD pThread)
{
	pThread->bIsShutdown = TRUE;
	WaitForSingleObject(pThread->hFinished, INFINITE);

	EnterCriticalSection(&GThreadCS);

	PGTHREAD pThrd;

	if(pThread == pGThreadHead)
	{
		dwGThreadCount--;
		pGThreadHead = pGThreadHead->pNext;
	}else
	{
		pThrd = pGThreadHead;
		while(pThrd)
		{
			if(pThrd->pNext == pThread)
			{
				pThrd->pNext = pThrd->pNext->pNext;				
				dwGThreadCount--;
				break;
			}
			pThrd = pThrd->pNext;
		}
	}

	LeaveCriticalSection(&GThreadCS);
	CloseHandle(pThread->hFinished);
}

void GThrd_Create(void)
{
	if(bGThreadIsActive)
		return;
	InitializeCriticalSection(&GThreadCS);
	pGThreadHead = NULL;
	dwGThreadCount = 0;
	bGThreadIsActive = TRUE;
}

void GThrd_Destroy(void)
{
	if(!bGThreadIsActive)
		return;
	bGThreadIsActive = FALSE;
	pGThreadHead = NULL;
	dwGThreadCount = 0;
	DeleteCriticalSection(&GThreadCS);
}