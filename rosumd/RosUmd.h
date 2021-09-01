#pragma once

#define DBG   1
#define debug(...) \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)); \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"));

////////////////////////////////////////////////////////////////////////////////////////////
// 
//
// Header used to include <d3dumddi.h>
//
// This header takes care of the various dependencies needed by d3dumddi.h
//
#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#ifndef ROUND_TO_PAGES
#define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#endif

#include <windef.h>
#include <wingdi.h>

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
#pragma warning(disable : 4201)

#define D3D12_TOKENIZED_PROGRAM_FORMAT_HEADER
#include <d3d10umddi.h>
#undef D3D12_TOKENIZED_PROGRAM_FORMAT_HEADER
#include <d3d11.h>

#include <stdio.h>
#include <exception>

#define assert( _exp ) ( ( _exp ) ? true : (\
    OutputDebugStringW( L"Assertion Failed\n" ),\
    OutputDebugStringW( #_exp L"\n" ),\
    DebugBreak() ) ); __assume( _exp )

#define AssertAndAssume(expression) {assert(expression); __analysis_assume(expression);}

////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <strsafe.h>
#include <debugapi.h>
//#include <d3dumddi.h>
//#include <ntassert.h>

#include <typeinfo>
#include <new>

#include <stdio.h>
#include <math.h>
#include <intrin.h>

#include "pixel.hpp"

//#pragma warning(disable : 4324)
#include <wdf.h>
#pragma comment(lib, "NtosKrnl.lib")

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

#include "RosUmdAdapter.h"
#include "RosUmdBlendState.h"
#include "RosUmdDepthStencilState.h"
#include "RosUmdDepthStencilView.h"
#include "RosUmdDeviceDdi.h"
#include "RosUmdElementLayout.h"
#include "RosUmdRasterizerState.h"
#include "RosUmdRenderTargetView.h"
#include "RosUmdSampler.h"


//#pragma warning(disable : 4324)
//#include <wdf.h>
//pragma comment(lib, "NtosKrnl.lib")

/*
#include <stdio.h>
#include <Windows.h>
#include <wdf.h>

#undef PVOID
//#define PVOID SomethingElse
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
*/