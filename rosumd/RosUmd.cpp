#include "RosUmd.h"

HINSTANCE g_hDLL;

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
            debug("RosUmd was loaded. (hmod = 0x%p)", hmod);
            //InitializeShaderCompilerLibrary();
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
    debug("[CALL]: HRESULT APIENTRY OpenAdapter10_2");
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

    catch (RosUmdException & e)
    {
        pAdapter->Close();
        delete pAdapter;
        return e.m_hr;
    }
    */
    return S_OK;
    UNREFERENCED_PARAMETER(pArgs);
}

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