#include "RosKmd.h"

void* RosKmAdapter::operator new(size_t size)
{
    return NULL; UNREFERENCED_PARAMETER(size); // ExAllocatePoolWithTag(NonPagedPoolNx, size, 'ROSD');
}

void RosKmAdapter::operator delete(void * ptr)
{
    UNREFERENCED_PARAMETER(ptr); //ExFreePool(ptr);
}

RosKmAdapter::RosKmAdapter(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject, OUT_PPVOID MiniportDeviceContext)// :
    //m_display(PhysicalDeviceObject, m_DxgkInterface, m_DxgkStartInfo, m_deviceInfo)
{
    m_magic = kMagic;
    m_pPhysicalDevice = PhysicalDeviceObject;

    // Enable in RosKmAdapter::Start() when device is ready for interrupt
    m_bReadyToHandleInterrupt = FALSE;

    // Set initial power management state.
    m_PowerManagementStarted = FALSE;
    m_AdapterPowerDState = PowerDeviceD0; // Device is at D0 at startup
    m_NumPowerComponents = 0;
    RtlZeroMemory(&m_EnginePowerFState[0], sizeof(m_EnginePowerFState)); // Components are F0 at startup.

    RtlZeroMemory(&m_deviceId, sizeof(m_deviceId));
    m_deviceIdLength = 0;

    m_flags.m_value = 0;

    *MiniportDeviceContext = this;
}

RosKmAdapter::~RosKmAdapter()
{
    // do nothing
}

NTSTATUS
RosKmAdapter::AddAdapter(
    IN_CONST_PDEVICE_OBJECT     PhysicalDeviceObject,
    OUT_PPVOID                  MiniportDeviceContext)
{
    NTSTATUS status;
    WCHAR deviceID[512];
    ULONG dataLen;

    status = IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyHardwareID, sizeof(deviceID), deviceID, &dataLen);
    if (!NT_SUCCESS(status))
    {
		debug(
            "Failed to get DevicePropertyHardwareID from PDO. (status=0x%08lX)",
            status);
        return status;
    }
    
    RosKmAdapter* pRosKmAdapter = nullptr;
    /*if (wcscmp(deviceID, L"ACPI\\VEN_BCM&DEV_2850") == 0)
    {
        pRosKmAdapter = new RosKmdRapAdapter(PhysicalDeviceObject, MiniportDeviceContext);
        if (!pRosKmAdapter) {
            debug("Failed to allocate RosKmdRapAdapter.");
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
    */
        pRosKmAdapter = new RosKmdSoftAdapter(PhysicalDeviceObject, MiniportDeviceContext);
        if (!pRosKmAdapter) {
            debug("Failed to allocate RosKmdSoftAdapter.");
            return STATUS_NO_MEMORY;
        }
    //}
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(MiniportDeviceContext);
}

NTSTATUS
RosKmAdapter::QueryEngineStatus(
    DXGKARG_QUERYENGINESTATUS  *pQueryEngineStatus)
{
    debug("QueryEngineStatus was called.");

    pQueryEngineStatus->EngineStatus.Responsive = 1;
    return STATUS_SUCCESS;
}

void RosKmAdapter::WorkerThread(void * inThis)
{
    RosKmAdapter  *pRosKmAdapter = RosKmAdapter::Cast(inThis);

    pRosKmAdapter->DoWork();
}

void RosKmAdapter::DoWork(void)
{
    bool done = false;

    while (!done)
    {
        NTSTATUS status = KeWaitForSingleObject(
            &m_workerThreadEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        status;
        NT_ASSERT(status == STATUS_SUCCESS);

        if (m_workerExit)
        {
            done = true;
            continue;
        }

        for (;;)
        {
            ROSDMABUFSUBMISSION *   pDmaBufSubmission = DequeueDmaBuffer(&m_dmaBufQueueLock);
            if (pDmaBufSubmission == NULL)
            {
                break;
            }

            ROSDMABUFINFO * pDmaBufInfo = pDmaBufSubmission->m_pDmaBufInfo;

            if (pDmaBufInfo->m_DmaBufState.m_bPaging)
            {
                //
                // Run paging buffer in software
                //

                ProcessPagingBuffer(pDmaBufSubmission);

                NotifyDmaBufCompletion(pDmaBufSubmission);
            }
            else
            {
                //
                // Process render DMA buffer
                //

                ProcessRenderBuffer(pDmaBufSubmission);

                NotifyDmaBufCompletion(pDmaBufSubmission);
            }

            ExInterlockedInsertTailList(&m_dmaBufSubmissionFree, &pDmaBufSubmission->m_QueueEntry, &m_dmaBufQueueLock);
        }
    }
}

ROSDMABUFSUBMISSION *
RosKmAdapter::DequeueDmaBuffer(
    KSPIN_LOCK *pDmaBufQueueLock)
{
    LIST_ENTRY *pDmaEntry;

    if (pDmaBufQueueLock)
    {
        pDmaEntry = ExInterlockedRemoveHeadList(&m_dmaBufQueue, pDmaBufQueueLock);
    }
    else
    {
        if (!IsListEmpty(&m_dmaBufQueue))
        {
            pDmaEntry = RemoveHeadList(&m_dmaBufQueue);
        }
        else
        {
            pDmaEntry = NULL;
        }
    }

    return CONTAINING_RECORD(pDmaEntry, ROSDMABUFSUBMISSION, m_QueueEntry);
}

void
RosKmAdapter::ProcessPagingBuffer(
    ROSDMABUFSUBMISSION * pDmaBufSubmission)
{
    ROSDMABUFINFO * pDmaBufInfo = pDmaBufSubmission->m_pDmaBufInfo;

    NT_ASSERT(0 == (pDmaBufSubmission->m_EndOffset - pDmaBufSubmission->m_StartOffset) % sizeof(DXGKARG_BUILDPAGINGBUFFER));

    DXGKARG_BUILDPAGINGBUFFER * pPagingBuffer = (DXGKARG_BUILDPAGINGBUFFER *)(pDmaBufInfo->m_pDmaBuffer + pDmaBufSubmission->m_StartOffset);
    DXGKARG_BUILDPAGINGBUFFER * pEndofBuffer = (DXGKARG_BUILDPAGINGBUFFER *)(pDmaBufInfo->m_pDmaBuffer + pDmaBufSubmission->m_EndOffset);

    for (; pPagingBuffer < pEndofBuffer; pPagingBuffer++)
    {
        switch (pPagingBuffer->Operation)
        {
        case DXGK_OPERATION_FILL:
        {
            NT_ASSERT(pPagingBuffer->Fill.Destination.SegmentId == ROSD_SEGMENT_VIDEO_MEMORY);
            NT_ASSERT(pPagingBuffer->Fill.FillSize % sizeof(ULONG) == 0);

            ULONG * const startAddress = reinterpret_cast<ULONG*>(
                (BYTE *)RosKmdGlobal::s_pVideoMemory +
                pPagingBuffer->Fill.Destination.SegmentAddress.QuadPart);
            for (ULONG * ptr = startAddress;
                 ptr != (startAddress + pPagingBuffer->Fill.FillSize / sizeof(ULONG));
                 ++ptr)
            {
                *ptr = pPagingBuffer->Fill.FillPattern;
            }
        }
        break;
        case DXGK_OPERATION_TRANSFER:
        {
            PBYTE   pSource, pDestination;
            MDL *   pMdlToRestore = NULL;
            CSHORT  savedMdlFlags = 0;
            PBYTE   pKmAddrToUnmap = NULL;

            if (pPagingBuffer->Transfer.Source.SegmentId == ROSD_SEGMENT_VIDEO_MEMORY)
            {
                pSource = ((BYTE *)RosKmdGlobal::s_pVideoMemory) + pPagingBuffer->Transfer.Source.SegmentAddress.QuadPart;
            }
            else
            {
                NT_ASSERT(pPagingBuffer->Transfer.Source.SegmentId == 0);

                pMdlToRestore = pPagingBuffer->Transfer.Source.pMdl;
                savedMdlFlags = pMdlToRestore->MdlFlags;

                pSource = (PBYTE)MmGetSystemAddressForMdlSafe(pPagingBuffer->Transfer.Source.pMdl, HighPagePriority);

                pKmAddrToUnmap = pSource;

                // Adjust the source address by MdlOffset
                pSource += ((LONGLONG)(pPagingBuffer->Transfer.MdlOffset)*PAGE_SIZE);
            }

            if (pPagingBuffer->Transfer.Destination.SegmentId == ROSD_SEGMENT_VIDEO_MEMORY)
            {
                pDestination = ((BYTE *)RosKmdGlobal::s_pVideoMemory) + pPagingBuffer->Transfer.Destination.SegmentAddress.QuadPart;
            }
            else
            {
                NT_ASSERT(pPagingBuffer->Transfer.Destination.SegmentId == 0);

                pMdlToRestore = pPagingBuffer->Transfer.Destination.pMdl;
                savedMdlFlags = pMdlToRestore->MdlFlags;

                pDestination = (PBYTE)MmGetSystemAddressForMdlSafe(pPagingBuffer->Transfer.Destination.pMdl, HighPagePriority);

                pKmAddrToUnmap = pDestination;

                // Adjust the destination address by MdlOffset
                pDestination += ((LONGLONG)(pPagingBuffer->Transfer.MdlOffset)*PAGE_SIZE);
            }

            if (pSource && pDestination)
            {
                RtlCopyMemory(pDestination, pSource, pPagingBuffer->Transfer.TransferSize);
            }
            else
            {
                // TODO[indyz]: Propagate the error back to runtime
                m_ErrorHit.m_PagingFailure = 1;
            }

            // Restore the state of the Mdl (for source or destionation)
            if ((0 == (savedMdlFlags & MDL_MAPPED_TO_SYSTEM_VA)) && pKmAddrToUnmap)
            {
                MmUnmapLockedPages(pKmAddrToUnmap, pMdlToRestore);
            }
        }
        break;

        default:
            NT_ASSERT(false);
        }
    }
}

void
RosKmAdapter::NotifyDmaBufCompletion(
    ROSDMABUFSUBMISSION * pDmaBufSubmission)
{
    ROSDMABUFINFO * pDmaBufInfo = pDmaBufSubmission->m_pDmaBufInfo;

    if (! pDmaBufInfo->m_DmaBufState.m_bPaging)
    {
        pDmaBufInfo->m_DmaBufState.m_bCompleted = 1;
    }

    //
    // Notify the VidSch of the completion of the DMA buffer
    //
    NTSTATUS    Status;

    RtlZeroMemory(&m_interruptData, sizeof(m_interruptData));

    m_interruptData.InterruptType = DXGK_INTERRUPT_DMA_COMPLETED;
    m_interruptData.DmaCompleted.SubmissionFenceId = pDmaBufSubmission->m_SubmissionFenceId;
    m_interruptData.DmaCompleted.NodeOrdinal = 0;
    m_interruptData.DmaCompleted.EngineOrdinal = 0;

    BOOLEAN bRet;

    Status = m_DxgkInterface.DxgkCbSynchronizeExecution(
        m_DxgkInterface.DeviceHandle,
        SynchronizeNotifyInterrupt,
        this,
        0,
        &bRet);

    if (!NT_SUCCESS(Status))
    {
        m_ErrorHit.m_NotifyDmaBufCompletion = 1;
    }
}

BOOLEAN RosKmAdapter::SynchronizeNotifyInterrupt(PVOID inThis)
{
    RosKmAdapter  *pRosKmAdapter = RosKmAdapter::Cast(inThis);

    return pRosKmAdapter->SynchronizeNotifyInterrupt();
}

BOOLEAN RosKmAdapter::SynchronizeNotifyInterrupt(void)
{
    m_DxgkInterface.DxgkCbNotifyInterrupt(m_DxgkInterface.DeviceHandle, &m_interruptData);

    return m_DxgkInterface.DxgkCbQueueDpc(m_DxgkInterface.DeviceHandle);
}

NTSTATUS
RosKmAdapter::Start(
    IN_PDXGK_START_INFO     DxgkStartInfo,
    IN_PDXGKRNL_INTERFACE   DxgkInterface,
    OUT_PULONG              NumberOfVideoPresentSources,
    OUT_PULONG              NumberOfChildren)
{
    m_DxgkStartInfo = *DxgkStartInfo;
    m_DxgkInterface = *DxgkInterface;

    //
    // Render only device has no VidPn source and target
    // Subclass should overwrite these values if it is not render-only.
    //
    *NumberOfVideoPresentSources = 0;
    *NumberOfChildren = 0;

    //
    // Sample for 1.3 model currently
    //
    m_WDDMVersion = DXGKDDI_WDDMv1_3;

    m_NumNodes = C_ROSD_GPU_ENGINE_COUNT;

    //
    // Initialize worker
    //

    KeInitializeEvent(&m_workerThreadEvent, SynchronizationEvent, FALSE);

    m_workerExit = false;

    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              hWorkerThread;

    InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    NTSTATUS status = PsCreateSystemThread(
        &hWorkerThread,
        THREAD_ALL_ACCESS,
        &ObjectAttributes,
        NULL,
        NULL,
        (PKSTART_ROUTINE) RosKmAdapter::WorkerThread,
        this);

    if (status != STATUS_SUCCESS)
    {
        debug(
            "PsCreateSystemThread(...) failed for RosKmAdapter::WorkerThread. (status=0x%08lX)",
            status);
        return status;
    }

    status = ObReferenceObjectByHandle(
        hWorkerThread,
        THREAD_ALL_ACCESS,
        *PsThreadType,
        KernelMode,
        (PVOID *)&m_pWorkerThread,
        NULL);

    ZwClose(hWorkerThread);

    if (!NT_SUCCESS(status))
    {
        debug(
            "ObReferenceObjectByHandle(...) failed for worker thread. (status=0x%08lX)",
            status);
        return status;
    }

    status = m_DxgkInterface.DxgkCbGetDeviceInformation(
        m_DxgkInterface.DeviceHandle,
        &m_deviceInfo);
    if (!NT_SUCCESS(status))
    {
        debug(
            "DxgkCbGetDeviceInformation(...) failed. (status=0x%08lX, m_DxgkInterface.DeviceHandle=%p)",
            status,
            m_DxgkInterface.DeviceHandle);
        return status;
    }

    //
    // Query APCI device ID
    //
    {
        NTSTATUS acpiStatus;

        RosKmAcpiReader acpiReader(this, DISPLAY_ADAPTER_HW_ID);
        acpiStatus = acpiReader.Read(ACPI_METHOD_HARDWARE_ID);
        if (NT_SUCCESS(acpiStatus) && (acpiReader.GetOutputArgumentCount() == 1))
        {
            RosKmAcpiArgumentParser acpiParser(&acpiReader, NULL);
            char *pDeviceId;
            ULONG DeviceIdLength;
            acpiStatus = acpiParser.GetAnsiString(&pDeviceId, &DeviceIdLength);
            if (NT_SUCCESS(acpiStatus) && DeviceIdLength)
            {
                m_deviceIdLength = min(DeviceIdLength, sizeof(m_deviceId));
                RtlCopyMemory(&m_deviceId[0], pDeviceId, m_deviceIdLength);
            }
        }
    }

    //
    // Initialize power component data.
    //
    InitializePowerComponentInfo();

    //
    // Initialize apperture state
    //

    memset(m_aperturePageTable, 0, sizeof(m_aperturePageTable));

    //
    // Intialize DMA buffer queue and lock
    //

    InitializeListHead(&m_dmaBufSubmissionFree);
    for (UINT i = 0; i < m_maxDmaBufQueueLength; i++)
    {
        InsertHeadList(&m_dmaBufSubmissionFree, &m_dmaBufSubssions[i].m_QueueEntry);
    }

    InitializeListHead(&m_dmaBufQueue);
    KeInitializeSpinLock(&m_dmaBufQueueLock);

    //
    // Initialize HW DMA buffer compeletion DPC and event
    //

    KeInitializeEvent(&m_hwDmaBufCompletionEvent, SynchronizationEvent, FALSE);
    KeInitializeDpc(&m_hwDmaBufCompletionDpc, HwDmaBufCompletionDpcRoutine, this);

    debug("Adapter was successfully started.");
    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::Stop()
{
    m_workerExit = true;

    KeSetEvent(&m_workerThreadEvent, 0, FALSE);

    NTSTATUS status = KeWaitForSingleObject(
        m_pWorkerThread,
        Executive,
        KernelMode,
        FALSE,
        NULL);

    status;
    NT_ASSERT(status == STATUS_SUCCESS);

    ObDereferenceObject(m_pWorkerThread);

    debug("Adapter was successfully stopped.");
    return STATUS_SUCCESS;
}

void RosKmAdapter::DpcRoutine(void)
{
    // dp nothing other than calling back into dxgk

    m_DxgkInterface.DxgkCbNotifyDpc(m_DxgkInterface.DeviceHandle);
}

NTSTATUS
RosKmAdapter::BuildPagingBuffer(
    IN_PDXGKARG_BUILDPAGINGBUFFER   pArgs)
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PBYTE       pDmaBufStart = (PBYTE)pArgs->pDmaBuffer;
    PBYTE       pDmaBufPos = (PBYTE)pArgs->pDmaBuffer;

    //
    // hAllocation is NULL for operation on DMA buffer and pages mapped into aperture
    //

    //
    // If there is insufficient space left in DMA buffer, we should return
    // STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER.
    //

    switch (pArgs->Operation)
    {
    case DXGK_OPERATION_MAP_APERTURE_SEGMENT:
    {
        if (pArgs->MapApertureSegment.SegmentId == kApertureSegmentId)
        {
            size_t pageIndex = pArgs->MapApertureSegment.OffsetInPages;
            size_t pageCount = pArgs->MapApertureSegment.NumberOfPages;

            NT_ASSERT(pageIndex + pageCount <= kApertureSegmentPageCount);

            size_t mdlPageOffset = pArgs->MapApertureSegment.MdlOffset;

            PMDL pMdl = pArgs->MapApertureSegment.pMdl;

            for (UINT i = 0; i < pageCount; i++)
            {
                m_aperturePageTable[pageIndex + i] = MmGetMdlPfnArray(pMdl)[mdlPageOffset + i];
            }
        }

    }
    break;

    case DXGK_OPERATION_UNMAP_APERTURE_SEGMENT:
    {
        if (pArgs->MapApertureSegment.SegmentId == kApertureSegmentId)
        {
            size_t pageIndex = pArgs->MapApertureSegment.OffsetInPages;
            size_t pageCount = pArgs->MapApertureSegment.NumberOfPages;

            NT_ASSERT(pageIndex + pageCount <= kApertureSegmentPageCount);

            while (pageCount--)
            {
                m_aperturePageTable[pageIndex++] = 0;
            }
        }
    }

    break;

    case DXGK_OPERATION_FILL:
    {
        RosKmdAllocation * pRosKmdAllocation = (RosKmdAllocation *)pArgs->Fill.hAllocation;
        pRosKmdAllocation;

        debug(
            "Filling DMA buffer. (Destination.SegmentAddress=0x%I64x, FillPattern=0x%lx, FillSize=%Id)",
            pArgs->Fill.Destination.SegmentAddress.QuadPart,
            pArgs->Fill.FillPattern,
            pArgs->Fill.FillSize);

        if (pArgs->DmaSize < sizeof(DXGKARG_BUILDPAGINGBUFFER))
        {
            debug(
                "DXGK_OPERATION_FILL: DMA buffer size is too small. (pArgs->DmaSize=%d, sizeof(DXGKARG_BUILDPAGINGBUFFER)=%llu)",
                pArgs->DmaSize,
                sizeof(DXGKARG_BUILDPAGINGBUFFER));
            return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
        }
        else
        {
            *((DXGKARG_BUILDPAGINGBUFFER *)pArgs->pDmaBuffer) = *pArgs;

            pDmaBufPos += sizeof(DXGKARG_BUILDPAGINGBUFFER);
        }
    }
    break;

    case DXGK_OPERATION_DISCARD_CONTENT:
    {
        // do nothing
    }
    break;

    case DXGK_OPERATION_TRANSFER:
    {
        if (pArgs->DmaSize < sizeof(DXGKARG_BUILDPAGINGBUFFER))
        {
            debug(
                "DXGK_OPERATION_TRANSFER: DMA buffer is too small. (pArgs->DmaSize=%d, sizeof(DXGKARG_BUILDPAGINGBUFFER)=%llu)",
                pArgs->DmaSize,
                sizeof(DXGKARG_BUILDPAGINGBUFFER));
            return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER;
        }
        else
        {
            *((DXGKARG_BUILDPAGINGBUFFER *)pArgs->pDmaBuffer) = *pArgs;

            pDmaBufPos += sizeof(DXGKARG_BUILDPAGINGBUFFER);
        }
    }
    break;

    default:
    {
        NT_ASSERT(false);

        m_ErrorHit.m_UnSupportedPagingOp = 1;
        Status = STATUS_SUCCESS;
    }
    break;
    }

    //
    // Update pDmaBuffer to point past the last byte used.
    pArgs->pDmaBuffer = pDmaBufPos;

    // Record DMA buffer information only when it is newly used
    ROSDMABUFINFO * pDmaBufInfo = (ROSDMABUFINFO *)pArgs->pDmaBufferPrivateData;
    if (pDmaBufInfo && (pArgs->DmaSize == ROSD_PAGING_BUFFER_SIZE))
    {
        pDmaBufInfo->m_DmaBufState.m_Value = 0;
        pDmaBufInfo->m_DmaBufState.m_bPaging = 1;

        pDmaBufInfo->m_pDmaBuffer = pDmaBufStart;
        pDmaBufInfo->m_DmaBufferSize = pArgs->DmaSize;
    }

    return Status;
}

NTSTATUS
RosKmAdapter::DispatchIoRequest(
    IN_ULONG                    VidPnSourceId,
    IN_PVIDEO_REQUEST_PACKET    VideoRequestPacket)
{
    if (RosKmdGlobal::IsRenderOnly())
    {
        debug(
            "Unsupported IO Control Code. (VideoRequestPacketPtr->IoControlCode = 0x%lx)",
            VideoRequestPacket->IoControlCode);
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(VidPnSourceId); //m_display.DispatchIoRequest(VidPnSourceId, VideoRequestPacket);
}

NTSTATUS
RosKmAdapter::SubmitCommand(
    IN_CONST_PDXGKARG_SUBMITCOMMAND     pSubmitCommand)
{
    NTSTATUS        Status = STATUS_SUCCESS;

    // NOTE: pRosKmContext will be NULL for paging operations
    RosKmContext *pRosKmContext = (RosKmContext *)pSubmitCommand->hContext;
    pRosKmContext;

    QueueDmaBuffer(pSubmitCommand);

    //
    // Wake up the worker thread for the GPU node
    //
    KeSetEvent(&m_workerThreadEvent, 0, FALSE);

    return Status;
}

NTSTATUS
RosKmAdapter::Patch(
    IN_CONST_PDXGKARG_PATCH     pPatch)
{
    ROSDMABUFINFO *pDmaBufInfo = (ROSDMABUFINFO *)pPatch->pDmaBufferPrivateData;

    RosKmContext * pRosKmContext = (RosKmContext *)pPatch->hContext;
    pRosKmContext;

    pDmaBufInfo->m_DmaBufferPhysicalAddress = pPatch->DmaBufferPhysicalAddress;

    PatchDmaBuffer(
        pDmaBufInfo,
        pPatch->pAllocationList,
        pPatch->AllocationListSize,
        pPatch->pPatchLocationList + pPatch->PatchLocationListSubmissionStart,
        pPatch->PatchLocationListSubmissionLength);

    // Record DMA buffer information
    pDmaBufInfo->m_DmaBufState.m_bPatched = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::CreateAllocation(
    INOUT_PDXGKARG_CREATEALLOCATION     pCreateAllocation)
{
    NT_ASSERT(pCreateAllocation->PrivateDriverDataSize == sizeof(RosAllocationGroupExchange));
    RosAllocationGroupExchange * pRosAllocationGroupExchange = (RosAllocationGroupExchange *)pCreateAllocation->pPrivateDriverData;

    pRosAllocationGroupExchange;
    NT_ASSERT(pRosAllocationGroupExchange->m_dummy == 0);

    NT_ASSERT(pCreateAllocation->NumAllocations == 1);

    DXGK_ALLOCATIONINFO * pAllocationInfo = pCreateAllocation->pAllocationInfo;

    NT_ASSERT(pAllocationInfo->PrivateDriverDataSize == sizeof(RosAllocationExchange));
    RosAllocationExchange * pRosAllocation = (RosAllocationExchange *)pAllocationInfo->pPrivateDriverData;

    RosKmdAllocation * pRosKmdAllocation = (RosKmdAllocation *)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(RosKmdAllocation), 'ROSD');
    if (!pRosKmdAllocation)
    {
        debug(
            "Failed to allocated nonpaged pool for RosKmdAllocation. (sizeof(RosKmdAllocation)=%llu)",
            sizeof(RosKmdAllocation));
        return STATUS_NO_MEMORY;
    }

    *(RosAllocationExchange *)pRosKmdAllocation = *pRosAllocation;

    pAllocationInfo->hAllocation = pRosKmdAllocation;

    pAllocationInfo->Alignment = 64;
    pAllocationInfo->AllocationPriority = D3DDDI_ALLOCATIONPRIORITY_NORMAL;
    pAllocationInfo->EvictionSegmentSet = 0; // don't use apperture for eviction

    pAllocationInfo->Flags.Value = 0;

    //
    // Allocations should be marked CPU visible unless they are shared or
    // can be flipped.
    // Shared allocations (including the primary) cannot be CPU visible unless
    // they are exclusively located in an aperture segment.
    //
    pAllocationInfo->Flags.CpuVisible =
        !((pRosAllocation->m_miscFlags & D3D10_DDI_RESOURCE_MISC_SHARED) ||
          (pRosAllocation->m_bindFlags & D3D10_DDI_BIND_PRESENT));

    // Allocations that will be flipped, such as the primary allocation,
    // cannot be cached.
    pAllocationInfo->Flags.Cached = pAllocationInfo->Flags.CpuVisible;

    pAllocationInfo->HintedBank.Value = 0;
    pAllocationInfo->MaximumRenamingListLength = 0;
    pAllocationInfo->pAllocationUsageHint = NULL;
    pAllocationInfo->PhysicalAdapterIndex = 0;
    pAllocationInfo->PitchAlignedSize = 0;
    pAllocationInfo->PreferredSegment.Value = 0;
    pAllocationInfo->PreferredSegment.SegmentId0 = ROSD_SEGMENT_VIDEO_MEMORY;
    pAllocationInfo->PreferredSegment.Direction0 = 0;

    // zero-size allocations are not allowed
    NT_ASSERT(pRosAllocation->m_hwSizeBytes != 0);
    pAllocationInfo->Size = pRosAllocation->m_hwSizeBytes;

    pAllocationInfo->SupportedReadSegmentSet = 1 << (ROSD_SEGMENT_VIDEO_MEMORY - 1);
    pAllocationInfo->SupportedWriteSegmentSet = 1 << (ROSD_SEGMENT_VIDEO_MEMORY - 1);
    /*
    if (pCreateAllocation->Flags.Resource && pCreateAllocation->hResource == NULL && pRosKmdResource != NULL)
    {
        pCreateAllocation->hResource = pRosKmdResource;
    }
    */
    debug(
        "Created allocation. (Flags.CpuVisible=%d, Flags.Cacheable=%d, Size=%Id)",
        pAllocationInfo->Flags.CpuVisible,
        pAllocationInfo->Flags.Cached,
        pAllocationInfo->Size);

    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::DestroyAllocation(
    IN_CONST_PDXGKARG_DESTROYALLOCATION     pDestroyAllocation)
{
    NT_ASSERT(pDestroyAllocation->NumAllocations == 1);
    RosKmdAllocation* pRosKmdAllocation = (RosKmdAllocation *)pDestroyAllocation->pAllocationList[0];
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(pRosKmdAllocation);
}

NTSTATUS
RosKmAdapter::QueryAdapterInfo(
    IN_CONST_PDXGKARG_QUERYADAPTERINFO      pQueryAdapterInfo)
{
    debug(
        "QueryAdapterInfo was called. (Type=%d)",
        pQueryAdapterInfo->Type);

    switch (pQueryAdapterInfo->Type)
    {
    case DXGKQAITYPE_UMDRIVERPRIVATE:
    {
        if (pQueryAdapterInfo->OutputDataSize < sizeof(ROSADAPTERINFO))
        {
            debug(
                "Output buffer is too small. (pQueryAdapterInfo->OutputDataSize=%d, sizeof(ROSADAPTERINFO)=%llu)",
                pQueryAdapterInfo->OutputDataSize,
                sizeof(ROSADAPTERINFO));
            return STATUS_BUFFER_TOO_SMALL;
        }
        ROSADAPTERINFO* pRosAdapterInfo = (ROSADAPTERINFO*)pQueryAdapterInfo->pOutputData;

        pRosAdapterInfo->m_version = ROSD_VERSION;
        pRosAdapterInfo->m_wddmVersion = m_WDDMVersion;

        // Software APCI device only claims an interrupt resource
        pRosAdapterInfo->m_isSoftwareDevice = (m_flags.m_isVC4 != 1);

        RtlCopyMemory(
            pRosAdapterInfo->m_deviceId,
            m_deviceId,
            m_deviceIdLength);
    }
    break;

    case DXGKQAITYPE_DRIVERCAPS:
    {
        if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_DRIVERCAPS))
        {
            debug(
                "Output buffer is too small. (pQueryAdapterInfo->OutputDataSize=%d, sizeof(DXGK_DRIVERCAPS)=%llu)",
                pQueryAdapterInfo->OutputDataSize,
                sizeof(DXGK_DRIVERCAPS));
            return STATUS_BUFFER_TOO_SMALL;
        }

        DXGK_DRIVERCAPS* pDriverCaps = (DXGK_DRIVERCAPS *)pQueryAdapterInfo->pOutputData;
        pDriverCaps->HighestAcceptableAddress.QuadPart = -1;
        pDriverCaps->PresentationCaps.SupportKernelModeCommandBuffer = FALSE;
        pDriverCaps->PresentationCaps.SupportSoftwareDeviceBitmaps = TRUE;
        pDriverCaps->PresentationCaps.NoScreenToScreenBlt = TRUE;
        pDriverCaps->PresentationCaps.NoOverlapScreenBlt = TRUE;
        pDriverCaps->PresentationCaps.MaxTextureWidthShift = 3;
        pDriverCaps->PresentationCaps.MaxTextureHeightShift = 3;
        pDriverCaps->FlipCaps.FlipOnVSyncMmIo = TRUE;
        if (!RosKmdGlobal::IsRenderOnly())
        {
            pDriverCaps->MaxQueuedFlipOnVSync = 1;
            pDriverCaps->FlipCaps.FlipOnVSyncWithNoWait = TRUE;
            pDriverCaps->FlipCaps.FlipInterval = FALSE;
            pDriverCaps->FlipCaps.FlipImmediateMmIo = FALSE;
            pDriverCaps->FlipCaps.FlipIndependent = TRUE;
        }
        pDriverCaps->SchedulingCaps.MultiEngineAware = 1;
        pDriverCaps->SchedulingCaps.CancelCommandAware = 1;
        pDriverCaps->SchedulingCaps.PreemptionAware = 1;
        pDriverCaps->MemoryManagementCaps.CrossAdapterResource = 1;
        pDriverCaps->GpuEngineTopology.NbAsymetricProcessingNodes = m_NumNodes;
        pDriverCaps->WDDMVersion = m_WDDMVersion;
        pDriverCaps->PreemptionCaps.GraphicsPreemptionGranularity = D3DKMDT_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY;
        pDriverCaps->PreemptionCaps.ComputePreemptionGranularity = D3DKMDT_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY;
        pDriverCaps->SupportNonVGA = TRUE;
        pDriverCaps->SupportSmoothRotation = TRUE;
        pDriverCaps->SupportPerEngineTDR = 1;
        pDriverCaps->SupportDirectFlip = 1;
        pDriverCaps->SupportRuntimePowerManagement = FALSE;
    }
    break;
    case DXGKQAITYPE_QUERYSEGMENT3:
    {
        if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_QUERYSEGMENTOUT3))
        {
            debug(
                "Output buffer is too small. (pQueryAdapterInfo->OutputDataSize=%d, sizeof(DXGK_QUERYSEGMENTOUT3)=%llu)",
                pQueryAdapterInfo->OutputDataSize,
                sizeof(DXGK_QUERYSEGMENTOUT3));
            return STATUS_BUFFER_TOO_SMALL;
        }

        DXGK_QUERYSEGMENTOUT3* pSegmentInfo = (DXGK_QUERYSEGMENTOUT3*)pQueryAdapterInfo->pOutputData;
        if (!pSegmentInfo[0].pSegmentDescriptor)
        {
            pSegmentInfo->NbSegment = 2;
        }
        else
        {
            DXGK_SEGMENTDESCRIPTOR3* pSegmentDesc = pSegmentInfo->pSegmentDescriptor;
            pSegmentInfo->PagingBufferPrivateDataSize = sizeof(ROSUMDDMAPRIVATEDATA2);
            pSegmentInfo->PagingBufferSegmentId = ROSD_SEGMENT_APERTURE;
            pSegmentInfo->PagingBufferSize = PAGE_SIZE;
            memset(&pSegmentDesc[0], 0, sizeof(pSegmentDesc[0]));
            pSegmentDesc[0].Flags.Aperture = TRUE;
            pSegmentDesc[0].Flags.CacheCoherent = TRUE;
            pSegmentDesc[0].BaseAddress.QuadPart = ROSD_SEGMENT_APERTURE_BASE_ADDRESS;
            pSegmentDesc[0].CpuTranslatedAddress.QuadPart = 0xFFFFFFFE00000000;
            pSegmentDesc[0].Flags.CpuVisible = TRUE;
            pSegmentDesc[0].Size = kApertureSegmentSize;
            pSegmentDesc[0].CommitLimit = kApertureSegmentSize;
            memset(&pSegmentDesc[1], 0, sizeof(pSegmentDesc[1]));
            pSegmentDesc[1].BaseAddress.QuadPart = 0LL; // Gpu base physical address
            pSegmentDesc[1].Flags.CpuVisible = true;
            pSegmentDesc[1].Flags.CacheCoherent = true;
            pSegmentDesc[1].Flags.DirectFlip = true;
            pSegmentDesc[1].CpuTranslatedAddress = RosKmdGlobal::s_videoMemoryPhysicalAddress; // cpu base physical address
            pSegmentDesc[1].Size = 0; // m_localVidMemSegmentSize;
        }
    }
    break;

    case DXGKQAITYPE_NUMPOWERCOMPONENTS:
    {
        if (pQueryAdapterInfo->OutputDataSize != sizeof(UINT))
        {
            debug(
                "Output buffer is unexpected size. (pQueryAdapterInfo->OutputDataSize=%d, sizeof(UINT)=%llu)",
                pQueryAdapterInfo->OutputDataSize,
                sizeof(UINT));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Support only one 3D engine(s).
        //
        *(static_cast<UINT*>(pQueryAdapterInfo->pOutputData)) = GetNumPowerComponents();
    }
    break;

    case DXGKQAITYPE_POWERCOMPONENTINFO:
    {
        if (pQueryAdapterInfo->InputDataSize != sizeof(UINT))
        {
            debug(
                "Input buffer is not of the expected size. (pQueryAdapterInfo->InputDataSize=%d, sizeof(UINT)=%llu)",
                pQueryAdapterInfo->InputDataSize,
                sizeof(UINT));
            return STATUS_INVALID_PARAMETER;
        }

        if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_POWER_RUNTIME_COMPONENT))
        {
            debug(
                "Output buffer is too small. (pQueryAdapterInfo->OutputDataSize=%d, sizeof(DXGK_POWER_RUNTIME_COMPONENT)=%llu)",
                pQueryAdapterInfo->OutputDataSize,
                sizeof(DXGK_POWER_RUNTIME_COMPONENT));
            return STATUS_BUFFER_TOO_SMALL;
        }

        ULONG ComponentIndex = *(reinterpret_cast<UINT*>(pQueryAdapterInfo->pInputData));
        DXGK_POWER_RUNTIME_COMPONENT* pPowerComponent = reinterpret_cast<DXGK_POWER_RUNTIME_COMPONENT*>(pQueryAdapterInfo->pOutputData);

        NTSTATUS status = GetPowerComponentInfo(ComponentIndex, pPowerComponent);
        if (!NT_SUCCESS(status))
        {
            debug(
                "GetPowerComponentInfo(...) failed. (status=0x%08lX, ComponentIndex=%d, pPowerComponent=%p)",
                status,
                ComponentIndex,
                pPowerComponent);
            return status;
        }
    }
    break;

    case DXGKQAITYPE_HISTORYBUFFERPRECISION:
    {
        UINT NumStructures = pQueryAdapterInfo->OutputDataSize / sizeof(DXGKARG_HISTORYBUFFERPRECISION);

        for (UINT i = 0; i < NumStructures; i++)
        {
            DXGKARG_HISTORYBUFFERPRECISION *pHistoryBufferPrecision = ((DXGKARG_HISTORYBUFFERPRECISION *)pQueryAdapterInfo->pOutputData) + i;

            pHistoryBufferPrecision->PrecisionBits = 64;
        }

    }
    break;

    default:
        debug(
            "Unsupported query type. (pQueryAdapterInfo->Type=%d, pQueryAdapterInfo=0x%p)",
            pQueryAdapterInfo->Type,
            pQueryAdapterInfo);
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::DescribeAllocation(
    INOUT_PDXGKARG_DESCRIBEALLOCATION       pDescribeAllocation)
{
    RosKmdAllocation *pAllocation = (RosKmdAllocation *)pDescribeAllocation->hAllocation;

    pDescribeAllocation->Width = pAllocation->m_mip0Info.TexelWidth;
    pDescribeAllocation->Height = pAllocation->m_mip0Info.TexelHeight;
    pDescribeAllocation->Format = TranslateDxgiFormat(pAllocation->m_format);

    pDescribeAllocation->MultisampleMethod.NumSamples = pAllocation->m_sampleDesc.Count;
    pDescribeAllocation->MultisampleMethod.NumQualityLevels = pAllocation->m_sampleDesc.Quality;

    pDescribeAllocation->RefreshRate.Numerator = pAllocation->m_primaryDesc.ModeDesc.RefreshRate.Numerator;
    pDescribeAllocation->RefreshRate.Denominator = pAllocation->m_primaryDesc.ModeDesc.RefreshRate.Denominator;

    return STATUS_SUCCESS;

}

NTSTATUS
RosKmAdapter::GetNodeMetadata(
    UINT                            NodeOrdinal,
    OUT_PDXGKARG_GETNODEMETADATA    pGetNodeMetadata
    )
{
    RtlZeroMemory(pGetNodeMetadata, sizeof(*pGetNodeMetadata));

    pGetNodeMetadata->EngineType = DXGK_ENGINE_TYPE_3D;

    RtlStringCbPrintfW(pGetNodeMetadata->FriendlyName,
        sizeof(pGetNodeMetadata->FriendlyName),
        L"3DNode%02X",
        NodeOrdinal);

    return STATUS_SUCCESS;
}


NTSTATUS
RosKmAdapter::SubmitCommandVirtual(
    IN_CONST_PDXGKARG_SUBMITCOMMANDVIRTUAL  /*pSubmitCommandVirtual*/)
{
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::PreemptCommand(
    IN_CONST_PDXGKARG_PREEMPTCOMMAND    /*pPreemptCommand*/)
{
    debug("Not implemented");
    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::RestartFromTimeout(void)
{
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::CancelCommand(
    IN_CONST_PDXGKARG_CANCELCOMMAND /*pCancelCommand*/)
{
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::QueryCurrentFence(
    INOUT_PDXGKARG_QUERYCURRENTFENCE pCurrentFence)
{
    debug("Not implemented");

    NT_ASSERT(pCurrentFence->NodeOrdinal == 0);
    NT_ASSERT(pCurrentFence->EngineOrdinal == 0);

    pCurrentFence->CurrentFence = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::ResetEngine(
    INOUT_PDXGKARG_RESETENGINE  /*pResetEngine*/)
{
    debug("Not implemented");
    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::CollectDbgInfo(
    IN_CONST_PDXGKARG_COLLECTDBGINFO        /*pCollectDbgInfo*/)
{
    debug("Not implemented");
    return STATUS_SUCCESS;
}

NTSTATUS
RosKmAdapter::CreateProcess(
    IN DXGKARG_CREATEPROCESS* /*pArgs*/)
{
    // pArgs->hKmdProcess = 0;
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::DestroyProcess(
    IN HANDLE /*KmdProcessHandle*/)
{
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

void
RosKmAdapter::SetStablePowerState(
    IN_CONST_PDXGKARG_SETSTABLEPOWERSTATE  pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);
    debug("Not implemented");
}

NTSTATUS
RosKmAdapter::CalibrateGpuClock(
    IN UINT32                                   /*NodeOrdinal*/,
    IN UINT32                                   /*EngineOrdinal*/,
    OUT_PDXGKARG_CALIBRATEGPUCLOCK              /*pClockCalibration*/
    )
{
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::Escape(
    IN_CONST_PDXGKARG_ESCAPE        pEscape)
{
    NTSTATUS        Status;

    if (pEscape->PrivateDriverDataSize < sizeof(UINT))
    {
        debug(
            "PrivateDriverDataSize is too small. (pEscape->PrivateDriverDataSize=%d, sizeof(UINT)=%llu)",
            pEscape->PrivateDriverDataSize,
            sizeof(UINT));
        return STATUS_BUFFER_TOO_SMALL;
    }

    UINT    EscapeId = *((UINT *)pEscape->pPrivateDriverData);
    switch (EscapeId)
    {

    default:

        NT_ASSERT(false);
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::ResetFromTimeout(void)
{
    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RosKmAdapter::QueryChildRelations(
    INOUT_PDXGK_CHILD_DESCRIPTOR    ChildRelations,
    IN_ULONG                        ChildRelationsSize)
{
    if (RosKmdGlobal::IsRenderOnly())
    {
        debug("QueryChildRelations() is not supported by render-only driver.");
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(ChildRelationsSize); UNREFERENCED_PARAMETER(ChildRelations); // m_display.QueryChildRelations(ChildRelations, ChildRelationsSize);
}

NTSTATUS
RosKmAdapter::QueryChildStatus(
    IN_PDXGK_CHILD_STATUS   ChildStatus,
    IN_BOOLEAN              NonDestructiveOnly)
{
    if (RosKmdGlobal::IsRenderOnly())
    {
        debug("QueryChildStatus() is not supported by render-only driver.");
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(NonDestructiveOnly); UNREFERENCED_PARAMETER(ChildStatus); // m_display.QueryChildStatus(ChildStatus, NonDestructiveOnly);
}

NTSTATUS
RosKmAdapter::QueryDeviceDescriptor(
    IN_ULONG                        ChildUid,
    INOUT_PDXGK_DEVICE_DESCRIPTOR   pDeviceDescriptor)
{
    if (RosKmdGlobal::IsRenderOnly())
    {
        debug("QueryChildStatus() is not supported by render-only driver.");
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(ChildUid); UNREFERENCED_PARAMETER(pDeviceDescriptor); // m_display.QueryDeviceDescriptor(ChildUid, pDeviceDescriptor);
}

NTSTATUS
RosKmAdapter::NotifyAcpiEvent(
    IN_DXGK_EVENT_TYPE  EventType,
    IN_ULONG            Event,
    IN_PVOID            Argument,
    OUT_PULONG          AcpiFlags)
{
    EventType;
    Event;
    Argument;
    AcpiFlags;

    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED;
}

void
RosKmAdapter::ResetDevice(void)
{
    // Do nothing
    debug("Not implemented");
}

void
RosKmAdapter::PatchDmaBuffer(
    ROSDMABUFINFO*                  pDmaBufInfo,
    CONST DXGK_ALLOCATIONLIST*      pAllocationList,
    UINT                            allocationListSize,
    CONST D3DDDI_PATCHLOCATIONLIST* pPatchLocationList,
    UINT                            patchAllocationList)
{
    PBYTE       pDmaBuf = (PBYTE)pDmaBufInfo->m_pDmaBuffer;

    for (UINT i = 0; i < patchAllocationList; i++)
    {
        auto patch = &pPatchLocationList[i];

        allocationListSize;
        NT_ASSERT(patch->AllocationIndex < allocationListSize);

        auto allocation = &pAllocationList[patch->AllocationIndex];

        RosKmdDeviceAllocation * pRosKmdDeviceAllocation = (RosKmdDeviceAllocation *)allocation->hDeviceSpecificAllocation;

        if (allocation->SegmentId != 0)
        {
            debug("Patch RosKmdDeviceAllocation %p at 0x%016llX\n", pRosKmdDeviceAllocation, allocation->PhysicalAddress.QuadPart);
            debug("Patch buffer offset %lx allocation offset %lx\n", patch->PatchOffset, patch->AllocationOffset);

            // Patch in dma buffer
            NT_ASSERT(allocation->SegmentId == ROSD_SEGMENT_VIDEO_MEMORY);
            if (pDmaBufInfo->m_DmaBufState.m_bSwCommandBuffer)
            {
                PHYSICAL_ADDRESS    allocAddress;

                allocAddress.QuadPart = allocation->PhysicalAddress.QuadPart + (LONGLONG)patch->AllocationOffset;
                *((PHYSICAL_ADDRESS *)(pDmaBuf + patch->PatchOffset)) = allocAddress;
            }
            else
            {
                // Patch HW command buffer
            }
        }
    }
}

//
// TODO[indyz]: Add proper validation for DMA buffer
//
bool
RosKmAdapter::ValidateDmaBuffer(
    ROSDMABUFINFO*                  pDmaBufInfo,
    CONST DXGK_ALLOCATIONLIST*      pAllocationList,
    UINT                            allocationListSize,
    CONST D3DDDI_PATCHLOCATIONLIST* pPatchLocationList,
    UINT                            patchAllocationList)
{
    PBYTE           pDmaBuf = (PBYTE)pDmaBufInfo->m_pDmaBuffer;
    bool            bValidateDmaBuffer = true;
    ROSDMABUFSTATE* pDmaBufState = &pDmaBufInfo->m_DmaBufState;

    pDmaBuf;

    if (! pDmaBufInfo->m_DmaBufState.m_bSwCommandBuffer)
    {
        for (UINT i = 0; i < patchAllocationList; i++)
        {
            auto patch = &pPatchLocationList[i];

            allocationListSize;
            NT_ASSERT(patch->AllocationIndex < allocationListSize);

            auto allocation = &pAllocationList[patch->AllocationIndex];

            RosKmdDeviceAllocation * pRosKmdDeviceAllocation = (RosKmdDeviceAllocation *)allocation->hDeviceSpecificAllocation; UNREFERENCED_PARAMETER(pRosKmdDeviceAllocation);
        }
    }

    return bValidateDmaBuffer; UNREFERENCED_PARAMETER(pDmaBufState);
}

void
RosKmAdapter::QueueDmaBuffer(
    IN_CONST_PDXGKARG_SUBMITCOMMAND pSubmitCommand)
{
    ROSDMABUFINFO *         pDmaBufInfo = (ROSDMABUFINFO *)pSubmitCommand->pDmaBufferPrivateData;
    KIRQL                   OldIrql;
    ROSDMABUFSUBMISSION *   pDmaBufSubmission;

    KeAcquireSpinLock(&m_dmaBufQueueLock, &OldIrql);

    //
    // Combination indicating preparation error, thus the DMA buffer should be discarded
    //
    if ((pSubmitCommand->DmaBufferPhysicalAddress.QuadPart == 0) &&
        (pSubmitCommand->DmaBufferSubmissionStartOffset == 0) &&
        (pSubmitCommand->DmaBufferSubmissionEndOffset == 0))
    {
        m_ErrorHit.m_PreparationError = 1;
    }

    if (!pDmaBufInfo->m_DmaBufState.m_bSubmittedOnce)
    {
        pDmaBufInfo->m_DmaBufState.m_bSubmittedOnce = 1;
    }

    NT_ASSERT(!IsListEmpty(&m_dmaBufSubmissionFree));

    pDmaBufSubmission = CONTAINING_RECORD(RemoveHeadList(&m_dmaBufSubmissionFree), ROSDMABUFSUBMISSION, m_QueueEntry);

    pDmaBufSubmission->m_pDmaBufInfo = pDmaBufInfo;

    pDmaBufSubmission->m_StartOffset = pSubmitCommand->DmaBufferSubmissionStartOffset;
    pDmaBufSubmission->m_EndOffset = pSubmitCommand->DmaBufferSubmissionEndOffset;
    pDmaBufSubmission->m_SubmissionFenceId = pSubmitCommand->SubmissionFenceId;

    InsertTailList(&m_dmaBufQueue, &pDmaBufSubmission->m_QueueEntry);

    KeReleaseSpinLock(&m_dmaBufQueueLock, OldIrql);
}

void
RosKmAdapter::HwDmaBufCompletionDpcRoutine(
    KDPC   *pDPC,
    PVOID   deferredContext,
    PVOID   systemArgument1,
    PVOID   systemArgument2)
{
    RosKmAdapter   *pRosKmAdapter = RosKmAdapter::Cast(deferredContext);

    UNREFERENCED_PARAMETER(pDPC);
    UNREFERENCED_PARAMETER(systemArgument1);
    UNREFERENCED_PARAMETER(systemArgument2);

    // Signal to the worker thread that a HW DMA buffer has completed
    KeSetEvent(&pRosKmAdapter->m_hwDmaBufCompletionEvent, 0, FALSE);
}

 //================================================


NTSTATUS RosKmAdapter::SetVidPnSourceAddress (
    IN_CONST_PDXGKARG_SETVIDPNSOURCEADDRESS SetVidPnSourceAddressPtr
    )
{
    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(SetVidPnSourceAddressPtr); // m_display.SetVidPnSourceAddress(SetVidPnSourceAddressPtr);
}

 //==================================================
 //===================================================


NTSTATUS RosKmAdapter::QueryInterface (QUERY_INTERFACE* Args)
{
    debug(
        "Received QueryInterface for unsupported interface. (InterfaceType=%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X)",
        (UINT32)Args->InterfaceType->Data1, (UINT16)Args->InterfaceType->Data2, (UINT16)Args->InterfaceType->Data3,
        (UINT8)Args->InterfaceType->Data4[0],
        (UINT8)Args->InterfaceType->Data4[1],
        (UINT8)Args->InterfaceType->Data4[2],
        (UINT8)Args->InterfaceType->Data4[3],
        (UINT8)Args->InterfaceType->Data4[4],
        (UINT8)Args->InterfaceType->Data4[5],
        (UINT8)Args->InterfaceType->Data4[6],
        (UINT8)Args->InterfaceType->Data4[7]);
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS RosKmAdapter::GetStandardAllocationDriverData (
    DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA* Args
    )
{
    debug("RosKmAdapter::GetStandardAllocationDriverData");
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    //
    // ResourcePrivateDriverDataSize gets passed to CreateAllocation as
    // PrivateDriverDataSize.
    // AllocationPrivateDriverDataSize get passed to CreateAllocation as
    // pAllocationInfo->PrivateDriverDataSize.
    //

    if (!Args->pResourcePrivateDriverData && !Args->pResourcePrivateDriverData)
    {
        Args->ResourcePrivateDriverDataSize = sizeof(RosAllocationGroupExchange);
        Args->AllocationPrivateDriverDataSize = sizeof(RosAllocationExchange);
        return STATUS_SUCCESS;
    }

    // we expect them to both be null or both be valid
    NT_ASSERT(Args->pResourcePrivateDriverData && Args->pResourcePrivateDriverData);
    NT_ASSERT(
        Args->ResourcePrivateDriverDataSize ==
        sizeof(RosAllocationGroupExchange));

    NT_ASSERT(
        Args->AllocationPrivateDriverDataSize ==
        sizeof(RosAllocationExchange));

    new (Args->pResourcePrivateDriverData) RosAllocationGroupExchange();
    //auto allocParams = new (Args->pAllocationPrivateDriverData) RosAllocationExchange();

    switch (Args->StandardAllocationType)
    {
    case D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE:
    {
        
        const D3DKMDT_SHAREDPRIMARYSURFACEDATA* surfData =
                Args->pCreateSharedPrimarySurfaceData;

        debug(
            "Preparing private allocation data for SHAREDPRIMARYSURFACEDATA. (Width=%d, Height=%d, Format=%d, RefreshRate=%d/%d, VidPnSourceId=%d)",
            surfData->Width,
            surfData->Height,
            surfData->Format,
            surfData->RefreshRate.Numerator,
            surfData->RefreshRate.Denominator,
            surfData->VidPnSourceId);
        /*
        allocParams->m_resourceDimension = D3D10DDIRESOURCE_TEXTURE2D;
        allocParams->m_mip0Info.TexelWidth = surfData->Width;
        allocParams->m_mip0Info.TexelHeight = surfData->Height;
        allocParams->m_mip0Info.TexelDepth = 1;
        allocParams->m_mip0Info.PhysicalWidth = surfData->Width;
        allocParams->m_mip0Info.PhysicalHeight = surfData->Height;
        allocParams->m_mip0Info.PhysicalDepth = 1;

        allocParams->m_usage = D3D10_DDI_USAGE_IMMUTABLE;

        // We must ensure that the D3D10_DDI_BIND_PRESENT is set so that
        // CreateAllocation() creates an allocation that is suitable
        // for the primary, which must be flippable.
        // The primary cannot be cached.
        allocParams->m_bindFlags = D3D10_DDI_BIND_RENDER_TARGET | D3D10_DDI_BIND_PRESENT;

        allocParams->m_mapFlags = 0;

        // The shared primary allocation is shared by definition
        allocParams->m_miscFlags = D3D10_DDI_RESOURCE_MISC_SHARED;

        allocParams->m_format = DxgiFormatFromD3dDdiFormat(surfData->Format);
        allocParams->m_sampleDesc.Count = 1;
        allocParams->m_sampleDesc.Quality = 0;
        allocParams->m_mipLevels = 1;
        allocParams->m_arraySize = 1;
        allocParams->m_isPrimary = true;
        allocParams->m_primaryDesc.Flags = 0;
        allocParams->m_primaryDesc.VidPnSourceId = surfData->VidPnSourceId;
        allocParams->m_primaryDesc.ModeDesc.Width = surfData->Width;
        allocParams->m_primaryDesc.ModeDesc.Height = surfData->Height;
        allocParams->m_primaryDesc.ModeDesc.Format = DxgiFormatFromD3dDdiFormat(surfData->Format);
        allocParams->m_primaryDesc.ModeDesc.RefreshRate.Numerator = surfData->RefreshRate.Numerator;
        allocParams->m_primaryDesc.ModeDesc.RefreshRate.Denominator = surfData->RefreshRate.Denominator;
        allocParams->m_primaryDesc.ModeDesc.ScanlineOrdering = DXGI_DDI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        allocParams->m_primaryDesc.ModeDesc.Rotation = DXGI_DDI_MODE_ROTATION_UNSPECIFIED;
        allocParams->m_primaryDesc.ModeDesc.Scaling = DXGI_DDI_MODE_SCALING_UNSPECIFIED;
        allocParams->m_primaryDesc.DriverFlags = 0;

        allocParams->m_hwLayout = RosHwLayout::Linear;
        allocParams->m_hwWidthPixels = surfData->Width;
        allocParams->m_hwHeightPixels = surfData->Height;

        NT_ASSERT(surfData->Format == D3DDDIFMT_A8R8G8B8);
        allocParams->m_hwSizeBytes = surfData->Width * 4 * surfData->Height;
        */
        return STATUS_SUCCESS;
    }
    case D3DKMDT_STANDARDALLOCATION_SHADOWSURFACE:
    {
        const D3DKMDT_SHADOWSURFACEDATA* surfData = Args->pCreateShadowSurfaceData;
        debug(
            "Preparing private allocation data for SHADOWSURFACE. (Width=%d, Height=%d, Format=%d)",
            surfData->Width,
            surfData->Height,
            surfData->Format);
        /*
        allocParams->m_resourceDimension = D3D10DDIRESOURCE_TEXTURE2D;
        allocParams->m_mip0Info.TexelWidth = surfData->Width;
        allocParams->m_mip0Info.TexelHeight = surfData->Height;
        allocParams->m_mip0Info.TexelDepth = 1;
        allocParams->m_mip0Info.PhysicalWidth = surfData->Width;
        allocParams->m_mip0Info.PhysicalHeight = surfData->Height;
        allocParams->m_mip0Info.PhysicalDepth = 1;
        allocParams->m_usage = D3D10_DDI_USAGE_DEFAULT;

        // The shadow allocation does not get flipped directly
        static_assert(
            !(D3D10_DDI_BIND_PIPELINE_MASK & D3D10_DDI_BIND_PRESENT),
            "BIND_PRESENT must not be part of BIND_MASK");
        allocParams->m_bindFlags = D3D10_DDI_BIND_PIPELINE_MASK;

        allocParams->m_mapFlags = D3D10_DDI_MAP_READWRITE;
        allocParams->m_miscFlags = D3D10_DDI_RESOURCE_MISC_SHARED;

        allocParams->m_format = DxgiFormatFromD3dDdiFormat(surfData->Format);
        allocParams->m_sampleDesc.Count = 1;
        allocParams->m_sampleDesc.Quality = 0;
        allocParams->m_mipLevels = 1;
        allocParams->m_arraySize = 1;
        allocParams->m_isPrimary = true;
        allocParams->m_primaryDesc.Flags = 0;
        allocParams->m_primaryDesc.ModeDesc.Width = surfData->Width;
        allocParams->m_primaryDesc.ModeDesc.Height = surfData->Height;
        allocParams->m_primaryDesc.ModeDesc.Format = DxgiFormatFromD3dDdiFormat(surfData->Format);
        allocParams->m_primaryDesc.DriverFlags = 0;
        allocParams->m_hwLayout = RosHwLayout::Linear;
        allocParams->m_hwWidthPixels = surfData->Width;
        allocParams->m_hwHeightPixels = surfData->Height;

        NT_ASSERT(surfData->Format == D3DDDIFMT_A8R8G8B8);
        allocParams->m_hwSizeBytes = surfData->Width * 4 * surfData->Height;
        */
        Args->pCreateShadowSurfaceData->Pitch = surfData->Width * 4; //allocParams->m_hwPitchBytes;
        return STATUS_SUCCESS;
    }
    case D3DKMDT_STANDARDALLOCATION_STAGINGSURFACE:
    {
        const D3DKMDT_STAGINGSURFACEDATA* surfData = Args->pCreateStagingSurfaceData;
        debug(
            "STAGINGSURFACEDATA is not implemented. (Width=%d, Height=%d, Pitch=%d)",
            surfData->Width,
            surfData->Height,
            surfData->Pitch);
        return STATUS_NOT_IMPLEMENTED;
    }
    case D3DKMDT_STANDARDALLOCATION_GDISURFACE:
    {
        const D3DKMDT_GDISURFACEDATA* surfData = Args->pCreateGdiSurfaceData;
        debug(
            "GDISURFACEDATA is not implemented. We must return a nonzero Pitch if allocation is CPU visible. (Width=%d, Height=%d, Format=%d, Type=%d, Flags=0x%x, Pitch=%d)",
            surfData->Width,
            surfData->Height,
            surfData->Format,
            surfData->Type,
            surfData->Flags.Value,
            surfData->Pitch);
        return STATUS_NOT_IMPLEMENTED;
    }
    default:
        debug(
            "Unknown standard allocation type. (StandardAllocationType=%d)",
            Args->StandardAllocationType);
        return STATUS_INVALID_PARAMETER;
    }
}


NTSTATUS RosKmAdapter::SetPalette (const DXGKARG_SETPALETTE* /*SetPalettePtr*/)
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    debug("Not implemented.");
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS RosKmAdapter::SetPointerPosition (
    const DXGKARG_SETPOINTERPOSITION* SetPointerPositionPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(SetPointerPositionPtr); // m_display.SetPointerPosition(SetPointerPositionPtr);
}


NTSTATUS RosKmAdapter::SetPointerShape (
    const DXGKARG_SETPOINTERSHAPE* SetPointerShapePtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(SetPointerShapePtr); // m_display.SetPointerShape(SetPointerShapePtr);
}


NTSTATUS RosKmAdapter::IsSupportedVidPn (
    DXGKARG_ISSUPPORTEDVIDPN* IsSupportedVidPnPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(IsSupportedVidPnPtr); // m_display.IsSupportedVidPn(IsSupportedVidPnPtr);
}


NTSTATUS RosKmAdapter::RecommendFunctionalVidPn (
    const DXGKARG_RECOMMENDFUNCTIONALVIDPN* const RecommendFunctionalVidPnPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(RecommendFunctionalVidPnPtr); // m_display.RecommendFunctionalVidPn(RecommendFunctionalVidPnPtr);
}


NTSTATUS RosKmAdapter::EnumVidPnCofuncModality (
    const DXGKARG_ENUMVIDPNCOFUNCMODALITY* const EnumCofuncModalityPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(EnumCofuncModalityPtr); // m_display.EnumVidPnCofuncModality(EnumCofuncModalityPtr);
}


NTSTATUS RosKmAdapter::SetVidPnSourceVisibility (
    const DXGKARG_SETVIDPNSOURCEVISIBILITY* SetVidPnSourceVisibilityPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(SetVidPnSourceVisibilityPtr); // m_display.SetVidPnSourceVisibility(SetVidPnSourceVisibilityPtr);
}


NTSTATUS RosKmAdapter::CommitVidPn (
    const DXGKARG_COMMITVIDPN* const CommitVidPnPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(CommitVidPnPtr); // m_display.CommitVidPn(CommitVidPnPtr);
}


NTSTATUS RosKmAdapter::UpdateActiveVidPnPresentPath (
    const DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* const UpdateActiveVidPnPresentPathPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(UpdateActiveVidPnPresentPathPtr); // m_display.UpdateActiveVidPnPresentPath(UpdateActiveVidPnPresentPathPtr);
}


NTSTATUS RosKmAdapter::RecommendMonitorModes (
    const DXGKARG_RECOMMENDMONITORMODES* const RecommendMonitorModesPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(RecommendMonitorModesPtr); // m_display.RecommendMonitorModes(RecommendMonitorModesPtr);
}


NTSTATUS RosKmAdapter::GetScanLine (DXGKARG_GETSCANLINE* GetScanLinePtr)
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    debug("Not implemented");
    return STATUS_NOT_IMPLEMENTED; UNREFERENCED_PARAMETER(GetScanLinePtr);
}


NTSTATUS RosKmAdapter::ControlInterrupt (
    const DXGK_INTERRUPT_TYPE InterruptType,
    BOOLEAN EnableInterrupt
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(InterruptType); UNREFERENCED_PARAMETER(EnableInterrupt); // m_display.ControlInterrupt(InterruptType, EnableInterrupt);
}


NTSTATUS RosKmAdapter::QueryVidPnHWCapability (
    DXGKARG_QUERYVIDPNHWCAPABILITY* VidPnHWCapsPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(VidPnHWCapsPtr); // m_display.QueryVidPnHWCapability(VidPnHWCapsPtr);
}


NTSTATUS RosKmAdapter::QueryDependentEngineGroup (
    DXGKARG_QUERYDEPENDENTENGINEGROUP* ArgsPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(ArgsPtr->NodeOrdinal == 0);
    NT_ASSERT(ArgsPtr->EngineOrdinal == 0);

    ArgsPtr->DependentNodeOrdinalMask = 0;
    return STATUS_SUCCESS;
}


NTSTATUS RosKmAdapter::StopDeviceAndReleasePostDisplayOwnership (
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    DXGK_DISPLAY_INFORMATION* DisplayInfoPtr
    )
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    NT_ASSERT(!RosKmdGlobal::IsRenderOnly());
    return STATUS_SUCCESS; UNREFERENCED_PARAMETER(TargetId); UNREFERENCED_PARAMETER(DisplayInfoPtr); // m_display.StopDeviceAndReleasePostDisplayOwnership(TargetId, DisplayInfoPtr);
}

 //=====================================================
