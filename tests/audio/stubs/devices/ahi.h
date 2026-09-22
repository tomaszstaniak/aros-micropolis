#ifndef AUDIO_TEST_AHI_H
#define AUDIO_TEST_AHI_H
#include <cstddef>
#include <cstdint>
struct MsgPort {};
struct IORequest { unsigned io_Command=0,io_Flags=0; int io_Error=0; unsigned io_Offset=0; void* io_Data=nullptr; size_t io_Length=0; };
using IOStdReq=IORequest;
struct AHIRequest { IOStdReq ahir_Std; unsigned ahir_Version=0,ahir_Type=0,ahir_Frequency=0,ahir_Volume=0,ahir_Position=0; AHIRequest* ahir_Link=nullptr; };
#define AHINAME "ahi.device"
#define AHI_DEFAULT_UNIT 0
#define CMD_WRITE 3
#define AHIST_M16S 1
MsgPort* CreateMsgPort();
void* CreateIORequest(MsgPort*,size_t);
int OpenDevice(const char*,unsigned,IORequest*,unsigned);
IORequest* CheckIO(IORequest*);
void WaitIO(IORequest*);
void SendIO(IORequest*);
void AbortIO(IORequest*);
void CloseDevice(IORequest*);
void DeleteIORequest(IORequest*);
void DeleteMsgPort(MsgPort*);
#endif
