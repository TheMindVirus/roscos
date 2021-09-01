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
