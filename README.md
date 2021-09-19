# roscos
Visual Studio 2019 Rebased Source Directories of Windows Render-Only-Sample Driver (ROS)

## List of Changes to Ros VS2019 Solution
```
* Linker -> Input -> Ignore Specific Default Libraries = %(IgnoreSpecificDefaultLibraries);kernel32.lib;user32.lib;libcmt.lib

* Linker -> Advanced -> Entry Point = OpenAdapter10_2

* Comment out:
    //CInstructionInfo() { m_Name = nullptr; }
    //~CInstructionInfo() {}

* Include Process for RosUmd.h:

    #pragma once

    #define DBG   1
    #define debug(...)   \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)); \
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"));
    //#define debug(...) 

    #include <windows.h>
    #include <strsafe.h>
    #include <debugapi.h>

    #ifndef PAGE_SIZE
    #define PAGE_SIZE 0x1000
    #endif
    #ifndef ROUND_TO_PAGES
    #define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
    #endif
    #include <windef.h>
    #include <wingdi.h>
    typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
    #pragma warning(push)
    #pragma warning(disable : 4201)
    #include <d3d10umddi.h>
    #include <d3d11.h>
    #pragma warning(pop)

    //#include <ntassert.h>

    #include <exception>
    #include <typeinfo>
    #include <new>

    #include <stdio.h>
    #include <math.h>
    #include <intrin.h>

    #include <wdf.h>

    #pragma comment(lib, "msvcrt.lib")

* Random Stuff Excluded when ignoring libcmt.lib:

    extern "C"
    {
        void _is_c_termination_complete() {}
        void __acrt_initialize() {}
        void __acrt_uninitialize() {}
        void __acrt_thread_attach() {}
        void __acrt_thread_detach() {}
        void __acrt_uninitialize_critical() {}
        void __vcrt_initialize() {}
        void __vcrt_uninitialize() {}
        void __vcrt_uninitialize_critical() {}
        void __vcrt_thread_attach() {}
        void __vcrt_thread_detach() {}
        void __std_terminate() {}
        void __current_exception() {}
        void __current_exception_context() {}
        void __CxxFrameHandler3() {}
        //void __C_specific_handler(int) {}
        EXCEPTION_DISPOSITION __C_specific_handler
        (
            _In_    struct _EXCEPTION_RECORD*,
            _In_    void*,
            _Inout_ struct _CONTEXT*,
            _Inout_ struct _DISPATCHER_CONTEXT*
        ) { return EXCEPTION_DISPOSITION(); }
    }

* OpenAdapter10_2 Definition:

    HRESULT APIENTRY OpenAdapter10_2(D3D10DDIARG_OPENADAPTER* pArgs)
    {
        debug("[CALL]: HRESULT APIENTRY OpenAdapter10_2 (pArgs = %p)", pArgs);
        DebugBreak();
        return S_OK;
        UNREFERENCED_PARAMETER(pArgs);
    }

* WinDbg output for RosUmd5 with modified entrypoint:

    Microsoft (R) Windows Debugger Version 10.0.22415.1003 AMD64
    Copyright (c) Microsoft Corporation. All rights reserved.

    Opened \\.\com5
    Waiting to reconnect...
    Connected to Windows 10 21337 ARM 64-bit (AArch64) target at (Sun Sep 19 13:24:23.678 2021 (UTC + 1:00)), ptr64 TRUE
    Kernel Debugger connection established.
    Symbol search path is: srv*
    Executable search path is: 
    Windows 10 Kernel Version 21337 MP (4 procs) Free ARM 64-bit (AArch64)
    Product: WinNt, suite: TerminalServer SingleUserTS
    Edition build lab: 21337.1000.arm64fre.rs_prerelease.210312-1502
    Machine Name:
    Kernel base = 0xfffff802`8c200000 PsLoadedModuleList = 0xfffff802`8ce2a860
    Debug session time: Sun Sep 19 13:24:20.598 2021 (UTC + 1:00)
    System Uptime: 0 days 0:03:15.337
    Break instruction exception - code 80000003 (first chance)
    *******************************************************************************
    *                                                                             *
    *   You are seeing this message because you pressed either                    *
    *       CTRL+C (if you run console kernel debugger) or,                       *
    *       CTRL+BREAK (if you run GUI kernel debugger),                          *
    *   on your debugger machine's keyboard.                                      *
    *                                                                             *
    *                   THIS IS NOT A BUG OR A SYSTEM CRASH                       *
    *                                                                             *
    * If you did not intend to break into the debugger, press the "g" key, then   *
    * press the "Enter" key now.  This message might immediately reappear.  If it *
    * does, press "g" and "Enter" again.                                          *
    *                                                                             *
    *******************************************************************************
    nt!DbgBreakPointWithStatus:
    fffff802`8c407570 d43e0000 brk         #0xF000
    1: kd> g
    [CALL]: RosKmdGlobal::DriverEntry
    Initializing roskmd. (pDriverObject=0xFFFFE78BD2C57E30, pRegistryPath=\REGISTRY\MACHINE\SYSTEM\ControlSet001\Services\RenderOnlySample)
    [CALL]: RosKmdDdi::DdiAddAdapter
    [CALL]: RosKmAdapter::AddAdapter
    [CALL]: RosKmdSoftAdapter::operator new
    Successfully constructed display subsystem. (this=0xFFFFE78BD2C6E898, PhysicalDeviceObjectPtr=0xFFFFE78BD64367A0)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=2564AA4F-DDDB-4495-B4976AD4A84163D7)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=693A2CB1-8C8D-4AB6-95554B85EF2C7C6B)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=7F098726-2EBB-4FF3-A27F1046B95DC517)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=E929EEA4-B9F1-407B-AAB9AB08BB44FBF4)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=BF4672DE-6B4E-4BE4-A32568A91EA49C09)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=AF03F190-22AF-48CB-94BBB78E76A25107)
    [CALL]: RosKmdDdi::DdiQueryAdapterInfo
    [CALL]: RosKmAdapter::QueryAdapterInfo
    QueryAdapterInfo was called. (Type=29)
    Unsupported query type. (pQueryAdapterInfo->Type=29, pQueryAdapterInfo=0xFFFFAD8566F4C028)
    [CALL]: RosKmdDdi::DdiStartAdapter
    [CALL]: RosKmdSoftAdapter::Start
    [CALL]: RosKmAdapter::Start
    Adapter was successfully started.
    [CALL]: RosKmdDdi::DdiQueryAdapterInfo
    [CALL]: RosKmAdapter::QueryAdapterInfo
    QueryAdapterInfo was called. (Type=1)
    [CALL]: RosKmdDdi::DdiQueryAdapterInfo
    [CALL]: RosKmAdapter::QueryAdapterInfo
    QueryAdapterInfo was called. (Type=16)
    Unsupported query type. (pQueryAdapterInfo->Type=16, pQueryAdapterInfo=0xFFFFAD85677CB678)
    [CALL]: RosKmdDdi::DdiGetNodeMetadata
    [CALL]: RosKmAdapter::GetNodeMetadata
    RosKmContext::DdiCreateContext hDevice=d53d3d50

    [CALL]: RosKmdDdi::DdiQueryAdapterInfo
    [CALL]: RosKmAdapter::QueryAdapterInfo
    QueryAdapterInfo was called. (Type=5)
    [CALL]: RosKmdDdi::DdiQueryAdapterInfo
    [CALL]: RosKmAdapter::QueryAdapterInfo
    QueryAdapterInfo was called. (Type=5)
    [CALL]: RosKmdDdi::DdiBuildPagingBuffer
    [CALL]: RosKmdDdi::DdiBuildPagingBuffer
    [CALL]: RosKmdDdi::DdiQueryAdapterInfo
    [CALL]: RosKmAdapter::QueryAdapterInfo
    QueryAdapterInfo was called. (Type=10)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=14F9DB8B-85E1-4AA5-8DAFFF4A7806D5E9)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=462BC153-40EB-484A-81689972E3CD5AEF)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=197A4A6E-0391-4322-96EAC2760F881D3A)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=148A3C98-0ECD-465A-B634B05F195F7739)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=FDE5BBA4-B3F9-46FB-BDAA0728CE3100B4)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=2D09818E-DFEB-4173-B5E9AEFD66B202F3)
    [CALL]: RosKmdDdi::DdiQueryInterface
    [CALL]: RosKmAdapter::QueryInterface
    Received QueryInterface for unsupported interface. (InterfaceType=962639F3-E9DC-42AB-94EB06516DECA126)
    [CALL]: HRESULT APIENTRY OpenAdapter10_2 (pArgs = 00007FFA20200000)
    Break instruction exception - code 80000003 (first chance)
    00007ffa`28ebc9d0 d43e0000 brk         #0xF000
    3: kd> kv
     # Child-SP          RetAddr               : Args to Child                                                           : Call Site
    00 0000000e`ed07df40 00007ffa`202010a4     : 0000000e`ed07df60 00007ffa`2cc7c2dc 00007ffa`20200000 00007ffa`2ccd8b1c : 0x00007ffa`28ebc9d0
    01 0000000e`ed07df40 00000000`00000000     : 0000000e`ed07df60 00007ffa`2cc7c2dc 00007ffa`20200000 00007ffa`2ccd8b1c : 0x00007ffa`202010a4
    3: kd> kc
     # Call Site
    00 0x0
    01 0x0

* Conclusion - While D3DRuntime (d3d11.dll) is meant to call this, setting it as the entry point may yield some useful information.

    [CALL]: HRESULT APIENTRY OpenAdapter10_2 (pArgs = 00007FFA20200000)
    Break instruction exception - code 80000003 (first chance)
    00007ffa`28ebc9d0 d43e0000 brk         #0xF000
    [INFO]: _In_  pArgs->hRTAdapter        = 0000000300905A4D
    [INFO]: _Out_ pArgs->hAdapter          = 0000FFFF00000004
    [INFO]: _In_  pArgs->Interface         = 184
    [INFO]: _In_  pArgs->Version           = 0
    [INFO]: _In_  pArgs->pAdapterCallbacks = 0000000000000040
    [INFO]: _Out_ pArgs->pAdapterFuncs     = 0000000000000000
    [INFO]: _Out_ pArgs->pAdapterFuncs_2   = 0000000000000000
```