#pragma once
#include "common.h"

class DxrContext
{
public:
	DxrContext(uint initWidth, uint initHeight);
	~DxrContext();
	void Flush();
private:
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGISwapChain4> swapChain;

	ComPtr<ID3D12Device10> device;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12StateObject> pso;
	ComPtr<ID3D12Fence> fence;

	ComPtr<ID3D12CommandQueue> commandQueue; //TODO: two queues for doing frame n+1's rtas build while frame n's raytracing (more memory usage? :\)
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList4> commandList;

};
