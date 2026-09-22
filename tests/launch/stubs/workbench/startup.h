#pragma once
#include <proto/dos.h>
// Only the fields LaunchDirectory reads.
struct WBArg { BPTR wa_Lock; char *wa_Name; };
struct WBStartup { long sm_NumArgs; WBArg *sm_ArgList; };
