#pragma once

#include <stdio.h>
#include <Windows.h>
#include <wdf.h>

#define PVOID SomethingElse
//#include <d3d.h>
//#include <d3dumddi.h> //#include <d3d12umddi.h>

#include <d3d10umddi.h>
#include <d3d12umddi.h>
#include <d3d9types.h>
//#include <d3dcaps.h>
#include <d3dhal.h>
#include <d3dkmddi.h>
#include <d3dkmdt.h>
#include <d3dkmthk.h>
#include <d3dukmdt.h>
#include <d3dumddi.h>
//#include <dispmprt.h>
//#include <dxapi.h>
#include <dxgiddi.h>
#include <dxgitype.h>
#include <dxva.h>
//#include <iddcx.h>
#include <igpupvdev.h>
//#include <netdispumdddi.h>
//#include <ntddvdeo.h>
//#include <umdprovider.h>
//#include <video.h>
//#include <videoagp.h>

#include <guiddef.h>
#include <initguid.h>
#include <ntddvdeo.h>

#include <winddi.h>

#define DBG   1
#define debug(...)   \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)); \
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"));

extern "C"
{
    DRIVER_INITIALIZE                             DriverEntry;
    EVT_WDF_DRIVER_DEVICE_ADD                     DriverDeviceAdd;
    EVT_WDF_DRIVER_UNLOAD                         DriverUnload;
}

////////////////////////////////////////////////////////////////////////////////////////////
/*
#include <strsafe.h>
//#include <d3dumddi.h>
//#include <ntassert.h>

#include <typeinfo>
#include <new>

#include <stdio.h>
#include <math.h>
#include <intrin.h>
*/
//#include "pixel.h"

////////////////////////////////////////////////////////////////////////////////////////////
/*
//RosAllocation

const UINT MAX_DEVICE_ID_LENGTH = 32;

//RosAdapter

//
// Structure for exchange adapter info between UMD and KMD
//
typedef struct _ROSADAPTERINFO
{
    UINT                m_version;
    DXGK_WDDMVERSION    m_wddmVersion;
    BOOL                m_isSoftwareDevice;
    CHAR                m_deviceId[MAX_DEVICE_ID_LENGTH];
} ROSADAPTERINFO;
*/
//#include "RosUmdAdapter.h"
//#include "RosUmdBlendState.h"
//#include "RosUmdDepthStencilState.h"
//#include "RosUmdDepthStencilView.h"
#include "RosUmdDeviceDdi.h"
//#include "RosUmdElementLayout.h"
//#include "RosUmdRasterizerState.h"
//#include "RosUmdRenderTargetView.h"
//#include "RosUmdSampler.h"
