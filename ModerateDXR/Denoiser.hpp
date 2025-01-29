#pragma once


class Denoiser
{
public:
	Denoiser(ComPtr<ID3D12Device> device);
	~Denoiser();
	void BuildRootSignature(ComPtr<ID3D12Device> device);
	void BuildPipelineStateObject(ComPtr<ID3D12Device> device);
	void BuildDescHeap(ComPtr<ID3D12Device> device);
	void SetScreenSizedResources(ComPtr<ID3D12Device> device, ComPtr<ID3D12Resource> source, ComPtr<ID3D12Resource> dest, ComPtr<ID3D12Resource> hitInfo);
	void Denoise(ComPtr<ID3D12GraphicsCommandList4> commandList);
private:
	uint width;
	uint height;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12DescriptorHeap> descHeap;


	D3D12_RESOURCE_BARRIER pre_barrier;
	D3D12_RESOURCE_BARRIER post_barrier;

	enum class DescHeapIds : uint
	{
		SourceFB,
		DestFB,
		HitInfo,//TODO:
		Count
	};
};