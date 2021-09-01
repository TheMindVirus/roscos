#pragma once

//
//    Render Only Sample (Ros) Kernel Mode Driver (Kmd) Header
//
//    Copyright (C) Microsoft.  All rights reserved.
//

//Debug

#define DBG   1
#define debug(...)   \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)); \
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n"));

#pragma warning(disable:4201) //Nameless struct/union
#pragma warning(disable:4065) //Switch contains default but no case labels

extern "C"
{
#include <ntddk.h>
#include <ntverp.h>
#include <ntstrsafe.h>
#include <guiddef.h>
#include <initguid.h>
#include <windef.h>
#include <wingdi.h>
#include <acpiioct.h>
#include <dispmprt.h>
#include <d3dkmddi.h>
#include <d3d10umddi.h>
#pragma comment(lib, "displib.lib")
}

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

#define ROSD_SEGMENT_APERTURE 1
#define ROSD_SEGMENT_VIDEO_MEMORY 2
#define ROSD_SEGMENT_APERTURE_BASE_ADDRESS  0xC0000000

#define ROSD_COMMAND_BUFFER_SIZE    PAGE_SIZE

#define ROSD_PAGING_BUFFER_SIZE     PAGE_SIZE

typedef struct _ROSPRIVATEINFO2 : public _ROSADAPTERINFO
{
    UINT             m_Dummy;
} ROSPRIVATEINFO2;

const UINT C_ROSD_DMAPRIVATEDATASIZE_KMD = 0x100;

typedef struct _ROSUMDDMAPRIVATEDATA
{
    BYTE              m_KmdPrivateData[C_ROSD_DMAPRIVATEDATASIZE_KMD];
    UINT              m_DmaPacketIndex;
    UINT              m_Dummy;
    UINT              m_DmaBufferSize;
} ROSUMDDMAPRIVATEDATA;

typedef struct _ROSDUMDMAPRIVATEDATA2 : public ROSUMDDMAPRIVATEDATA
{
    UINT  m_Dummy;
} ROSUMDDMAPRIVATEDATA2;

#define ROSD_VERSION 2

const int C_ROSD_ALLOCATION_LIST_SIZE = 64;
const int C_ROSD_PATCH_LOCATION_LIST_SIZE = 128;

const int C_ROSD_GPU_ENGINE_COUNT = 1;

//RosKmdAcpi

#define ROS_KMD_TEMP_ACPI_BUFFER_SIZE (256 - sizeof(ACPI_EVAL_OUTPUT_BUFFER))

class RosKmAdapter;

class RosKmAcpiReader
{
public:

    RosKmAcpiReader(RosKmAdapter* pRosKmAdapter, ULONG DeviceUid);
    ~RosKmAcpiReader();

    void Reset()
    {
        RtlZeroMemory(&m_InputBuffer, sizeof(m_InputBuffer));
        m_InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        // TODO:[hideyukn] no input arguments support at this point.
        // m_InputBuffer.Size = 0;
        // m_InputBuffer.ArgumentCount = 0;

        if ((m_pOutputBuffer != NULL) &&
            (m_pOutputBuffer != (ACPI_EVAL_OUTPUT_BUFFER*)&m_ScratchBuffer[0]))
        {
            ExFreePool(m_pOutputBuffer);
        }

        RtlZeroMemory(&m_ScratchBuffer[0], sizeof(m_ScratchBuffer));
        m_pOutputBuffer = (ACPI_EVAL_OUTPUT_BUFFER*)&m_ScratchBuffer[0];
        m_OutputBufferSize = sizeof(m_ScratchBuffer);
    }

    NTSTATUS Read(ULONG MethodName)
    {
        SetMethod(MethodName);
        return EvalAcpiMethod();
    }

    UNALIGNED ACPI_METHOD_ARGUMENT* GetOutputArgument()
    {
        return NULL;// &(m_pOutputBuffer->Argument[0]);
    }

    ULONG GetOutputArgumentCount()
    {
        return m_pOutputBuffer->Count;
    }

private:

    void SetMethod(ULONG MethodName)
    {
        m_InputBuffer.MethodNameAsUlong = MethodName;
    }

    NTSTATUS EvalAcpiMethod();

private:

    RosKmAdapter* m_pRosKmAdapter;
    ULONG  m_DeviceUid;

    ACPI_EVAL_INPUT_BUFFER_COMPLEX m_InputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER* m_pOutputBuffer;
    ULONG m_OutputBufferSize;

    BYTE m_ScratchBuffer[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + ROS_KMD_TEMP_ACPI_BUFFER_SIZE];
};

class RosKmAcpiArgumentParser
{
public:

    RosKmAcpiArgumentParser(RosKmAcpiReader* pReader, UNALIGNED ACPI_METHOD_ARGUMENT* pParentArgument);
    ~RosKmAcpiArgumentParser();

    void Reset()
    {
        m_CurrentArgumentIndex = 0;
        if (m_pParentArgument)
        {
            m_pCurrentArgument = (UNALIGNED ACPI_METHOD_ARGUMENT*)(&m_pParentArgument->Data[0]);
        }
        else
        {
            m_pCurrentArgument = m_pReader->GetOutputArgument();
        }
    }

    BOOLEAN IsCurrentArgumentValid()
    {
        if (m_pParentArgument)
        {
            return m_pParentArgument->DataLength > ((PBYTE)m_pCurrentArgument - (PBYTE)(&m_pParentArgument->Data[0]));
        }
        else
        {
            return m_pReader->GetOutputArgumentCount() > m_CurrentArgumentIndex;
        }
    }

    UNALIGNED ACPI_METHOD_ARGUMENT* NextArgument()
    {
        if (IsCurrentArgumentValid())
        {
            m_CurrentArgumentIndex++;
            m_pCurrentArgument = ACPI_METHOD_NEXT_ARGUMENT(m_pCurrentArgument);
            NT_ASSERT((PBYTE)m_pCurrentArgument > (PBYTE)(&m_pParentArgument->Data[0]));
            if (IsCurrentArgumentValid())
            {
                return m_pCurrentArgument;
            }
        }
        return NULL;
    }

    UNALIGNED ACPI_METHOD_ARGUMENT* GetCurrentArgument()
    {
        return m_pCurrentArgument;
    }

    UNALIGNED ACPI_METHOD_ARGUMENT* GetPackage()
    {
        if (IsCurrentArgumentValid() && (GetCurrentArgument()->Type == ACPI_METHOD_ARGUMENT_PACKAGE))
        {
            UNALIGNED ACPI_METHOD_ARGUMENT* currentArgument = GetCurrentArgument();
            NextArgument();
            return currentArgument;
        }
        return NULL;
    }

    NTSTATUS GetAnsiString(char** ppString, ULONG* pLength)
    {
        NT_ASSERT(ppString);
        NT_ASSERT(pLength);

        NTSTATUS Status = STATUS_SUCCESS;
        if (IsCurrentArgumentValid() && (GetCurrentArgument()->Type == ACPI_METHOD_ARGUMENT_STRING))
        {
            *ppString = reinterpret_cast<char*>(&m_pCurrentArgument->Data[0]);
            *pLength = m_pCurrentArgument->DataLength;
            NextArgument();
        }
        else
        {
            *ppString = NULL;
            *pLength = 0;
            Status = STATUS_ACPI_INVALID_DATA;
        }
        return Status;
    }

    NTSTATUS GetBuffer(BYTE** ppData, ULONG* pLength)
    {
        NT_ASSERT(ppData);
        NT_ASSERT(pLength);

        NTSTATUS Status = STATUS_SUCCESS;
        if (IsCurrentArgumentValid() && (GetCurrentArgument()->Type == ACPI_METHOD_ARGUMENT_BUFFER))
        {
            *ppData = reinterpret_cast<BYTE*>(&m_pCurrentArgument->Data[0]);
            *pLength = m_pCurrentArgument->DataLength;
            NextArgument();
        }
        else
        {
            *ppData = NULL;
            *pLength = 0;
            Status = STATUS_ACPI_INVALID_DATA;
        }
        return Status;
    }

    template <typename _Ty>
    NTSTATUS GetValue(_Ty* pValue)
    {
        NT_ASSERT(pValue);
        NTSTATUS Status = STATUS_SUCCESS;
        if (IsCurrentArgumentValid() && (GetCurrentArgument()->Type == ACPI_METHOD_ARGUMENT_INTEGER))
        {
            if (GetCurrentArgument()->DataLength >= 8)
            {
                *pValue = (_Ty)(*((reinterpret_cast<UNALIGNED ULONGLONG*>(&m_pCurrentArgument->Data[0]))));
            }
            else if (GetCurrentArgument()->DataLength >= 4)
            {
                *pValue = (_Ty)(*((reinterpret_cast<UNALIGNED ULONG*>(&m_pCurrentArgument->Data[0]))));
            }
            else if (GetCurrentArgument()->DataLength >= 2)
            {
                *pValue = (_Ty)(*((reinterpret_cast<UNALIGNED USHORT*>(&m_pCurrentArgument->Data[0]))));
            }
            else if (GetCurrentArgument()->DataLength >= 1)
            {
                *pValue = (_Ty)(*((reinterpret_cast<BYTE*>(&m_pCurrentArgument->Data[0]))));
            }
            else
            {
                Status = STATUS_ACPI_INVALID_DATA;
            }
            NextArgument();
        }
        return Status;
    }

private:

    RosKmAcpiReader* m_pReader;
    UNALIGNED ACPI_METHOD_ARGUMENT* m_pParentArgument;

    ULONG m_CurrentArgumentIndex;
    UNALIGNED ACPI_METHOD_ARGUMENT* m_pCurrentArgument;
};

//RosKmdGlobal

class RosKmdGlobal
{
public:

    static void DdiUnload(void);

    static void DdiControlEtwLogging(
        IN_BOOLEAN  Enable,
        IN_ULONG    Flags,
        IN_UCHAR    Level);

    static NTSTATUS DriverEntry(__in IN DRIVER_OBJECT* pDriverObject, __in IN UNICODE_STRING* pRegistryPath);

    __forceinline static bool IsRenderOnly() { return s_bRenderOnly; }

    static const size_t kMaxVideoMemorySize = 128 * 1024 * 1024;

    static DRIVER_OBJECT* s_pDriverObject;
    static size_t s_videoMemorySize;
    static void* s_pVideoMemory;
    static PHYSICAL_ADDRESS s_videoMemoryPhysicalAddress;

private:

    static bool s_bDoNotInstall;
    static bool s_bRenderOnly;

};

//RosKmdAdapter

enum RosHwLayout
{
    Linear,
    Tiled
};

enum RosHwFormat
{
    X565d,
    X8888,
    X565,
    X32,
    X16,
    X8,
    D24S8       // For depth stencil
};

struct RosAllocationExchange
{
    // Input from UMD CreateResource DDI
    D3D10DDIRESOURCE_TYPE   m_resourceDimension;

    D3D10DDI_MIPINFO        m_mip0Info;
    UINT                    m_usage;        // D3D10_DDI_RESOURCE_USAGE
    UINT                    m_bindFlags;    // D3D10_DDI_RESOURCE_BIND_FLAG
    UINT                    m_mapFlags;     // D3D10_DDI_MAP
    UINT                    m_miscFlags;    // D3D10_DDI_RESOURCE_MISC_FLAG
    DXGI_FORMAT             m_format;
    DXGI_SAMPLE_DESC        m_sampleDesc;
    UINT                    m_mipLevels;
    UINT                    m_arraySize;

    bool                    m_isPrimary;
    DXGI_DDI_PRIMARY_DESC   m_primaryDesc;

    RosHwLayout             m_hwLayout;
    UINT                    m_hwWidthPixels;
    UINT                    m_hwHeightPixels;
    UINT                    m_hwSizeBytes;
};

struct RosAllocationGroupExchange
{
    int     m_dummy;
};

typedef struct __ROSKMERRORCONDITION
{
    union
    {
        struct
        {
            UINT    m_UnSupportedPagingOp : 1;
            UINT    m_NotifyDmaBufCompletion : 1;
            UINT    m_NotifyPreemptionCompletion : 1;
            UINT    m_NotifyDmaBufFault : 1;
            UINT    m_PreparationError : 1;
            UINT    m_PagingFailure : 1;
        };

        UINT        m_Value;
    };
} ROSKMERRORCONDITION;

typedef struct _ROSDMABUFSTATE
{
    union
    {
        struct
        {
            UINT    m_bRender : 1;

            UINT    m_bPresent : 1;
            UINT    m_bPaging : 1;
            UINT    m_bSwCommandBuffer : 1;
            UINT    m_bPatched : 1;
            UINT    m_bSubmittedOnce : 1;
            UINT    m_bRun : 1;
            UINT    m_bPreempted : 1;
            UINT    m_bReset : 1;
            UINT    m_bCompleted : 1;
        };

        UINT        m_Value;
    };
} ROSDMABUFSTATE;

typedef struct _ROSDMABUFINFO
{
    PBYTE                       m_pDmaBuffer;
    LARGE_INTEGER               m_DmaBufferPhysicalAddress;
    UINT                        m_DmaBufferSize;
    ROSDMABUFSTATE              m_DmaBufState;

} ROSDMABUFINFO;

typedef struct _ROSDMABUFSUBMISSION
{
    LIST_ENTRY      m_QueueEntry;
    ROSDMABUFINFO* m_pDmaBufInfo;
    UINT            m_StartOffset;
    UINT            m_EndOffset;
    UINT            m_SubmissionFenceId;
} ROSDMABUFSUBMISSION;

typedef union _RosKmAdapterFlags
{
    struct
    {
        UINT    m_isVC4 : 1;
    };

    UINT        m_value;
} RosKmAdapterFlags;

class RosKmdDdi;

class RosKmAdapter
{
public:

    static NTSTATUS AddAdapter(
        IN_CONST_PDEVICE_OBJECT     PhysicalDeviceObject,
        OUT_PPVOID                  MiniportDeviceContext);

    NTSTATUS SubmitCommand(
        IN_CONST_PDXGKARG_SUBMITCOMMAND     pSubmitCommand);

    NTSTATUS DispatchIoRequest(
        IN_ULONG                    VidPnSourceId,
        IN_PVIDEO_REQUEST_PACKET    VideoRequestPacket);

    virtual BOOLEAN InterruptRoutine(
        IN_ULONG        MessageNumber) = NULL;

    NTSTATUS APIENTRY Patch(
        IN_CONST_PDXGKARG_PATCH     pPatch);

    NTSTATUS APIENTRY CreateAllocation(
        INOUT_PDXGKARG_CREATEALLOCATION     pCreateAllocation);

    NTSTATUS APIENTRY DestroyAllocation(
        IN_CONST_PDXGKARG_DESTROYALLOCATION     pDestroyAllocation);

    NTSTATUS QueryAdapterInfo(
        IN_CONST_PDXGKARG_QUERYADAPTERINFO      pQueryAdapterInfo);

    NTSTATUS APIENTRY DescribeAllocation(
        INOUT_PDXGKARG_DESCRIBEALLOCATION       pDescribeAllocation);

    NTSTATUS GetNodeMetadata(
        UINT                            NodeOrdinal,
        OUT_PDXGKARG_GETNODEMETADATA    pGetNodeMetadata);

    NTSTATUS APIENTRY GetStandardAllocationDriverData(
        INOUT_PDXGKARG_GETSTANDARDALLOCATIONDRIVERDATA  pGetStandardAllocationDriverData);

    NTSTATUS
        SubmitCommandVirtual(
            IN_CONST_PDXGKARG_SUBMITCOMMANDVIRTUAL  pSubmitCommandVirtual);

    NTSTATUS
        PreemptCommand(
            IN_CONST_PDXGKARG_PREEMPTCOMMAND    pPreemptCommand);

    NTSTATUS
        RestartFromTimeout();

    NTSTATUS
        CancelCommand(
            IN_CONST_PDXGKARG_CANCELCOMMAND pCancelCommand);

    NTSTATUS
        QueryCurrentFence(
            INOUT_PDXGKARG_QUERYCURRENTFENCE   pCurrentFence);


    NTSTATUS
        QueryEngineStatus(
            INOUT_PDXGKARG_QUERYENGINESTATUS    pQueryEngineStatus);

    NTSTATUS
        ResetEngine(
            INOUT_PDXGKARG_RESETENGINE  pResetEngine);


    NTSTATUS
        CollectDbgInfo(
            IN_CONST_PDXGKARG_COLLECTDBGINFO        pCollectDbgInfo);

    NTSTATUS
        CreateProcess(
            IN DXGKARG_CREATEPROCESS* pArgs);

    NTSTATUS
        DestroyProcess(
            IN HANDLE KmdProcessHandle);

    void
        SetStablePowerState(
            IN_CONST_PDXGKARG_SETSTABLEPOWERSTATE  pArgs);

    NTSTATUS
        CalibrateGpuClock(
            IN UINT32                                   NodeOrdinal,
            IN UINT32                                   EngineOrdinal,
            OUT_PDXGKARG_CALIBRATEGPUCLOCK              pClockCalibration
        );

    NTSTATUS
        Escape(
            IN_CONST_PDXGKARG_ESCAPE        pEscape);

    NTSTATUS
        ResetFromTimeout();

    NTSTATUS
        QueryInterface(
            IN_PQUERY_INTERFACE     QueryInterface);

    NTSTATUS
        QueryChildRelations(
            INOUT_PDXGK_CHILD_DESCRIPTOR    ChildRelations,
            IN_ULONG                        ChildRelationsSize);

    NTSTATUS
        QueryChildStatus(
            IN_PDXGK_CHILD_STATUS   ChildStatus,
            IN_BOOLEAN              NonDestructiveOnly);
    NTSTATUS
        QueryDeviceDescriptor(
            IN_ULONG                        ChildUid,
            INOUT_PDXGK_DEVICE_DESCRIPTOR   pDeviceDescriptor);

    NTSTATUS
        SetPowerState(
            IN_ULONG                DeviceUid,
            IN_DEVICE_POWER_STATE   DevicePowerState,
            IN_POWER_ACTION         ActionType);

    NTSTATUS
        SetPowerComponentFState(
            IN UINT              ComponentIndex,
            IN UINT              FState);

    NTSTATUS
        PowerRuntimeControlRequest(
            IN LPCGUID           PowerControlCode,
            IN OPTIONAL PVOID    InBuffer,
            IN SIZE_T            InBufferSize,
            OUT OPTIONAL PVOID   OutBuffer,
            IN SIZE_T            OutBufferSize,
            OUT OPTIONAL PSIZE_T BytesReturned);

    NTSTATUS
        NotifyAcpiEvent(
            IN_DXGK_EVENT_TYPE  EventType,
            IN_ULONG            Event,
            IN_PVOID            Argument,
            OUT_PULONG          AcpiFlags);

    void ResetDevice(void);

protected:

    RosKmAdapter(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject, OUT_PPVOID MiniportDeviceContext);
    virtual ~RosKmAdapter();

    void* operator new(size_t  size);
    void operator delete(void* ptr);

public:

    static RosKmAdapter* Cast(void* ptr)
    {
        RosKmAdapter* rosKmAdapter = static_cast<RosKmAdapter*>(ptr);

        NT_ASSERT(rosKmAdapter->m_magic == RosKmAdapter::kMagic);

        return rosKmAdapter;
    }

    DXGKRNL_INTERFACE* GetDxgkInterface()
    {
        return &m_DxgkInterface;
    }

    void QueueDmaBuffer(IN_CONST_PDXGKARG_SUBMITCOMMAND pSubmitCommand);

    bool
        ValidateDmaBuffer(
            ROSDMABUFINFO* pDmaBufInfo,
            CONST DXGK_ALLOCATIONLIST* pAllocationList,
            UINT                            allocationListSize,
            CONST D3DDDI_PATCHLOCATIONLIST* pPatchLocationList,
            UINT                            patchAllocationList);

    void
        PatchDmaBuffer(
            ROSDMABUFINFO* pDmaBufInfo,
            CONST DXGK_ALLOCATIONLIST* pAllocationList,
            UINT                            allocationListSize,
            CONST D3DDDI_PATCHLOCATIONLIST* pPatchLocationList,
            UINT                            patchAllocationList);

protected:

    friend class RosKmdDdi;

    //
    // If Start() succeeds, then Stop() must be called to clean up. For example,
    // if a subclass first calls Start() and then does controller-specific
    // stuff that fails, it needs to call Stop() before returning failure
    // to the framework.
    //
    virtual NTSTATUS Start(
        IN_PDXGK_START_INFO     DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE   DxgkInterface,
        OUT_PULONG              NumberOfVideoPresentSources,
        OUT_PULONG              NumberOfChildren);

    virtual NTSTATUS Stop();

    NTSTATUS BuildPagingBuffer(
        IN_PDXGKARG_BUILDPAGINGBUFFER   pArgs);

protected:

    virtual void ProcessRenderBuffer(ROSDMABUFSUBMISSION* pDmaBufSubmission) = 0;

private:

    static void WorkerThread(void* StartContext);
    void DoWork(void);
    void DpcRoutine(void);
    void NotifyDmaBufCompletion(ROSDMABUFSUBMISSION* pDmaBufSubmission);
    static BOOLEAN SynchronizeNotifyInterrupt(PVOID SynchronizeContext);
    BOOLEAN SynchronizeNotifyInterrupt();
    ROSDMABUFSUBMISSION* DequeueDmaBuffer(KSPIN_LOCK* pDmaBufQueueLock);
    void ProcessPagingBuffer(ROSDMABUFSUBMISSION* pDmaBufSubmission);
    static void HwDmaBufCompletionDpcRoutine(KDPC*, PVOID, PVOID, PVOID);

protected:

    static const size_t kPageSize = 4096;
    static const size_t kPageShift = 12;

    static const size_t kApertureSegmentId = 1;
    static const size_t kApertureSegmentPageCount = 1024;
    static const size_t kApertureSegmentSize = kApertureSegmentPageCount * kPageSize;

    PFN_NUMBER  m_aperturePageTable[kApertureSegmentPageCount];

    static const size_t kVideoMemorySegmentId = 2;

    UINT GetAperturePhysicalAddress(UINT apertureAddress)
    {
        UINT pageIndex = (apertureAddress - ROSD_SEGMENT_APERTURE_BASE_ADDRESS) / kPageSize;

        return ((UINT)m_aperturePageTable[pageIndex]) * kPageSize + (apertureAddress & (kPageSize - 1));
    };

protected:

    RosKmAdapterFlags           m_flags;

    //
    // Indexes of hardware resources expected by the render subsystem.
    // Hardware resources for the display subsystem follow those for the renderer.
    //
    enum _RENDERER_CM_RESOURCE_INDEX : ULONG {
        _RENDERER_CM_RESOURCE_INDEX_MEMORY,
        _RENDERER_CM_RESOURCE_INDEX_INTERRUPT,
        _RENDERER_CM_RESOURCE_COUNT,
    };

private:

    static const UINT32 kMagic = 'ADPT';

    UINT32                      m_magic;

protected:

    DEVICE_OBJECT* m_pPhysicalDevice;
    DXGKRNL_INTERFACE           m_DxgkInterface;
    DXGK_START_INFO             m_DxgkStartInfo;

    ROSKMERRORCONDITION         m_ErrorHit;

    PKTHREAD                    m_pWorkerThread;
    KEVENT                      m_workerThreadEvent;
    bool                        m_workerExit;

    // TODO[indyz]: Switch to use the m_DxgkStartInfo::RequiredDmaQueueEntry
    const static UINT           m_maxDmaBufQueueLength = 32;
    ROSDMABUFSUBMISSION         m_dmaBufSubssions[m_maxDmaBufQueueLength];

    LIST_ENTRY                  m_dmaBufSubmissionFree;
    LIST_ENTRY                  m_dmaBufQueue;
    KSPIN_LOCK                  m_dmaBufQueueLock;

    KDPC                        m_hwDmaBufCompletionDpc;
    KEVENT                      m_hwDmaBufCompletionEvent;

    DXGKARGCB_NOTIFY_INTERRUPT_DATA m_interruptData;

    DXGKARG_RESETENGINE* m_pResetEngine;

    BOOL                        m_bReadyToHandleInterrupt;

    DXGK_DEVICE_INFO            m_deviceInfo;

    BYTE                        m_deviceId[MAX_DEVICE_ID_LENGTH];
    ULONG                       m_deviceIdLength;

public:

    DEVICE_POWER_STATE          m_AdapterPowerDState;
    BOOLEAN                     m_PowerManagementStarted;
    UINT                        m_NumPowerComponents;
    DXGK_POWER_RUNTIME_COMPONENT m_PowerComponents[C_ROSD_GPU_ENGINE_COUNT];
    UINT                         m_EnginePowerFState[C_ROSD_GPU_ENGINE_COUNT];

    UINT                        m_NumNodes;
    DXGK_WDDMVERSION            m_WDDMVersion;



    NTSTATUS InitializePowerComponentInfo();
    NTSTATUS GetNumPowerComponents();
    NTSTATUS GetPowerComponentInfo(UINT ComponentIndex, DXGK_POWER_RUNTIME_COMPONENT* pPowerComponent);

    //
    // Display-only DDIs
    //
    NTSTATUS SetVidPnSourceAddress(IN_CONST_PDXGKARG_SETVIDPNSOURCEADDRESS pSetVidPnSourceAddress);
    NTSTATUS SetPalette(IN_CONST_PDXGKARG_SETPALETTE pSetPalette);
    NTSTATUS SetPointerPosition(IN_CONST_PDXGKARG_SETPOINTERPOSITION SetPointerPositionPtr);
    NTSTATUS SetPointerShape(IN_CONST_PDXGKARG_SETPOINTERSHAPE SetPointerShapePtr);
    NTSTATUS IsSupportedVidPn(INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn);
    NTSTATUS RecommendFunctionalVidPn(IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn);
    NTSTATUS EnumVidPnCofuncModality(IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality);
    NTSTATUS SetVidPnSourceVisibility(IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility);
    NTSTATUS CommitVidPn(IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn);
    NTSTATUS UpdateActiveVidPnPresentPath(IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath);
    NTSTATUS RecommendMonitorModes(IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes);
    NTSTATUS GetScanLine(INOUT_PDXGKARG_GETSCANLINE pGetScanLine);
    NTSTATUS ControlInterrupt(IN_CONST_DXGK_INTERRUPT_TYPE InterruptType, IN_BOOLEAN EnableInterrupt);
    NTSTATUS QueryVidPnHWCapability(INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps);
    NTSTATUS QueryDependentEngineGroup(INOUT_DXGKARG_QUERYDEPENDENTENGINEGROUP Args);
    NTSTATUS StopDeviceAndReleasePostDisplayOwnership(D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId, PDXGK_DISPLAY_INFORMATION DisplayInfo);
};

template<typename TypeCur, typename TypeNext>
void MoveToNextCommand(TypeCur pCurCommand, TypeNext& pNextCommand)
{
    pNextCommand = (TypeNext)(pCurCommand + 1);
}

//RosKmdAllocation

struct RosKmdAllocation : public RosAllocationExchange
{
};

struct RosKmdDeviceAllocation
{
    D3DKMT_HANDLE       m_hKMAllocation;
    RosKmdAllocation* m_pRosKmdAllocation;
};

//RosKmdContext

class RosKmDevice;

class RosKmContext
{
    //
    // RosKmdCreateContext acts as the constructor so we list it as a friend
    // RosKmdEscape also acts as an initializer
    //
    friend NTSTATUS RosKmdCreateContext(IN_CONST_HANDLE, INOUT_PDXGKARG_CREATECONTEXT);
    friend NTSTATUS RosKmdEscape(IN_CONST_HANDLE, IN_CONST_PDXGKARG_ESCAPE);

public:

    static NTSTATUS APIENTRY
        
        DdiRender(
            IN_CONST_HANDLE         hContext,
            INOUT_PDXGKARG_RENDER   pRender);

    static NTSTATUS APIENTRY
        
        DdiCreateContext(
            IN_CONST_HANDLE                 hDevice,
            INOUT_PDXGKARG_CREATECONTEXT    pCreateContext);

    static NTSTATUS APIENTRY
        
        DdiDestroyContext(
            IN_CONST_HANDLE     hContext);

protected:
    RosKmDevice* m_pDevice;
    UINT                    m_Node;
    HANDLE                  m_hRTContext;

    DXGK_CREATECONTEXTFLAGS m_Flags;

public:
    UINT GetNode() { return m_Node; }
    RosKmDevice* GetDevice() { return m_pDevice; }
    DXGK_CREATECONTEXTFLAGS GetFlags() const { return m_Flags; }

    static RosKmContext* Cast(IN_CONST_HANDLE hContext)
    {
        RosKmContext* rosKmContext = reinterpret_cast<RosKmContext*>(hContext);
        NT_ASSERT(rosKmContext->m_magic == RosKmContext::kMagic);
        return rosKmContext;
    }

    NTSTATUS APIENTRY RenderKm(INOUT_PDXGKARG_RENDER pRender);
    NTSTATUS APIENTRY Present(INOUT_PDXGKARG_PRESENT Args);

private:
    static const UINT32 kMagic = 'CTXT';
    UINT32 m_magic;
};

//RosKmdDdi

class RosKmdDdi
{
public:

    static NTSTATUS DdiAddAdapter(
        IN_CONST_PDEVICE_OBJECT     PhysicalDeviceObject,
        OUT_PPVOID                  MiniportDeviceContext);

    static NTSTATUS DdiStartAdapter(
        IN_CONST_PVOID          MiniportDeviceContext,
        IN_PDXGK_START_INFO     DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE   DxgkInterface,
        OUT_PULONG              NumberOfVideoPresentSources,
        OUT_PULONG              NumberOfChildren);

    static NTSTATUS DdiStopAdapter(
        IN_CONST_PVOID  MiniportDeviceContext);

    static NTSTATUS DdiRemoveAdapter(
        IN_CONST_PVOID  MiniportDeviceContext);

    static void DdiDpcRoutine(
        IN_CONST_PVOID  MiniportDeviceContext);

    static NTSTATUS
        DdiDispatchIoRequest(
            IN_CONST_PVOID              MiniportDeviceContext,
            IN_ULONG                    VidPnSourceId,
            IN_PVIDEO_REQUEST_PACKET    VideoRequestPacket);

    static BOOLEAN
        DdiInterruptRoutine(
            IN_CONST_PVOID  MiniportDeviceContext,
            IN_ULONG        MessageNumber);

    static NTSTATUS APIENTRY DdiBuildPagingBuffer(
        IN_CONST_HANDLE                 hAdapter,
        IN_PDXGKARG_BUILDPAGINGBUFFER   pArgs);

    static NTSTATUS APIENTRY DdiSubmitCommand(
        IN_CONST_HANDLE                     hAdapter,
        IN_CONST_PDXGKARG_SUBMITCOMMAND     pSubmitCommand);

    static NTSTATUS APIENTRY DdiPatch(
        IN_CONST_HANDLE             hAdapter,
        IN_CONST_PDXGKARG_PATCH     pPatch);

    static NTSTATUS APIENTRY DdiCreateAllocation(
        IN_CONST_HANDLE                     hAdapter,
        INOUT_PDXGKARG_CREATEALLOCATION     pCreateAllocation);

    static NTSTATUS APIENTRY DdiDestroyAllocation(
        IN_CONST_HANDLE                         hAdapter,
        IN_CONST_PDXGKARG_DESTROYALLOCATION     pDestroyAllocation);

    static NTSTATUS APIENTRY DdiQueryAdapterInfo(
        IN_CONST_HANDLE                         hAdapter,
        IN_CONST_PDXGKARG_QUERYADAPTERINFO      pQueryAdapterInfo);

    static NTSTATUS APIENTRY DdiDescribeAllocation(
        IN_CONST_HANDLE                         hAdapter,
        INOUT_PDXGKARG_DESCRIBEALLOCATION       pDescribeAllocation);

    static NTSTATUS APIENTRY DdiGetNodeMetadata(
        IN_CONST_HANDLE                 hAdapter,
        UINT                            NodeOrdinal,
        OUT_PDXGKARG_GETNODEMETADATA    pGetNodeMetadata);

    static NTSTATUS APIENTRY DdiGetStandardAllocationDriverData(
        IN_CONST_HANDLE                                 hAdapter,
        INOUT_PDXGKARG_GETSTANDARDALLOCATIONDRIVERDATA  pGetStandardAllocationDriverData);

    static NTSTATUS APIENTRY
        
        DdiSubmitCommandVirtual(
            IN_CONST_HANDLE                         hAdapter,
            IN_CONST_PDXGKARG_SUBMITCOMMANDVIRTUAL  pSubmitCommandVirtual);

    static NTSTATUS APIENTRY
        
        DdiPreemptCommand(
            IN_CONST_HANDLE                     hAdapter,
            IN_CONST_PDXGKARG_PREEMPTCOMMAND    pPreemptCommand);

    static NTSTATUS APIENTRY
        DdiRestartFromTimeout(
            IN_CONST_HANDLE     hAdapter);

    static NTSTATUS APIENTRY
        
        DdiCancelCommand(
            IN_CONST_HANDLE                 hAdapter,
            IN_CONST_PDXGKARG_CANCELCOMMAND pCancelCommand);

    static NTSTATUS APIENTRY
        
        DdiQueryCurrentFence(
            IN_CONST_HANDLE                    hAdapter,
            INOUT_PDXGKARG_QUERYCURRENTFENCE   pCurrentFence);

    static NTSTATUS APIENTRY
        
        DdiResetEngine(
            IN_CONST_HANDLE             hAdapter,
            INOUT_PDXGKARG_RESETENGINE  pResetEngine);

    static NTSTATUS APIENTRY
        
        DdiQueryEngineStatus(
            IN_CONST_HANDLE                     hAdapter,
            INOUT_PDXGKARG_QUERYENGINESTATUS    pQueryEngineStatus);

    static NTSTATUS APIENTRY
        
        DdiControlInterrupt(
            IN_CONST_HANDLE                 hAdapter,
            IN_CONST_DXGK_INTERRUPT_TYPE    InterruptType,
            IN_BOOLEAN                      EnableInterrupt);

    static NTSTATUS APIENTRY
        
        DdiCollectDbgInfo(
            IN_CONST_HANDLE                         hAdapter,
            IN_CONST_PDXGKARG_COLLECTDBGINFO        pCollectDbgInfo);

    static NTSTATUS APIENTRY
        
        DdiPresent(
            IN_CONST_HANDLE         hContext,
            INOUT_PDXGKARG_PRESENT  pPresent);

    static NTSTATUS APIENTRY
        
        DdiCreateProcess(
            IN_PVOID  pMiniportDeviceContext,
            IN DXGKARG_CREATEPROCESS* pArgs);

    static NTSTATUS APIENTRY
        
        DdiDestroyProcess(
            IN_PVOID pMiniportDeviceContext,
            IN HANDLE KmdProcessHandle);

    static void APIENTRY
        
        DdiSetStablePowerState(
            IN_CONST_HANDLE                        hAdapter,
            IN_CONST_PDXGKARG_SETSTABLEPOWERSTATE  pArgs);

    static NTSTATUS APIENTRY
        
        DdiCalibrateGpuClock(
            IN_CONST_HANDLE                             hAdapter,
            IN UINT32                                   NodeOrdinal,
            IN UINT32                                   EngineOrdinal,
            OUT_PDXGKARG_CALIBRATEGPUCLOCK              pClockCalibration
        );

    static NTSTATUS APIENTRY
        
        DdiRenderKm(
            IN_CONST_HANDLE         hContext,
            INOUT_PDXGKARG_RENDER   pRender);

    static NTSTATUS APIENTRY
        
        DdiEscape(
            IN_CONST_HANDLE                 hAdapter,
            IN_CONST_PDXGKARG_ESCAPE        pEscape);

    static NTSTATUS APIENTRY
        
        DdiResetFromTimeout(
            IN_CONST_HANDLE     hAdapter);

    static NTSTATUS
        DdiQueryInterface(
            IN_CONST_PVOID          MiniportDeviceContext,
            IN_PQUERY_INTERFACE     QueryInterface);

    static NTSTATUS
        DdiQueryChildRelations(
            IN_CONST_PVOID                  MiniportDeviceContext,
            INOUT_PDXGK_CHILD_DESCRIPTOR    ChildRelations,
            IN_ULONG                        ChildRelationsSize);

    static NTSTATUS
        DdiQueryChildStatus(
            IN_CONST_PVOID          MiniportDeviceContext,
            IN_PDXGK_CHILD_STATUS   ChildStatus,
            IN_BOOLEAN              NonDestructiveOnly);
    static NTSTATUS
        DdiQueryDeviceDescriptor(
            IN_CONST_PVOID                  MiniportDeviceContext,
            IN_ULONG                        ChildUid,
            INOUT_PDXGK_DEVICE_DESCRIPTOR   pDeviceDescriptor);

    static NTSTATUS
        DdiSetPowerState(
            IN_CONST_PVOID          MiniportDeviceContext,
            IN_ULONG                DeviceUid,
            IN_DEVICE_POWER_STATE   DevicePowerState,
            IN_POWER_ACTION         ActionType);

    static NTSTATUS APIENTRY
        DdiSetPowerComponentFState(
            IN_CONST_PVOID       MiniportDeviceContext,
            UINT              ComponentIndex,
            UINT              FState);

    static NTSTATUS APIENTRY
        DdiPowerRuntimeControlRequest(
            IN_CONST_PVOID       MiniportDeviceContext,
            IN LPCGUID           PowerControlCode,
            IN OPTIONAL PVOID    InBuffer,
            IN SIZE_T            InBufferSize,
            OUT OPTIONAL PVOID   OutBuffer,
            IN SIZE_T            OutBufferSize,
            OUT OPTIONAL PSIZE_T BytesReturned);

    static NTSTATUS
        DdiNotifyAcpiEvent(
            IN_CONST_PVOID      MiniportDeviceContext,
            IN_DXGK_EVENT_TYPE  EventType,
            IN_ULONG            Event,
            IN_PVOID            Argument,
            OUT_PULONG          AcpiFlags);

    static void
        DdiResetDevice(
            IN_CONST_PVOID  MiniportDeviceContext);


public:

    static DXGKDDI_OPENALLOCATIONINFO DdiOpenAllocation;
    static DXGKDDI_QUERYDEPENDENTENGINEGROUP DdiQueryDependentEngineGroup;

};

//
// DDIs that don't have to be registered in render only mode.
//
class RosKmdDisplayDdi
{
public:

    static DXGKDDI_SETVIDPNSOURCEADDRESS DdiSetVidPnSourceAddress;

private:
public:

    static DXGKDDI_SETPALETTE DdiSetPalette;
    static DXGKDDI_SETPOINTERPOSITION DdiSetPointerPosition;
    static DXGKDDI_SETPOINTERSHAPE DdiSetPointerShape;

    static DXGKDDI_ISSUPPORTEDVIDPN DdiIsSupportedVidPn;
    static DXGKDDI_RECOMMENDFUNCTIONALVIDPN DdiRecommendFunctionalVidPn;
    static DXGKDDI_ENUMVIDPNCOFUNCMODALITY DdiEnumVidPnCofuncModality;
    static DXGKDDI_SETVIDPNSOURCEVISIBILITY DdiSetVidPnSourceVisibility;
    static DXGKDDI_COMMITVIDPN DdiCommitVidPn;
    static DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH DdiUpdateActiveVidPnPresentPath;

    static DXGKDDI_RECOMMENDMONITORMODES DdiRecommendMonitorModes;
    static DXGKDDI_GETSCANLINE DdiGetScanLine;
    static DXGKDDI_QUERYVIDPNHWCAPABILITY DdiQueryVidPnHWCapability;
    static DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP
        DdiStopDeviceAndReleasePostDisplayOwnership;

private:

};

//RosKmdDevice

class RosKmDevice
{
public: 

    __forceinline static RosKmDevice* Cast(HANDLE hDevice)
    {
        return static_cast<RosKmDevice*>(hDevice);
    }

public:

    static NTSTATUS APIENTRY DdiCreateDevice(IN_CONST_HANDLE hAdapter, INOUT_PDXGKARG_CREATEDEVICE pCreateDevice);
    static NTSTATUS APIENTRY DdiDestroyDevice(IN_CONST_HANDLE hDevice);

    static NTSTATUS APIENTRY
        
        DdiCloseAllocation(
            IN_CONST_HANDLE                     hDevice,
            IN_CONST_PDXGKARG_CLOSEALLOCATION   pCloseAllocation);

private:

    RosKmDevice(IN_CONST_HANDLE hAdapter, INOUT_PDXGKARG_CREATEDEVICE pCreateDevice);
    ~RosKmDevice();

    DXGK_CREATEDEVICEFLAGS  m_Flags;
    HANDLE                  m_hRTDevice;

public:

    RosKmAdapter* m_pRosKmAdapter;
    NTSTATUS OpenAllocation(IN_CONST_PDXGKARG_OPENALLOCATION);
};

//RosKmdUtil

#define ROS_ASSERT_MAX_IRQL(Irql) NT_ASSERT(KeGetCurrentIrql() <= (Irql))
#define ROS_ASSERT_LOW_IRQL() ROS_ASSERT_MAX_IRQL(APC_LEVEL)

//
// Pool allocation tags for use by RosKmd
//
enum ROS_ALLOC_TAG : ULONG {
    TEMP = '04CV', // will be freed in the same routine
    GLOBAL = '14CV',
    DEVICE = '24CV',
}; // enum ROS_ALLOC_TAG

_When_(Status < 0, _Post_satisfies_(return < 0)) _When_(Status >= 0, _Post_satisfies_(return >= 0))
    __forceinline NTSTATUS RosSanitizeNtstatus(
        NTSTATUS Status,
        NTSTATUS PassThrough1 = STATUS_SUCCESS,
        NTSTATUS PassThrough2 = STATUS_SUCCESS,
        NTSTATUS PassThrough3 = STATUS_SUCCESS
    )
{
    if ((Status == PassThrough1) ||
        (Status == PassThrough2) ||
        (Status == PassThrough3) ||
        (Status == STATUS_INSUFFICIENT_RESOURCES) ||
        (Status == STATUS_NO_MEMORY)) {

        return Status;
    } // if

    return NT_SUCCESS(Status) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
} // RosSanitizeNtstatus (...)

//
// Default memory allocation and object construction for C++ modules
//

void* __cdecl operator new (
    size_t Size,
    POOL_TYPE PoolType,
    ROS_ALLOC_TAG Tag
    ) throw ();

void __cdecl operator delete (void* Ptr) throw ();

void __cdecl operator delete (void* Ptr, size_t) throw ();

void* __cdecl operator new[](
    size_t Size,
    POOL_TYPE PoolType,
    ROS_ALLOC_TAG Tag
    ) throw ();

void __cdecl operator delete[](void* Ptr) throw ();

void* __cdecl operator new (size_t, void* Ptr) throw ();

void __cdecl operator delete (void*, void*) throw ();

void* __cdecl operator new[](size_t, void* Ptr) throw ();

void __cdecl operator delete[](void*, void*) throw ();

//
// class ROS_FINALLY
//
class ROS_FINALLY {
private:

    ROS_FINALLY() = delete;

    template < typename T_OP > class _FINALIZER {
        friend class ROS_FINALLY;
        T_OP op;
        __forceinline _FINALIZER(T_OP Op) throw () : op(Op) {}
    public:
        __forceinline ~_FINALIZER() { (void)this->op(); }
    }; // class _FINALIZER

    template < typename T_OP > class _FINALIZER_EX {
        friend class ROS_FINALLY;
        bool doNot;
        T_OP op;
        __forceinline _FINALIZER_EX(T_OP Op, bool DoNot) throw () : doNot(DoNot), op(Op) {}
    public:
        __forceinline ~_FINALIZER_EX() { if (!this->doNot) (void)this->op(); }
        __forceinline void DoNot(bool DoNot = true) throw () { this->doNot = DoNot; }
        __forceinline void DoNow() throw () { this->DoNot(); (void)this->op(); }
    }; // class _FINALIZER_EX

public:

    template < typename T_OP > __forceinline static _FINALIZER<T_OP> Do(T_OP Op) throw ()
    {
        return _FINALIZER<T_OP>(Op);
    }; // Do<...> (...)

    template < typename T_OP > __forceinline static _FINALIZER_EX<T_OP> DoUnless(
        T_OP Op,
        bool DoNot = false
    ) throw ()
    {
        return _FINALIZER_EX<T_OP>(Op, DoNot);
    }; // DoUnless<...> (...)
}; // class ROS_FINALLY


NTSTATUS RosOpenDevice(
    _In_ UNICODE_STRING * FileNamePtr,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess,
    _Out_ FILE_OBJECT * *FileObjectPPtr,
    ROS_ALLOC_TAG AllocTag = ROS_ALLOC_TAG::DEVICE
);


NTSTATUS RosSendWriteSynchronously(
    _In_ FILE_OBJECT * FileObjectPtr,
    _In_reads_bytes_opt_(InputBufferLength) void* InputBufferPtr,
    ULONG InputBufferLength,
    _Out_ ULONG_PTR * InformationPtr
);


NTSTATUS RosSendIoctlSynchronously(
    _In_ FILE_OBJECT * FileObjectPtr,
    ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) void* InputBufferPtr,
    ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) void* OutputBufferPtr,
    ULONG OutputBufferLength,
    BOOLEAN InternalDeviceIoControl,
    _Out_ ULONG_PTR * InformationPtr
);


DXGI_FORMAT DxgiFormatFromD3dDdiFormat(D3DDDIFORMAT Format);


D3DDDIFORMAT
TranslateDxgiFormat(DXGI_FORMAT dxgiFormat);





//RosKmdSoftAdapter (CRUCIAL)

class RosKmdSoftAdapter : public RosKmAdapter
{
private:

    friend class RosKmAdapter;

    RosKmdSoftAdapter(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject, OUT_PPVOID MiniportDeviceContext) :
        RosKmAdapter(PhysicalDeviceObject, MiniportDeviceContext)
    {
        // do nothing
    }

    virtual ~RosKmdSoftAdapter()
    {
        // do nothing
    }

    void* operator new(size_t  size);
    void operator delete(void* ptr);

protected:

    virtual void ProcessRenderBuffer(ROSDMABUFSUBMISSION* pDmaBufSubmission);

    virtual NTSTATUS Start(
        IN_PDXGK_START_INFO     DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE   DxgkInterface,
        OUT_PULONG              NumberOfVideoPresentSources,
        OUT_PULONG              NumberOfChildren);

    virtual BOOLEAN InterruptRoutine(
        IN_ULONG        MessageNumber);
};