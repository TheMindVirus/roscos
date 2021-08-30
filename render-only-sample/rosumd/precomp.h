#ifndef _ROSUMD_PRECOMP_H_
#define _ROSUMD_PRECOMP_H_

#include <Windows.h>
#include <strsafe.h>
#include <debugapi.h>
#include "d3dumddi_.h"
//#include <ntassert.h>

#include <exception>
#include <typeinfo>
#include <new>

#include <stdio.h>
#include <math.h>
#include <intrin.h>

#pragma warning(disable : 4324)
#include <wdf.h>
#pragma comment(lib, "NtosKrnl.lib")

#define DBG 1
#define debug(...)   \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)); \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"));

#endif // _ROSUMD_PRECOMP_H_
