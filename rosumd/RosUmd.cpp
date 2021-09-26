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
    debug("[CALL]: %s (pArgs = %p)", __FUNCTION__, pArgs);
    /*DebugBreak();
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
    */
    ///*
    RosUmdAdapter* pAdapter = (RosUmdAdapter*)malloc(sizeof(RosUmdAdapter));
    if( NULL == pAdapter ) { debug("[ASRT]: pAdapter == NULL"); return E_OUTOFMEMORY; }
    *pAdapter = RosUmdAdapter();

    try
    {
        pAdapter->Open(pArgs);
    }
    catch(...) //(RosUmdException & e)
    {
        debug("[WARN]: pAdapter->Open(pArgs) Failed to open Direct3D Render Adapter");
        pAdapter->Close();
        free(pAdapter); //delete pAdapter;
        return E_ABORT; //e.m_hr;
    }
    //*/
    return S_OK;
}

//Random Predefined Stuff included by MSVCRT x86 Compiler

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
    /*
    //void _CxxThrowException() {}
    void __stdcall _CxxThrowException //Cannot be referenced by Standard Definition
    (
        void*,
        void*
    ) {} //Just don't use new and delete, they've been wrecked. Use malloc() and free() instead
    */
    //void __std_exception_copy() {}
    void __std_exception_copy
    (
        _In_ __std_exception_data const* src,
        _Out_ __std_exception_data* dst
    ) { dst = (__std_exception_data*)src; }

    //void __std_exception_destroy() {}
    void __std_exception_destroy
    (
        _Inout_ __std_exception_data*
    ) {}
}