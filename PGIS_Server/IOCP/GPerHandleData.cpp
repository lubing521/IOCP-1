#include "stdafx.h"
#include "GLog.h"
#include "GMemory.h"
#include "GPerHandleData.h"
#include "GSocket.h"

/*********************************************************************************
                   常量定义
*********************************************************************************/
#define	COUNT_GHND_DATA_DEF					1100

/*********************************************************************************
                   默认函数定义
*********************************************************************************/
void GHndDat_OnCreateDef(DWORD dwClientContext)
{
}

void GHndDat_OnDestroyDef(DWORD dwClientContext)
{
}

/*********************************************************************************
                   变量定义
*********************************************************************************/
PFN_ON_GHND_EVENT			pfnGHndDatOnCreate = GHndDat_OnCreateDef;
PFN_ON_GHND_EVENT			pfnGHndDatOnDestroy = GHndDat_OnDestroyDef;

DWORD						dwGHndDataMemBytes = 0;
DWORD						dwGHndDataTotal = COUNT_GHND_DATA_DEF;
PGHND_DATA					pGHndDataPoolAddr = NULL;
DWORD						dwGHndDataAlloc = 0;
DWORD						dwGHndDataFree = 0;
PGHND_DATA					pGHndDataPoolHead = NULL;
PGHND_DATA					pGHndDataPoolTail = NULL;
CRITICAL_SECTION			GHndDataPoolHeadCS;
CRITICAL_SECTION			GHndDataPoolTailCS;
BOOL						bGHndDataIsActive = FALSE;

/*********************************************************************************
                   Handle参数获取和设置
*********************************************************************************/
/***
 *名称:GHndDat_GetMemBytes
 *说明:获取GHndData占内存数
 *输入:
 *输出:
 *条件:
 *******************************************************************************/
DWORD GHndDat_GetMemBytes(void)
{
	return(dwGHndDataMemBytes);
}

DWORD GHndDat_GetTotal(void)
{
	return(dwGHndDataTotal);
}

DWORD GHndDat_GetSize(void)
{
	return(sizeof(GHND_DATA));
}

void GHndDat_SetTotal(DWORD dwTotal)
{
	if(bGHndDataIsActive || (!dwTotal))
		return;
	dwGHndDataTotal = dwTotal;
}

DWORD GHndDat_GetUsed(void)
{
	return(dwGHndDataAlloc - dwGHndDataFree);
}

GHND_TYPE GHndDat_GetType(DWORD dwClientContext)
{
	return(((PGHND_DATA)dwClientContext)->htType);
}

GHND_STATE GHndDat_GetState(DWORD dwClientContext)
{
	return(((PGHND_DATA)dwClientContext)->hsState);
}

DWORD GHndDat_GetAddr(DWORD dwClientContext)
{
	return(((PGHND_DATA)dwClientContext)->dwAddr);
}

DWORD GHndDat_GetPort(DWORD dwClientContext)
{
	return(((PGHND_DATA)dwClientContext)->dwPort);
}

DWORD GHndDat_GetTickCountAcitve(DWORD dwClientContext)
{
	return(((PGHND_DATA)dwClientContext)->dwTickCountAcitve);
}

DWORD GHndDat_GetOwner(DWORD dwClientContext)
{
	return((DWORD)((PGHND_DATA)dwClientContext)->pOwner);
}

void* GHndDat_GetData(DWORD dwClientContext)
{
	return(((PGHND_DATA)dwClientContext)->pData);
}

void GHndDat_SetData(DWORD dwClientContext, void* pData)
{
	((PGHND_DATA)dwClientContext)->pData = pData;
}

void GHndDat_SetProcOnHndCreate(PFN_ON_GHND_EVENT pfnOnProc)
{
	if(pfnOnProc)
		pfnGHndDatOnCreate = pfnOnProc;
}

void GHndDat_SetProcOnHndDestroy(PFN_ON_GHND_EVENT pfnOnProc)
{
	if(pfnOnProc)
		pfnGHndDatOnDestroy = pfnOnProc;
}

/*********************************************************************************
                   句柄池管理
*********************************************************************************/
PGHND_DATA GHndDat_Alloc(void)
{
	EnterCriticalSection(&GHndDataPoolHeadCS);
	if(pGHndDataPoolHead->pNext)
	{
		PGHND_DATA Result;

		Result = pGHndDataPoolHead;
		pGHndDataPoolHead = pGHndDataPoolHead->pNext;
		dwGHndDataAlloc++;
		LeaveCriticalSection(&GHndDataPoolHeadCS);
		return(Result);
	}else
	{
		LeaveCriticalSection(&GHndDataPoolHeadCS);
		GLog_Write("GHndDat_Alloc：分配句柄上下文失败");
		return(NULL);
	}
}

void GHndDat_Free(PGHND_DATA pHndData)
{
	EnterCriticalSection(&GHndDataPoolTailCS);
	pHndData->pNext = NULL;
	pGHndDataPoolTail->pNext = pHndData;
	pGHndDataPoolTail = pHndData;
	dwGHndDataFree++;
	LeaveCriticalSection(&GHndDataPoolTailCS);
}

void GHndDat_Create(PFN_ON_GHND_CREATE pfnOnCreate)
{
	if(bGHndDataIsActive)
		return;	

	dwGHndDataMemBytes = dwGHndDataTotal * sizeof(GHND_DATA);
	pGHndDataPoolAddr = (PGHND_DATA)GMem_Alloc(dwGHndDataMemBytes);
	if(!pGHndDataPoolAddr)
	{
		dwGHndDataMemBytes = 0;
		GLog_Write("GHndDat_Create：从GMem分配句柄上下文所需内存失败");
		return;
	}

	DWORD i;

	dwGHndDataAlloc = 0;
	dwGHndDataFree = 0;
	pGHndDataPoolHead = pGHndDataPoolAddr;
	pGHndDataPoolTail = pGHndDataPoolAddr;

	for(i = 0; i < dwGHndDataTotal - 1; i++)
	{
		if(pfnOnCreate)
			pfnOnCreate(pGHndDataPoolTail);
		pfnGHndDatOnCreate((DWORD)pGHndDataPoolTail);
		pGHndDataPoolTail->pNext = pGHndDataPoolTail + 1;
		pGHndDataPoolTail = pGHndDataPoolTail->pNext;
	}
	if(pfnOnCreate)
		pfnOnCreate(pGHndDataPoolTail);
	pfnGHndDatOnCreate((DWORD)pGHndDataPoolTail);		
	pGHndDataPoolTail->pNext = NULL;
	InitializeCriticalSection(&GHndDataPoolHeadCS);
	InitializeCriticalSection(&GHndDataPoolTailCS);
	
	bGHndDataIsActive = TRUE;
}

void GHndDat_Destroy(PFN_ON_GHND_DESTROY pfnOnDestroy)
{
	bGHndDataIsActive = FALSE;
	if(!pGHndDataPoolAddr)
		return;

	PGHND_DATA pHndData;

	pHndData = pGHndDataPoolHead;
	while(pHndData)
	{
		pfnGHndDatOnDestroy((DWORD)pHndData);
		if(pfnOnDestroy)
			pfnOnDestroy(pHndData);
		pHndData = pHndData->pNext;
	}

	GMem_Free(pGHndDataPoolAddr);
	pGHndDataPoolAddr = NULL;
	pGHndDataPoolHead = NULL;
	pGHndDataPoolTail = NULL;
	dwGHndDataAlloc = 0;
	dwGHndDataFree = 0;
	DeleteCriticalSection(&GHndDataPoolHeadCS);
	DeleteCriticalSection(&GHndDataPoolTailCS);
	dwGHndDataMemBytes = 0;
}
