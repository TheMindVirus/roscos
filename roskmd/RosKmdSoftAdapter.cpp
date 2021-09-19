#include "Ros.h"

void * RosKmdSoftAdapter::operator new(size_t size)
{
    debug("[CALL]: %s", __FUNCTION__);
    return ExAllocatePoolWithTag(NonPagedPoolNx, size, 'ROSD');
}

void RosKmdSoftAdapter::operator delete(void * ptr)
{
    debug("[CALL]: %s", __FUNCTION__);
    ExFreePool(ptr);
}

NTSTATUS
RosKmdSoftAdapter::Start(
    IN_PDXGK_START_INFO     DxgkStartInfo,
    IN_PDXGKRNL_INTERFACE   DxgkInterface,
    OUT_PULONG              NumberOfVideoPresentSources,
    OUT_PULONG              NumberOfChildren)
{
    debug("[CALL]: %s", __FUNCTION__);
    return RosKmAdapter::Start(DxgkStartInfo, DxgkInterface, NumberOfVideoPresentSources, NumberOfChildren);
}

void
RosKmdSoftAdapter::ProcessRenderBuffer(
    ROSDMABUFSUBMISSION * pDmaBufSubmission)
{
    debug("[CALL]: %s", __FUNCTION__);
    ROSDMABUFINFO * pDmaBufInfo = pDmaBufSubmission->m_pDmaBufInfo;

    NT_ASSERT(pDmaBufInfo->m_DmaBufState.m_bSwCommandBuffer);

    NT_ASSERT(0 == (pDmaBufSubmission->m_EndOffset - pDmaBufSubmission->m_StartOffset) % sizeof(GpuCommand));

    GpuCommand * pGpuCommand = (GpuCommand *)(pDmaBufInfo->m_pDmaBuffer + pDmaBufSubmission->m_StartOffset);
    GpuCommand * pEndofCommand = (GpuCommand *)(pDmaBufInfo->m_pDmaBuffer + pDmaBufSubmission->m_EndOffset);

    for (; pGpuCommand < pEndofCommand; pGpuCommand++)
    {
        switch (pGpuCommand->m_commandId)
        {
        case Header:
        case Nop:
            break;
        case ResourceCopy:
        {
            RtlCopyMemory(
                ((BYTE *)RosKmdGlobal::s_pVideoMemory) + pGpuCommand->m_resourceCopy.m_dstGpuAddress.QuadPart,
                ((BYTE *)RosKmdGlobal::s_pVideoMemory) + pGpuCommand->m_resourceCopy.m_srcGpuAddress.QuadPart,
                pGpuCommand->m_resourceCopy.m_sizeBytes);
        }
        break;
        default:
            break;
        }
    }
}

BOOLEAN RosKmdSoftAdapter::InterruptRoutine(
    IN_ULONG        MessageNumber)
{
    MessageNumber;

    return false;
}

