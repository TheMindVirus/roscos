#include "RosKmd.h"

// TODO(bhouse) RosKmdPnpDispatch appears to be unused
NTSTATUS
RosKmdPnpDispatch(
    __in struct _DEVICE_OBJECT *DeviceObject,
    __inout struct _IRP *pIrp)
{
    DeviceObject;
    pIrp;

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
RosKmdDdi::DdiAddAdapter(
    IN_CONST_PDEVICE_OBJECT     PhysicalDeviceObject,
    OUT_PPVOID                  MiniportDeviceContext)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);
    return RosKmAdapter::AddAdapter(PhysicalDeviceObject, MiniportDeviceContext);
}

NTSTATUS
RosKmdDdi::DdiStartAdapter(
    IN_CONST_PVOID          MiniportDeviceContext,
    IN_PDXGK_START_INFO     DxgkStartInfo,
    IN_PDXGKRNL_INTERFACE   DxgkInterface,
    OUT_PULONG              NumberOfVideoPresentSources,
    OUT_PULONG              NumberOfChildren)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);
    RosKmAdapter  *pRosKmAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    return pRosKmAdapter->Start(DxgkStartInfo, DxgkInterface, NumberOfVideoPresentSources, NumberOfChildren);
}


NTSTATUS
RosKmdDdi::DdiStopAdapter(
    IN_CONST_PVOID  MiniportDeviceContext)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);
    if (MiniportDeviceContext != NULL)
    {
        RosKmAdapter* pRosKmAdapter = RosKmAdapter::Cast(MiniportDeviceContext);
        return pRosKmAdapter->Stop();
    }
    return STATUS_SUCCESS;
}


NTSTATUS
RosKmdDdi::DdiRemoveAdapter(
    IN_CONST_PVOID  MiniportDeviceContext)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);
    //RosKmAdapter  *pRosKmAdapter = RosKmAdapter::Cast(MiniportDeviceContext);
    //delete pRosKmAdapter;
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(MiniportDeviceContext);
}

void
RosKmdDdi::DdiDpcRoutine(
    IN_CONST_PVOID  MiniportDeviceContext)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);

    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    pRosKmdAdapter->DpcRoutine();
}

NTSTATUS
RosKmdDdi::DdiDispatchIoRequest(
    IN_CONST_PVOID              MiniportDeviceContext,
    IN_ULONG                    VidPnSourceId,
    IN_PVIDEO_REQUEST_PACKET    VideoRequestPacket)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);

    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);
    return pRosKmdAdapter->DispatchIoRequest(VidPnSourceId, VideoRequestPacket);
}

BOOLEAN
RosKmdDdi::DdiInterruptRoutine(
    IN_CONST_PVOID  MiniportDeviceContext,
    IN_ULONG        MessageNumber)
{
    debug("%s MiniportDeviceContext=%p\n",
        __FUNCTION__, MiniportDeviceContext);

    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    return pRosKmdAdapter->InterruptRoutine(MessageNumber);
}

NTSTATUS
RosKmdDdi::DdiBuildPagingBuffer(
    IN_CONST_HANDLE                 hAdapter,
    IN_PDXGKARG_BUILDPAGINGBUFFER   pArgs)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->BuildPagingBuffer(pArgs);
}

NTSTATUS RosKmdDdi::DdiSubmitCommand(
    IN_CONST_HANDLE                     hAdapter,
    IN_CONST_PDXGKARG_SUBMITCOMMAND     pSubmitCommand)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->SubmitCommand(pSubmitCommand);
}

NTSTATUS RosKmdDdi::DdiPatch(
    IN_CONST_HANDLE             hAdapter,
    IN_CONST_PDXGKARG_PATCH     pPatch)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->Patch(pPatch);
}

NTSTATUS APIENTRY RosKmdDdi::DdiCreateAllocation(
    IN_CONST_HANDLE                     hAdapter,
    INOUT_PDXGKARG_CREATEALLOCATION     pCreateAllocation)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->CreateAllocation(pCreateAllocation);
}


NTSTATUS APIENTRY RosKmdDdi::DdiDestroyAllocation(
    IN_CONST_HANDLE                         hAdapter,
    IN_CONST_PDXGKARG_DESTROYALLOCATION     pDestroyAllocation)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->DestroyAllocation(pDestroyAllocation);
}


NTSTATUS APIENTRY RosKmdDdi::DdiQueryAdapterInfo(
    IN_CONST_HANDLE                         hAdapter,
    IN_CONST_PDXGKARG_QUERYADAPTERINFO      pQueryAdapterInfo)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->QueryAdapterInfo(pQueryAdapterInfo);
}


NTSTATUS APIENTRY RosKmdDdi::DdiDescribeAllocation(
    IN_CONST_HANDLE                         hAdapter,
    INOUT_PDXGKARG_DESCRIBEALLOCATION       pDescribeAllocation)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->DescribeAllocation(pDescribeAllocation);
}


NTSTATUS RosKmdDdi::DdiGetNodeMetadata(
    IN_CONST_HANDLE                 hAdapter,
    UINT                            NodeOrdinal,
    OUT_PDXGKARG_GETNODEMETADATA    pGetNodeMetadata)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->GetNodeMetadata(NodeOrdinal, pGetNodeMetadata);
}


NTSTATUS APIENTRY RosKmdDdi::DdiGetStandardAllocationDriverData(
    IN_CONST_HANDLE                                 hAdapter,
    INOUT_PDXGKARG_GETSTANDARDALLOCATIONDRIVERDATA  pGetStandardAllocationDriverData)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->GetStandardAllocationDriverData(pGetStandardAllocationDriverData);
}


NTSTATUS
RosKmdDdi::DdiSubmitCommandVirtual(
    IN_CONST_HANDLE                         hAdapter,
    IN_CONST_PDXGKARG_SUBMITCOMMANDVIRTUAL  pSubmitCommandVirtual)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->SubmitCommandVirtual(pSubmitCommandVirtual);
}


NTSTATUS
RosKmdDdi::DdiPreemptCommand(
    IN_CONST_HANDLE                     hAdapter,
    IN_CONST_PDXGKARG_PREEMPTCOMMAND    pPreemptCommand)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->PreemptCommand(pPreemptCommand);
}


NTSTATUS CALLBACK
RosKmdDdi::DdiRestartFromTimeout(
    IN_CONST_HANDLE     hAdapter)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->RestartFromTimeout();
}


NTSTATUS
RosKmdDdi::DdiCancelCommand(
    IN_CONST_HANDLE                 hAdapter,
    IN_CONST_PDXGKARG_CANCELCOMMAND pCancelCommand)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->CancelCommand(pCancelCommand);
}


NTSTATUS
RosKmdDdi::DdiQueryCurrentFence(
    IN_CONST_HANDLE                    hAdapter,
    INOUT_PDXGKARG_QUERYCURRENTFENCE   pCurrentFence)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->QueryCurrentFence(pCurrentFence);
}

NTSTATUS
RosKmdDdi::DdiResetEngine(
    IN_CONST_HANDLE             hAdapter,
    INOUT_PDXGKARG_RESETENGINE  pResetEngine)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->ResetEngine(pResetEngine);
}


NTSTATUS
RosKmdDdi::DdiQueryEngineStatus(
    IN_CONST_HANDLE                     hAdapter,
    INOUT_PDXGKARG_QUERYENGINESTATUS    pQueryEngineStatus)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->QueryEngineStatus(pQueryEngineStatus);
}




NTSTATUS
RosKmdDdi::DdiControlInterrupt(
    IN_CONST_HANDLE                 hAdapter,
    IN_CONST_DXGK_INTERRUPT_TYPE    InterruptType,
    IN_BOOLEAN                      EnableInterrupt)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->ControlInterrupt(InterruptType, EnableInterrupt);
}


NTSTATUS
RosKmdDdi::DdiCollectDbgInfo(
    IN_CONST_HANDLE                         hAdapter,
    IN_CONST_PDXGKARG_COLLECTDBGINFO        pCollectDbgInfo)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->CollectDbgInfo(pCollectDbgInfo);
}


NTSTATUS
RosKmdDdi::DdiPresent(
    IN_CONST_HANDLE         hContext,
    INOUT_PDXGKARG_PRESENT  pPresent)
{
    RosKmContext  *pRosKmdContext = RosKmContext::Cast(hContext);

    debug("%s hContext=%p\n", __FUNCTION__, hContext);

    return pRosKmdContext->Present(pPresent);
}


NTSTATUS
RosKmdDdi::DdiCreateProcess(
    IN_PVOID  pMiniportDeviceContext,
    IN DXGKARG_CREATEPROCESS* pArgs)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(pMiniportDeviceContext);

    debug("%s pMiniportDeviceContext=%p\n", __FUNCTION__, pMiniportDeviceContext);

    return pRosKmdAdapter->CreateProcess(pArgs);
}


NTSTATUS

RosKmdDdi::DdiDestroyProcess(
    IN_PVOID pMiniportDeviceContext,
    IN HANDLE KmdProcessHandle)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(pMiniportDeviceContext);

    debug("%s pMiniportDeviceContext=%p\n", __FUNCTION__, pMiniportDeviceContext);

    return pRosKmdAdapter->DestroyProcess(KmdProcessHandle);
}


void

RosKmdDdi::DdiSetStablePowerState(
    IN_CONST_HANDLE                        hAdapter,
    IN_CONST_PDXGKARG_SETSTABLEPOWERSTATE  pArgs)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->SetStablePowerState(pArgs);
}


NTSTATUS

RosKmdDdi::DdiCalibrateGpuClock(
    IN_CONST_HANDLE                             hAdapter,
    IN UINT32                                   NodeOrdinal,
    IN UINT32                                   EngineOrdinal,
    OUT_PDXGKARG_CALIBRATEGPUCLOCK              pClockCalibration
    )
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->CalibrateGpuClock(NodeOrdinal, EngineOrdinal, pClockCalibration);
}


NTSTATUS

RosKmdDdi::DdiRenderKm(
    IN_CONST_HANDLE         hContext,
    INOUT_PDXGKARG_RENDER   pRender)
{
    RosKmContext  *pRosKmContext = RosKmContext::Cast(hContext);

    debug("%s hContext=%p\n", __FUNCTION__, hContext);

    return pRosKmContext->RenderKm(pRender);
}

NTSTATUS

RosKmdDdi::DdiEscape(
    IN_CONST_HANDLE                 hAdapter,
    IN_CONST_PDXGKARG_ESCAPE        pEscape)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->Escape(pEscape);
}

NTSTATUS

RosKmdDdi::DdiResetFromTimeout(
    IN_CONST_HANDLE     hAdapter)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(hAdapter);

    debug("%s hAdapter=%p\n", __FUNCTION__, hAdapter);

    return pRosKmdAdapter->ResetFromTimeout();
}


NTSTATUS
RosKmdDdi::DdiQueryInterface(
    IN_CONST_PVOID          MiniportDeviceContext,
    IN_PQUERY_INTERFACE     QueryInterface)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->QueryInterface(QueryInterface);
}


NTSTATUS
RosKmdDdi::DdiQueryChildRelations(
    IN_CONST_PVOID                  MiniportDeviceContext,
    INOUT_PDXGK_CHILD_DESCRIPTOR    ChildRelations,
    IN_ULONG                        ChildRelationsSize)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->QueryChildRelations(ChildRelations, ChildRelationsSize);
}


NTSTATUS
RosKmdDdi::DdiQueryChildStatus(
    IN_CONST_PVOID          MiniportDeviceContext,
    IN_PDXGK_CHILD_STATUS   ChildStatus,
    IN_BOOLEAN              NonDestructiveOnly)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->QueryChildStatus(ChildStatus, NonDestructiveOnly);
}

NTSTATUS
RosKmdDdi::DdiQueryDeviceDescriptor(
    IN_CONST_PVOID                  MiniportDeviceContext,
    IN_ULONG                        ChildUid,
    INOUT_PDXGK_DEVICE_DESCRIPTOR   pDeviceDescriptor)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->QueryDeviceDescriptor(ChildUid, pDeviceDescriptor);
}


NTSTATUS
RosKmdDdi::DdiSetPowerState(
    IN_CONST_PVOID          MiniportDeviceContext,
    IN_ULONG                DeviceUid,
    IN_DEVICE_POWER_STATE   DevicePowerState,
    IN_POWER_ACTION         ActionType)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->SetPowerState(DeviceUid, DevicePowerState, ActionType);
}

NTSTATUS APIENTRY
RosKmdDdi::DdiSetPowerComponentFState(
    IN_CONST_PVOID       MiniportDeviceContext,
    UINT              ComponentIndex,
    UINT              FState)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->SetPowerComponentFState(ComponentIndex, FState);
}

NTSTATUS APIENTRY
RosKmdDdi::DdiPowerRuntimeControlRequest(
    IN_CONST_PVOID       MiniportDeviceContext,
    IN LPCGUID           PowerControlCode,
    IN OPTIONAL PVOID    InBuffer,
    IN SIZE_T            InBufferSize,
    OUT OPTIONAL PVOID   OutBuffer,
    IN SIZE_T            OutBufferSize,
    OUT OPTIONAL PSIZE_T BytesReturned)
{
    RosKmAdapter  *pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->PowerRuntimeControlRequest(PowerControlCode, InBuffer, InBufferSize, OutBuffer, OutBufferSize, BytesReturned);
}


NTSTATUS
RosKmdDdi::DdiNotifyAcpiEvent(
    IN_CONST_PVOID      MiniportDeviceContext,
    IN_DXGK_EVENT_TYPE  EventType,
    IN_ULONG            Event,
    IN_PVOID            Argument,
    OUT_PULONG          AcpiFlags)
{
    RosKmAdapter* pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->NotifyAcpiEvent(EventType, Event, Argument, AcpiFlags);
}


void
RosKmdDdi::DdiResetDevice(
    IN_CONST_PVOID  MiniportDeviceContext)
{
    RosKmAdapter* pRosKmdAdapter = RosKmAdapter::Cast(MiniportDeviceContext);

    debug("%s MiniportDeviceContext=%p\n", __FUNCTION__, MiniportDeviceContext);

    return pRosKmdAdapter->ResetDevice();
}

 //================================================


NTSTATUS RosKmdDisplayDdi::DdiSetVidPnSourceAddress (
    HANDLE const hAdapter,
    const DXGKARG_SETVIDPNSOURCEADDRESS* SetVidPnSourceAddressPtr
    )
{
    return RosKmAdapter::Cast(hAdapter)->SetVidPnSourceAddress(
            SetVidPnSourceAddressPtr);
}

 //==================================================
 //===================================================
// TODO[jordanh] put PASSIVE_LEVEL DDIs in the paged section

//
// RosKmdDdi
//


NTSTATUS RosKmdDdi::DdiOpenAllocation (
    HANDLE const hDevice,
    const DXGKARG_OPENALLOCATION* ArgsPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmDevice::Cast(hDevice)->OpenAllocation(ArgsPtr);
}


NTSTATUS RosKmdDdi::DdiQueryDependentEngineGroup (
    HANDLE const hAdapter,
    DXGKARG_QUERYDEPENDENTENGINEGROUP* ArgsPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(hAdapter)->QueryDependentEngineGroup(ArgsPtr);
}

//
// RosKmdDisplayDdi
//


NTSTATUS RosKmdDisplayDdi::DdiSetPalette (
    HANDLE const AdapterPtr,
    const DXGKARG_SETPALETTE* SetPalettePtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(AdapterPtr)->SetPalette(SetPalettePtr);
}


NTSTATUS RosKmdDisplayDdi::DdiSetPointerPosition (
    HANDLE const AdapterPtr,
    const DXGKARG_SETPOINTERPOSITION* SetPointerPositionPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(AdapterPtr)->SetPointerPosition(
        SetPointerPositionPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiSetPointerShape (
    HANDLE const AdapterPtr,
    const DXGKARG_SETPOINTERSHAPE* SetPointerShapePtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(AdapterPtr)->SetPointerShape(SetPointerShapePtr);
}


NTSTATUS RosKmdDisplayDdi::DdiIsSupportedVidPn (
    VOID* const MiniportDeviceContextPtr,
    DXGKARG_ISSUPPORTEDVIDPN* IsSupportedVidPnPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->IsSupportedVidPn(
            IsSupportedVidPnPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiRecommendFunctionalVidPn (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_RECOMMENDFUNCTIONALVIDPN* const RecommendFunctionalVidPnPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->RecommendFunctionalVidPn(
            RecommendFunctionalVidPnPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiEnumVidPnCofuncModality (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_ENUMVIDPNCOFUNCMODALITY* const EnumCofuncModalityPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->EnumVidPnCofuncModality(
            EnumCofuncModalityPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiSetVidPnSourceVisibility (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_SETVIDPNSOURCEVISIBILITY* SetVidPnSourceVisibilityPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->SetVidPnSourceVisibility(
            SetVidPnSourceVisibilityPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiCommitVidPn (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_COMMITVIDPN* const CommitVidPnPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->CommitVidPn(
            CommitVidPnPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiUpdateActiveVidPnPresentPath (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* const UpdateActiveVidPnPresentPathPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->UpdateActiveVidPnPresentPath(
            UpdateActiveVidPnPresentPathPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiRecommendMonitorModes (
    VOID* const MiniportDeviceContextPtr,
    const DXGKARG_RECOMMENDMONITORMODES* const RecommendMonitorModesPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->RecommendMonitorModes(
            RecommendMonitorModesPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiGetScanLine (
    HANDLE const AdapterPtr,
    DXGKARG_GETSCANLINE*  GetScanLinePtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(AdapterPtr)->GetScanLine(GetScanLinePtr);
}


NTSTATUS RosKmdDisplayDdi::DdiQueryVidPnHWCapability (
    VOID* const MiniportDeviceContextPtr,
    DXGKARG_QUERYVIDPNHWCAPABILITY* VidPnHWCapsPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->QueryVidPnHWCapability(
            VidPnHWCapsPtr);
}


NTSTATUS RosKmdDisplayDdi::DdiStopDeviceAndReleasePostDisplayOwnership (
    VOID* const MiniportDeviceContextPtr,
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    DXGK_DISPLAY_INFORMATION* DisplayInfoPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    return RosKmAdapter::Cast(MiniportDeviceContextPtr)->StopDeviceAndReleasePostDisplayOwnership(
            TargetId,
            DisplayInfoPtr);
}

 //=====================================================
