#include <ntddk.h>
#include "helper.h"


PVOID get_ntoskrnl_base(){
    void *base = NULL;
    return RtlPcToFileHeader(KdDebuggerEnabled, &base);
};