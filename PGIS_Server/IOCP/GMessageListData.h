#pragma once

#include "GSocket.h"

typedef struct _GHND_MESSAGELIST_DATA
{
_GHND_MESSAGELIST_DATA*		pNext;
_GHND_MESSAGELIST_DATA*		pPrior;
GHND_MESSAGETYPE			htType;
DWORD						dwMessageID;
DWORD						dwTickCountAcitve;
DWORD						dwClientContext;
char						szData[256];
void*						pOwner;
}GHND_MESSAGELIST_DATA, *PGHND_MESSAGELIST_DATA;

typedef	BOOL(*PFN_ON_GHND_MESSAGELIST_CREATE)(PGHND_MESSAGELIST_DATA pHndData);
typedef	void(*PFN_ON_GHND_MESSAGELIST_DESTROY)(PGHND_MESSAGELIST_DATA pHndData);

PGHND_MESSAGELIST_DATA GHndMesListDat_Alloc(void);
void GHndMesListDat_Free(PGHND_MESSAGELIST_DATA pHndData);

extern BOOL bGHndMesListDataIsActive;
void GHndMesListDat_Create(PFN_ON_GHND_MESSAGELIST_CREATE pfnOnCreate);
void GHndMesListDat_Destroy(PFN_ON_GHND_MESSAGELIST_DESTROY pfnOnDestroy);


