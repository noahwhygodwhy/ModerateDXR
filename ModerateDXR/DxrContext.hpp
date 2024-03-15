#pragma once
#include "common.h"



struct HitGroupData
{
	wstring hgName;
	wstring chName;
	D3D12_HIT_GROUP_DESC desc;
	D3D12_STATE_SUBOBJECT so;
	HitGroupData(wstring name)
	{
		hgName = L"hg_" + name;
		chName = L"ch_" + name;
		desc = { .HitGroupExport = hgName.data(), .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES, .ClosestHitShaderImport = chName.data()};
		so = { .Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &desc };
	}
};

class DxrContext
{
public:
	DxrContext(HWND hwnd, uint initWidth, uint initHeight);
	~DxrContext();
	void* MapConstantBuffer();
	void Resize(uint initWidth, uint initHeight);
	void CreateScreenSizedResources();
	void BuildTlas();
	void DispatchRays();
	void CopyFramebuffer();
	void Present();
	void ResetCommandList();
	void ExecuteCommandList();
	void Flush();
	void Render(float ct);
	void CreateTlasResources(vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs);
	ComPtr<ID3D12Device10> device;
	ComPtr<ID3D12GraphicsCommandList4> commandList;
	ComPtr<ID3D12DescriptorHeap> descHeap;
private:
	uint32_t frameNumber = 0;
	uint32_t screenWidth;
	uint32_t screenHeight;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInput;

	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGISwapChain4> swapChain;

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12StateObject> pso;
	ComPtr<ID3D12Fence> fence;

	ComPtr<ID3D12CommandQueue> commandQueue; //TODO: two queues for doing frame n+1's rtas build while frame n's raytracing (more memory usage? :\)
	ComPtr<ID3D12CommandAllocator> commandAllocator;

	ComPtr<ID3D12Resource> constantBuffer;
	ComPtr<ID3D12Resource> shaderBuffer;
	ComPtr<ID3D12Resource> instanceBuffer;
	ComPtr<ID3D12Resource> framebuffer;
	ComPtr<ID3D12Resource> backbuffer;

	ComPtr<ID3D12Resource> tlas;
	ComPtr<ID3D12Resource> tlasScratch;

	void BuildRootSignature();
	void BuildPipelineStateObject();
};
