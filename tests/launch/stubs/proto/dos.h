#pragma once
#include <cstdint>
using BPTR=uintptr_t; using CONST_STRPTR=const char *; using STRPTR=char *;
#define SHARED_LOCK -2
BPTR Lock(CONST_STRPTR,int);int AddPart(STRPTR,CONST_STRPTR,unsigned long);void UnLock(BPTR);int NameFromLock(BPTR,STRPTR,long);BPTR CurrentDir(BPTR);
