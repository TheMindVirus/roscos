#include "RosUmd.h"

HINSTANCE g_hDLL;
CInstructionInfo g_InstructionInfo[D3D10_SB_NUM_OPCODES];

BOOL WINAPI DllMain(
    HINSTANCE hmod,
    UINT dwReason,
    LPVOID lpvReserved )
{
    lpvReserved; // unused

    // Warning, do not call outside of this module, except for functions located in kernel32.dll. BUT, do not call LoadLibrary nor
    // FreeLibrary, either. Nor, call malloc nor new; use HeapAlloc directly.

    switch( dwReason )
    {
        case( DLL_PROCESS_ATTACH ):
        {
            debug("RosUmd was loaded... (hmod = 0x%p)", hmod);
            //DebugBreak();
            //InitializeShaderCompilerLibrary();
            InitInstructionInfo();
            g_hDLL = hmod;
        }
        break;
        case( DLL_PROCESS_DETACH ):
        {
            g_hDLL = NULL;
            return TRUE;
        }
        default: break;
    }
    return TRUE;
}

HRESULT APIENTRY OpenAdapter10_2(D3D10DDIARG_OPENADAPTER* pArgs)
{
    debug("[CALL]: HRESULT APIENTRY OpenAdapter10_2 (pArgs = %p)", pArgs);
    DebugBreak();
    if (pArgs)
    {
        debug("[INFO]: _In_  pArgs->hRTAdapter        = %p", pArgs->hRTAdapter.handle);
        debug("[INFO]: _Out_ pArgs->hAdapter          = %p", pArgs->hAdapter.pDrvPrivate);
        debug("[INFO]: _In_  pArgs->Interface         = %d", pArgs->Interface);
        debug("[INFO]: _In_  pArgs->Version           = %d", pArgs->Version);
        debug("[INFO]: _In_  pArgs->pAdapterCallbacks = %p", pArgs->pAdapterCallbacks);
        debug("[INFO]: _Out_ pArgs->pAdapterFuncs     = %p", pArgs->pAdapterFuncs);
        debug("[INFO]: _Out_ pArgs->pAdapterFuncs_2   = %p", pArgs->pAdapterFuncs_2);
    }
    /*
    RosUmdAdapter* pAdapter = new RosUmdAdapter;
    if( NULL == pAdapter )
    {
        return E_OUTOFMEMORY;
    }
    
    try
    {
        pAdapter->Open(pArgs);
    }

    catch(...) //(RosUmdException & e)
    {
        pAdapter->Close();
        delete pAdapter;
        return E_ABORT; //e.m_hr;
    }
    */
    return S_OK;
    UNREFERENCED_PARAMETER(pArgs);
}
/*
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    debug("[CALL]: NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)");
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG Config = WDF_DRIVER_CONFIG();
    WDFDRIVER Driver = NULL;

    WDF_DRIVER_CONFIG_INIT(&Config, DriverDeviceAdd);
    Config.EvtDriverUnload = DriverUnload;
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, &Driver);
    if (NT_ERROR(status)) { debug("[WARN]: WdfDriverCreate failed with Status (0x%08lX)", status); return status; }
    return STATUS_SUCCESS;
}

NTSTATUS DriverDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    debug("[CALL]: NTSTATUS DriverDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)");
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE Device = NULL;

    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &Device);
    if (NT_ERROR(status)) { debug("[WARN]: WdfDeviceCreate failed with Status (0x%08lX)", status); return status; }

    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(Driver);
}

VOID DriverUnload(WDFDRIVER Driver)
{
    debug("[CALL]: VOID DriverUnload(WDFDRIVER Driver)");
    UNREFERENCED_PARAMETER(Driver);
}
*/

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