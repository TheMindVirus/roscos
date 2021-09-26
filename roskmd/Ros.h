#pragma once

#define DBG   1
#define debug(...)   \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)); \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"));
//#define debug(FORMAT, ...) __VA_ARGS__;

#include <initguid.h>
#include <ntddk.h>
#include <ntintsafe.h>
#include <ntstrsafe.h>
#include <ntverp.h>

#define ENABLE_DXGK_SAL

extern "C"
{
    #include <dispmprt.h>
    #include <dxgiformat.h>
    //#include <WppRecorder.h>
}

#include "RosAllocation.h"
#include "RosAdapter.h"
#include "RosLogging.h"
#include "RosGpuCommand.h"

#include "RosKmd.h"
#include "RosKmdAllocation.h"
#include "RosKmdAdapter.h"
#include "RosKmdRapAdapter.h"
#include "RosKmdSoftAdapter.h"
#include "RosKmdDdi.h"
#include "RosKmdGlobal.h"
#include "RosKmdDevice.h"
#include "RosKmdContext.h"
#include "RosKmdResource.h"
#include "RosKmdUtil.h"
#include "RosKmdAcpi.h"
#include "RosKmdLogging.h"

#include "Vc4Hw.h"
#include "Vc4Ddi.h"
#include "Vc4Hvs.h"
#include "Vc4Mailbox.h"
#include "Vc4PixelValve.h"
#include "Vc4Display.h"
#include "Vc4Debug.h"
