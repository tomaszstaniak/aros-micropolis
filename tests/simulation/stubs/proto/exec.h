#pragma once
#include <devices/timer.h>
MsgPort *CreateMsgPort();
void *CreateIORequest(MsgPort *,unsigned long);
int OpenDevice(CONST_STRPTR,int,IORequest *,int);
void CloseDevice(IORequest *);
void DeleteIORequest(IORequest *);
void DeleteMsgPort(MsgPort *);
void SendIO(IORequest *);
IORequest *CheckIO(IORequest *);
int WaitIO(IORequest *);
void AbortIO(IORequest *);
