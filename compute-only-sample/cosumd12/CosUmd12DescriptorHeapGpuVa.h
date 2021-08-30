#pragma once

#include "CosUmd12.h"

class CosUmd12Device;

class CosUmd12DescriptorHeap
{
public:
    static HRESULT
    Create(
        CosUmd12Device *    pDevice,
        _In_ const D3D12DDIARG_CREATE_DESCRIPTOR_HEAP_0001 *    pDesc,
        D3D12DDI_HDESCRIPTORHEAP DescriptorHeap);

    explicit
    CosUmd12DescriptorHeap(
        CosUmd12Device * pDevice,
        const D3D12DDIARG_CREATE_DESCRIPTOR_HEAP_0001 * pDesc,
        const D3D12DDIARG_CREATEHEAP_0001 * pHeapDesc) :
        m_hwDescriptorHeap(pDevice, MAKE_D3D10DDI_HRTRESOURCE(NULL), pHeapDesc)
    {
        m_pDevice = pDevice;
        m_desc = *pDesc;
    }

    HRESULT Standup();
    void Teardown();

    static int CalculateSize(const D3D12DDIARG_CREATE_DESCRIPTOR_HEAP_0001 * pArgs);

    static CosUmd12DescriptorHeap * CastFrom(D3D12DDI_HDESCRIPTORHEAP);
    D3D12DDI_HDESCRIPTORHEAP CastTo() const;

    D3D12DDI_GPU_VIRTUAL_ADDRESS GetGpuAddress() const
    {
        return m_hwDescriptorHeap.GetGpuVa();
    }

    CosUmd12Descriptor * GetCpuAddress() const
    {
        return m_pHwDescriptors;
    }

    D3D12DDI_DESCRIPTOR_HEAP_TYPE GetHeapType() const
    {
        return m_desc.Type;
    }

private:

    friend class CosUmd12RootSignature;
    friend class CosUmd12CommandList;

    CosUmd12Device * m_pDevice;
    D3D12DDIARG_CREATE_DESCRIPTOR_HEAP_0001 m_desc;

    CosUmd12Heap m_hwDescriptorHeap;
    CosUmd12Descriptor * m_pHwDescriptors;

    D3D12DDI_GPU_VIRTUAL_ADDRESS m_heapUniqueAddress;
};

inline CosUmd12DescriptorHeap * CosUmd12DescriptorHeap::CastFrom(D3D12DDI_HDESCRIPTORHEAP hDescriptorHeap)
{
    return static_cast<CosUmd12DescriptorHeap *>(hDescriptorHeap.pDrvPrivate);
}

inline D3D12DDI_HDESCRIPTORHEAP CosUmd12DescriptorHeap::CastTo() const
{
    return MAKE_D3D12DDI_HDESCRIPTORHEAP(const_cast<CosUmd12DescriptorHeap *>(this));
}

