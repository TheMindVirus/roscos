#include "RosKmd.h"

DRIVER_OBJECT* RosKmdGlobal::s_pDriverObject;
bool RosKmdGlobal::s_bDoNotInstall = false;
size_t RosKmdGlobal::s_videoMemorySize = 0;
void * RosKmdGlobal::s_pVideoMemory = NULL;
PHYSICAL_ADDRESS RosKmdGlobal::s_videoMemoryPhysicalAddress;
bool RosKmdGlobal::s_bRenderOnly;

void
RosKmdGlobal::DdiUnload(
    void)
{
    debug("%s\n", __FUNCTION__);

    if ((s_pVideoMemory != NULL) && (s_pVideoMemory != (PVOID)0xCDCDCDCDCDCDCDCD))
    {
        MmFreeContiguousMemory(s_pVideoMemory);
        s_pVideoMemory = NULL;
        s_videoMemorySize = 0;
    }

    if ((s_pDriverObject != NULL) && (s_pDriverObject != (PVOID)0xCDCDCDCDCDCDCDCD))
    {
        s_pDriverObject = nullptr;
    }
}

void
RosKmdGlobal::DdiControlEtwLogging(
    IN_BOOLEAN  Enable,
    IN_ULONG    Flags,
    IN_UCHAR    Level)
{
    debug("%s Enable=%d Flags=%lx Level=%lx\n",
        __FUNCTION__, Enable, (ULONG)Flags, (ULONG)Level);
}

extern "C" __control_entrypoint(DeviceDriver)
    NTSTATUS DriverEntry(__in IN DRIVER_OBJECT* pDriverObject, __in IN UNICODE_STRING* pRegistryPath)
{
    return RosKmdGlobal::DriverEntry(pDriverObject, pRegistryPath);
}

NTSTATUS RosKmdGlobal::DriverEntry(__in IN DRIVER_OBJECT* pDriverObject, __in IN UNICODE_STRING* pRegistryPath)
{
    NTSTATUS    Status;
    DRIVER_INITIALIZATION_DATA DriverInitializationData;

    NT_ASSERT(!RosKmdGlobal::s_pDriverObject);
    RosKmdGlobal::s_pDriverObject = pDriverObject;

    debug(
        "Initializing roskmd. (pDriverObject=0x%p, pRegistryPath=%wZ)",
        pDriverObject,
        pRegistryPath);

    if (s_bDoNotInstall)
    {
        debug("s_bDoNotInstall is set; aborting driver initialization.");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate physically contiguous memory to represent cpu visible video memory
    //

    PHYSICAL_ADDRESS lowestAcceptableAddress;
    lowestAcceptableAddress.QuadPart = 0;

    PHYSICAL_ADDRESS highestAcceptableAddress;
    highestAcceptableAddress.QuadPart = -1;

    PHYSICAL_ADDRESS boundaryAddressMultiple;
    boundaryAddressMultiple.QuadPart = 0;

    s_videoMemorySize = kMaxVideoMemorySize;

    while (s_pVideoMemory == NULL)
    {
        //
        // The allocated contiguous memory is mapped as cached
        //
        // TODO[indyz]: Evaluate if flushing CPU cache for GPU access is the best option
        //
        // Use this option because GenerateRenderControlList has data alignment issue
        //

        s_pVideoMemory = MmAllocateContiguousMemorySpecifyCache(
                            s_videoMemorySize,
                            lowestAcceptableAddress,
                            highestAcceptableAddress,
                            boundaryAddressMultiple,
                            MmCached);
        if (s_pVideoMemory != NULL)
        {
            break;
        }

        s_videoMemorySize >>= 1;
    }

    s_videoMemoryPhysicalAddress = MmGetPhysicalAddress(s_pVideoMemory);
    s_bRenderOnly = true; //CRUCIAL

    //
    // Fill in the DriverInitializationData structure and call DlInitialize()
    //

    memset(&DriverInitializationData, 0, sizeof(DriverInitializationData));

    DriverInitializationData.Version = DXGKDDI_INTERFACE_VERSION;

    DriverInitializationData.DxgkDdiAddDevice = RosKmdDdi::DdiAddAdapter;
    DriverInitializationData.DxgkDdiStartDevice = RosKmdDdi::DdiStartAdapter;
    DriverInitializationData.DxgkDdiStopDevice = RosKmdDdi::DdiStopAdapter;
    DriverInitializationData.DxgkDdiRemoveDevice = RosKmdDdi::DdiRemoveAdapter;

    DriverInitializationData.DxgkDdiDispatchIoRequest = RosKmdDdi::DdiDispatchIoRequest;
    DriverInitializationData.DxgkDdiInterruptRoutine = RosKmdDdi::DdiInterruptRoutine;
    DriverInitializationData.DxgkDdiDpcRoutine = RosKmdDdi::DdiDpcRoutine;

    DriverInitializationData.DxgkDdiQueryChildRelations = RosKmdDdi::DdiQueryChildRelations;
    DriverInitializationData.DxgkDdiQueryChildStatus = RosKmdDdi::DdiQueryChildStatus;
    DriverInitializationData.DxgkDdiQueryDeviceDescriptor = RosKmdDdi::DdiQueryDeviceDescriptor;
    DriverInitializationData.DxgkDdiSetPowerState = RosKmdDdi::DdiSetPowerState;
    DriverInitializationData.DxgkDdiNotifyAcpiEvent = RosKmdDdi::DdiNotifyAcpiEvent;
    DriverInitializationData.DxgkDdiResetDevice = RosKmdDdi::DdiResetDevice;
    DriverInitializationData.DxgkDdiUnload = RosKmdGlobal::DdiUnload;
    DriverInitializationData.DxgkDdiQueryInterface = RosKmdDdi::DdiQueryInterface;
    DriverInitializationData.DxgkDdiControlEtwLogging = RosKmdGlobal::DdiControlEtwLogging;

    DriverInitializationData.DxgkDdiQueryAdapterInfo = RosKmdDdi::DdiQueryAdapterInfo;

    DriverInitializationData.DxgkDdiCreateDevice = RosKmDevice::DdiCreateDevice;

    DriverInitializationData.DxgkDdiCreateAllocation = RosKmdDdi::DdiCreateAllocation;
    DriverInitializationData.DxgkDdiDestroyAllocation = RosKmdDdi::DdiDestroyAllocation;

    DriverInitializationData.DxgkDdiDescribeAllocation = RosKmdDdi::DdiDescribeAllocation;
    DriverInitializationData.DxgkDdiGetStandardAllocationDriverData = RosKmdDdi::DdiGetStandardAllocationDriverData;

    // DriverInitializationData.DxgkDdiAcquireSwizzlingRange   = RosKmdAcquireSwizzlingRange;
    // DriverInitializationData.DxgkDdiReleaseSwizzlingRange   = RosKmdReleaseSwizzlingRange;

    DriverInitializationData.DxgkDdiOpenAllocation = RosKmdDdi::DdiOpenAllocation;
    DriverInitializationData.DxgkDdiCloseAllocation = RosKmDevice::DdiCloseAllocation;

    DriverInitializationData.DxgkDdiPatch = RosKmdDdi::DdiPatch;
    DriverInitializationData.DxgkDdiSubmitCommand = RosKmdDdi::DdiSubmitCommand;
    DriverInitializationData.DxgkDdiBuildPagingBuffer = RosKmdDdi::DdiBuildPagingBuffer;
    DriverInitializationData.DxgkDdiPreemptCommand = RosKmdDdi::DdiPreemptCommand;

    DriverInitializationData.DxgkDdiDestroyDevice = RosKmDevice::DdiDestroyDevice;

    DriverInitializationData.DxgkDdiRender = RosKmContext::DdiRender;
    DriverInitializationData.DxgkDdiPresent = RosKmdDdi::DdiPresent;
    DriverInitializationData.DxgkDdiResetFromTimeout = RosKmdDdi::DdiResetFromTimeout;
    DriverInitializationData.DxgkDdiRestartFromTimeout = RosKmdDdi::DdiRestartFromTimeout;
    DriverInitializationData.DxgkDdiEscape = RosKmdDdi::DdiEscape;
    DriverInitializationData.DxgkDdiCollectDbgInfo = RosKmdDdi::DdiCollectDbgInfo;
    DriverInitializationData.DxgkDdiQueryCurrentFence = RosKmdDdi::DdiQueryCurrentFence;
    DriverInitializationData.DxgkDdiControlInterrupt = RosKmdDdi::DdiControlInterrupt;

    DriverInitializationData.DxgkDdiCreateContext = RosKmContext::DdiCreateContext;
    DriverInitializationData.DxgkDdiDestroyContext = RosKmContext::DdiDestroyContext;

    DriverInitializationData.DxgkDdiRenderKm = RosKmdDdi::DdiRenderKm;

    //
    // Fill in DDI routines for resetting individual engine
    //
    DriverInitializationData.DxgkDdiQueryDependentEngineGroup = RosKmdDdi::DdiQueryDependentEngineGroup;
    DriverInitializationData.DxgkDdiQueryEngineStatus = RosKmdDdi::DdiQueryEngineStatus;
    DriverInitializationData.DxgkDdiResetEngine = RosKmdDdi::DdiResetEngine;

    //
    // Fill in DDI for canceling DMA buffers
    //
    DriverInitializationData.DxgkDdiCancelCommand = RosKmdDdi::DdiCancelCommand;

    //
    // Fill in DDI for component power management
    //
    DriverInitializationData.DxgkDdiSetPowerComponentFState = RosKmdDdi::DdiSetPowerComponentFState;
    DriverInitializationData.DxgkDdiPowerRuntimeControlRequest = RosKmdDdi::DdiPowerRuntimeControlRequest;

    DriverInitializationData.DxgkDdiGetNodeMetadata = RosKmdDdi::DdiGetNodeMetadata;

    DriverInitializationData.DxgkDdiSubmitCommandVirtual = RosKmdDdi::DdiSubmitCommandVirtual;

    DriverInitializationData.DxgkDdiCreateProcess = RosKmdDdi::DdiCreateProcess;
    DriverInitializationData.DxgkDdiDestroyProcess = RosKmdDdi::DdiDestroyProcess;

    DriverInitializationData.DxgkDdiCalibrateGpuClock = RosKmdDdi::DdiCalibrateGpuClock;
    DriverInitializationData.DxgkDdiSetStablePowerState = RosKmdDdi::DdiSetStablePowerState;

    //
    // Register display subsystem DDIS.
    // Refer to adapterdisplay.cxx:ADAPTER_DISPLAY::CreateDisplayCore() for
    // required DDIs.
    //
    if (!IsRenderOnly())
    {
        DriverInitializationData.DxgkDdiSetPalette = RosKmdDisplayDdi::DdiSetPalette;
        DriverInitializationData.DxgkDdiSetPointerPosition = RosKmdDisplayDdi::DdiSetPointerPosition;
        DriverInitializationData.DxgkDdiSetPointerShape = RosKmdDisplayDdi::DdiSetPointerShape;
    
        DriverInitializationData.DxgkDdiIsSupportedVidPn = RosKmdDisplayDdi::DdiIsSupportedVidPn;
        DriverInitializationData.DxgkDdiRecommendFunctionalVidPn = RosKmdDisplayDdi::DdiRecommendFunctionalVidPn;
        DriverInitializationData.DxgkDdiEnumVidPnCofuncModality = RosKmdDisplayDdi::DdiEnumVidPnCofuncModality;
        DriverInitializationData.DxgkDdiSetVidPnSourceAddress = RosKmdDisplayDdi::DdiSetVidPnSourceAddress;
        DriverInitializationData.DxgkDdiSetVidPnSourceVisibility = RosKmdDisplayDdi::DdiSetVidPnSourceVisibility;
        DriverInitializationData.DxgkDdiCommitVidPn = RosKmdDisplayDdi::DdiCommitVidPn;
        DriverInitializationData.DxgkDdiUpdateActiveVidPnPresentPath = RosKmdDisplayDdi::DdiUpdateActiveVidPnPresentPath;

        DriverInitializationData.DxgkDdiRecommendMonitorModes = RosKmdDisplayDdi::DdiRecommendMonitorModes;
        DriverInitializationData.DxgkDdiGetScanLine = RosKmdDisplayDdi::DdiGetScanLine;
        DriverInitializationData.DxgkDdiQueryVidPnHWCapability = RosKmdDisplayDdi::DdiQueryVidPnHWCapability;
        DriverInitializationData.DxgkDdiStopDeviceAndReleasePostDisplayOwnership = RosKmdDisplayDdi::DdiStopDeviceAndReleasePostDisplayOwnership;
    }

    Status = DxgkInitialize(pDriverObject, pRegistryPath, &DriverInitializationData);

    if (!NT_SUCCESS(Status))
    {
        debug("[WARN]: DxgkInitialize Failed");
        return Status;
    }
    debug("[INFO]: RosKmd DriverEntry Complete");
    return Status;
}
