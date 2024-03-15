#include "Raytracable.hpp"

void Raytracable::ReleaseScratch() {
	this->blasScratch->Release();
}



void Raytracable::AddGeomSRV(ComPtr<ID3D12Device10> device, ComPtr<ID3D12DescriptorHeap> descHeap)
{
    static uint nextDescHeapIndex = 1;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
    {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = this->getGeomBufferInfo()
    };

    uint descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapCpuBase = descHeap->GetCPUDescriptorHandleForHeapStart();
    //uint nextDescHeapIndex = descHeap->GetDesc().NumDescriptors;

    if (nextDescHeapIndex >= MAX_NUM_DESCRIPTORS)
    {
        TIE(E_FAIL, "out of descriptor heap space rip");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handleAtIndex = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, nextDescHeapIndex, descriptorSize);
    device->CreateShaderResourceView(this->geomBuffer.Get(), &srvDesc, handleAtIndex);
    this->heapDescIndex = nextDescHeapIndex;
    nextDescHeapIndex++;
}


D3D12_GPU_VIRTUAL_ADDRESS Raytracable::getBlasAddress() const
{
    return this->blas->GetGPUVirtualAddress();
}