#pragma once
#include "common.h"

class Raytracable 
{
protected:
	ComPtr<ID3D12Resource> geomBuffer;
	ComPtr<ID3D12Resource> blas;
	ComPtr<ID3D12Resource> blasScratch;
public:
	uint heapDescIndex = 0;
	Raytracable() {};
	~Raytracable() {};
	virtual D3D12_RESOURCE_BARRIER BuildBLAS(ComPtr<ID3D12Device10> device, ComPtr<ID3D12GraphicsCommandList4> commandList) = 0;
	void ReleaseScratch();
	void AddGeomSRV(ComPtr<ID3D12Device10> device, ComPtr<ID3D12DescriptorHeap> descHeap);
	virtual D3D12_BUFFER_SRV getGeomBufferInfo() = 0;
	D3D12_GPU_VIRTUAL_ADDRESS getBlasAddress() const;
	virtual void CreateResources(ComPtr<ID3D12Device10> device) = 0;
private:

};
