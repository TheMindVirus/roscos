#include "RosKmd.h"

NTSTATUS APIENTRY RosKmDevice::DdiCreateDevice(
    IN_CONST_HANDLE hAdapter,
    INOUT_PDXGKARG_CREATEDEVICE pCreateDevice)
{
    pCreateDevice->hDevice = new (NonPagedPoolNx, ROS_ALLOC_TAG::DEVICE)
        RosKmDevice(hAdapter, pCreateDevice);
    if (!pCreateDevice->hDevice)
    {
        debug("Failed to allocate RosKmDevice.");
        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}

NTSTATUS  RosKmDevice::DdiDestroyDevice(
    IN_CONST_HANDLE     hDevice)
{
    RosKmDevice   *pRosKmDevice = (RosKmDevice *)hDevice;
    UNREFERENCED_PARAMETER(pRosKmDevice);
    //delete pRosKmDevice;

    return STATUS_SUCCESS;
}

RosKmDevice::RosKmDevice(IN_CONST_HANDLE hAdapter, INOUT_PDXGKARG_CREATEDEVICE pCreateDevice)
{
    m_hRTDevice = pCreateDevice->hDevice;
    m_pRosKmAdapter = (RosKmAdapter *)hAdapter;
    m_Flags = pCreateDevice->Flags;
    
    pCreateDevice->hDevice = this;
}

RosKmDevice::~RosKmDevice()
{
    // do nothing
}

NTSTATUS APIENTRY RosKmDevice::DdiCloseAllocation(
    IN_CONST_HANDLE                     hDevice,
    IN_CONST_PDXGKARG_CLOSEALLOCATION   pCloseAllocation)
{
    debug("%s hDevice=%p\n", __FUNCTION__, hDevice);

    RosKmDevice   *pRosKmDevice = (RosKmDevice *)hDevice;
    pRosKmDevice;

    NT_ASSERT(pCloseAllocation->NumAllocations == 1);

    RosKmdDeviceAllocation * pRosKmdDeviceAllocation = (RosKmdDeviceAllocation *)pCloseAllocation->pOpenHandleList[0];

    ExFreePoolWithTag(pRosKmdDeviceAllocation, ROS_ALLOC_TAG::DEVICE);

    return STATUS_SUCCESS;
}

 //===================================================


NTSTATUS RosKmDevice::OpenAllocation (const DXGKARG_OPENALLOCATION* ArgsPtr)
{
    
    ROS_ASSERT_MAX_IRQL(PASSIVE_LEVEL);

    const DXGKRNL_INTERFACE& dxgkInterface = *m_pRosKmAdapter->GetDxgkInterface();

    for (UINT i = 0; i < ArgsPtr->NumAllocations; i++)
    {
        DXGK_OPENALLOCATIONINFO* openAllocInfoPtr = ArgsPtr->pOpenAllocation + i;

        RosKmdAllocation* rosKmdAllocationPtr;
        {
            DXGKARGCB_GETHANDLEDATA getHandleData;
            getHandleData.hObject = openAllocInfoPtr->hAllocation;
            getHandleData.Type = DXGK_HANDLE_ALLOCATION;
            getHandleData.Flags.DeviceSpecific = 0;
            rosKmdAllocationPtr = static_cast<RosKmdAllocation*>(
                dxgkInterface.DxgkCbGetHandleData(&getHandleData));
        }

        RosKmdDeviceAllocation* rosKmdDeviceAllocationPtr;
        {
            // TODO[jordanrh] this structure can probably be paged
            rosKmdDeviceAllocationPtr = new (NonPagedPoolNx, ROS_ALLOC_TAG::DEVICE)
                    RosKmdDeviceAllocation();
            if (!rosKmdDeviceAllocationPtr)
            {
                debug(
                    "Failed to allocate memory for RosKmdDeviceAllocation structure. (sizeof(RosKmdDeviceAllocation)=%lu)",
                    sizeof(RosKmdDeviceAllocation));
                return STATUS_NO_MEMORY;
            }

            rosKmdDeviceAllocationPtr->m_hKMAllocation = openAllocInfoPtr->hAllocation;
            rosKmdDeviceAllocationPtr->m_pRosKmdAllocation = rosKmdAllocationPtr;
        }

        // Return the per process allocation info
        openAllocInfoPtr->hDeviceSpecificAllocation = rosKmdDeviceAllocationPtr;
    }

    debug(
        "Successfully opened allocation. (Flags.Create=%d, Flags.ReadOnly=%d)",
        ArgsPtr->Flags.Create,
        ArgsPtr->Flags.ReadOnly);

    return STATUS_SUCCESS;
}

 //=====================================================