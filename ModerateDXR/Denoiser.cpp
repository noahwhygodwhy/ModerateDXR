#include "common.h"
#include "Denoiser.hpp"
#include "denoise.h"
#include "shared.h"

void Denoiser::BuildRootSignature(ComPtr<ID3D12Device> device)
{
    CD3DX12_DESCRIPTOR_RANGE1 descRanges[3];
    descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, /*u*/0, 0); //u0 colorbuffer
    descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, /*u*/1, 0); //u1 framebuffer
    descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, /*t*/0, 0); //t hitinfo

    CD3DX12_ROOT_PARAMETER1 rootParams[1];
    //rootParams[0].InitAsConstantBufferView(0, 0);  // b0 | constant buffer
    rootParams[0].InitAsDescriptorTable(_countof(descRanges), descRanges); //

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSigDesc(_countof(rootParams), rootParams);

    ID3DBlob* pSerializedRootSig;
    TIF(D3D12SerializeVersionedRootSignature(&RootSigDesc, &pSerializedRootSig, nullptr));
    TIF(device->CreateRootSignature(0, pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    rootSignature->SetName(L"denoiserrootsig");
}
void Denoiser::BuildPipelineStateObject(ComPtr<ID3D12Device> device)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDsec = D3D12_COMPUTE_PIPELINE_STATE_DESC{
		.pRootSignature = this->rootSignature.Get(),
		.CS = D3D12_SHADER_BYTECODE{.pShaderBytecode = compiledDenoise, .BytecodeLength = std::size(compiledDenoise)},
		.NodeMask = 0,
		.CachedPSO = {.pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0},
		.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
	};
    //ComPtr<ID3D12PipelineState> pipelineState;
	TIF(device->CreateComputePipelineState(&pipelineDsec, IID_PPV_ARGS(&pso)));

    pso->SetName(L"denoiserpso");
}

void Denoiser::BuildDescHeap(ComPtr<ID3D12Device> device)
{
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = uint(DescHeapIds::Count),
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    };
    TIF(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap)));
}

Denoiser::Denoiser(ComPtr<ID3D12Device> device)
{
    this->BuildRootSignature(device);
    this->BuildPipelineStateObject(device);
    this->BuildDescHeap(device);
}

void Denoiser::SetScreenSizedResources(ComPtr<ID3D12Device> device, ComPtr<ID3D12Resource> source, ComPtr<ID3D12Resource> dest, ComPtr<ID3D12Resource> hitInfo)
{
    uint sizeOfResourceDesc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //D3D12_RESOURCE_DESC sourceDesc = source->GetDesc();
    //D3D12_SHADER_RESOURCE_VIEW_DESC sourceViewDesc{
    //    .Format = sourceDesc.Format,
    //    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
    //    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    //    .Texture2D = {0, 1} };
    //D3D12_CPU_DESCRIPTOR_HANDLE sourceDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), uint(DescHeapIds::SourceFB), sizeOfResourceDesc);
    //device->CreateShaderResourceView(source.Get(), &sourceViewDesc, sourceDescHandle);


    D3D12_RESOURCE_DESC sourceDesc = dest->GetDesc();
    D3D12_UNORDERED_ACCESS_VIEW_DESC sourceViewDesc{ .Format = sourceDesc.Format, .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D, .Texture2D = {0, 0} };
    D3D12_CPU_DESCRIPTOR_HANDLE sourceDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), uint(DescHeapIds::DestFB), sizeOfResourceDesc);
    device->CreateUnorderedAccessView(dest.Get(), nullptr, &sourceViewDesc, sourceDescHandle);



    D3D12_RESOURCE_DESC hitInfoDesc = hitInfo->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC hitInfoViewDesc{
        .Format = hitInfoDesc.Format,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer ={.FirstElement = 0, .NumElements = uint32_t(hitInfoDesc.Width / sizeof(HitInfo)), .StructureByteStride = sizeof(HitInfo), .Flags = D3D12_BUFFER_SRV_FLAG_NONE} };

    D3D12_CPU_DESCRIPTOR_HANDLE hitInfoDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), (uint)DescHeapIds::HitInfo, sizeOfResourceDesc);
    device->CreateShaderResourceView(hitInfo.Get(), &hitInfoViewDesc, hitInfoDescHandle);




    D3D12_RESOURCE_DESC destDesc = dest->GetDesc();
    D3D12_UNORDERED_ACCESS_VIEW_DESC destViewDesc{ .Format = destDesc.Format, .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D, .Texture2D = {0, 0} };
    D3D12_CPU_DESCRIPTOR_HANDLE destDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), uint(DescHeapIds::DestFB), sizeOfResourceDesc);
    device->CreateUnorderedAccessView(dest.Get(), nullptr, &destViewDesc, destDescHandle);

    this->width = (uint)sourceDesc.Width;
    this->height = (uint)sourceDesc.Height;

    this->pre_barrier = CD3DX12_RESOURCE_BARRIER::UAV(source.Get());
    this->post_barrier = CD3DX12_RESOURCE_BARRIER::UAV(dest.Get());
}

void Denoiser::Denoise(ComPtr<ID3D12GraphicsCommandList4> commandList)
{

    commandList->ResourceBarrier(1, &this->pre_barrier);
    commandList->SetPipelineState(pso.Get());
    //commandList->SetPipelineState1(pso.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());
    commandList->SetDescriptorHeaps(1, descHeap.GetAddressOf());
    D3D12_GPU_DESCRIPTOR_HANDLE descHandle = descHeap->GetGPUDescriptorHandleForHeapStart();

    commandList->SetComputeRootDescriptorTable(0, descHandle);

    uvec2 wh = uvec2(glm::ceil(fvec2(this->width, this->height)/8.0f));


    commandList->Dispatch(wh.x, wh.y, 1);
    commandList->ResourceBarrier(1, &this->post_barrier);
}

Denoiser::~Denoiser()
{
}