#pragma once
using ULONG=unsigned long;
using CONST_STRPTR=const char *;
struct MsgPort{int mp_SigBit=4;};
struct IORequest{int io_Command=0;};
struct TestTime{long tv_secs=0,tv_micro=0;};
struct timerequest{IORequest tr_node;TestTime tr_time;};
#define TIMERNAME "timer.device"
#define UNIT_MICROHZ 0
#define TR_ADDREQUEST 9
