#include "DxrContext.hpp"
#include "shared.h"
#include "denoise.h"
#include "raytrace.h"


void DxrContext::CreateScreenSizedResources()
{
//FRAMEBUFFER=============================================================
    D3D12_HEAP_PROPERTIES fbProps{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    D3D12_RESOURCE_DESC fbDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = this->screenWidth,
        .Height = this->screenHeight,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };

    TIF(this->device->CreateCommittedResource(
        &fbProps,
        D3D12_HEAP_FLAG_NONE,
        &fbDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&framebuffer)));

    framebuffer->SetName(L"framebuffer");

//COLORBUFFER=============================================================
    D3D12_HEAP_PROPERTIES colorBufferProps{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };


    D3D12_RESOURCE_DESC colorBufferDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = this->screenWidth,
        .Height = this->screenHeight,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };

    TIF(this->device->CreateCommittedResource(
        &colorBufferProps,
        D3D12_HEAP_FLAG_NONE,
        &colorBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&colorBuffer)));

    framebuffer->SetName(L"colorBuffer");

    D3D12_UNORDERED_ACCESS_VIEW_DESC colorBufferViewDesc = {
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D = {0, 0}
    };

    D3D12_CPU_DESCRIPTOR_HANDLE colorBufferDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), (uint)DescHeapIds::Color, sizeOfResourceDesc);
    device->CreateUnorderedAccessView(colorBuffer.Get(), nullptr, &colorBufferViewDesc, colorBufferDescHandle);

//hitInfo=============================================================
    D3D12_HEAP_PROPERTIES hitInfoProps{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    D3D12_RESOURCE_DESC hitInfoDesc = CD3DX12_RESOURCE_DESC::Buffer(this->screenWidth * this->screenHeight * sizeof(HitInfo), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    TIF(this->device->CreateCommittedResource(
        &hitInfoProps,
        D3D12_HEAP_FLAG_NONE,
        &hitInfoDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&hitInfoBuffer)));

    framebuffer->SetName(L"hitinfobuffer");

    D3D12_UNORDERED_ACCESS_VIEW_DESC hitInfoViewDesc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
        .Buffer = {.FirstElement = 0, .NumElements = this->screenWidth * this->screenHeight, .StructureByteStride = sizeof(HitInfo), .Flags = D3D12_BUFFER_UAV_FLAG_NONE},
    };

    D3D12_CPU_DESCRIPTOR_HANDLE hitInfoDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), (uint)DescHeapIds::HitInfo, sizeOfResourceDesc);
    device->CreateUnorderedAccessView(hitInfoBuffer.Get(), nullptr, &hitInfoViewDesc, hitInfoDescHandle);



//RANDBUFFER=============================================================

    D3D12_HEAP_PROPERTIES randBufferProps{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    D3D12_RESOURCE_DESC randBufferDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = this->screenWidth,
        .Height = this->screenHeight,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R32G32_UINT,
        .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    };

    TIF(this->device->CreateCommittedResource(
        &randBufferProps,
        D3D12_HEAP_FLAG_NONE,
        &randBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&randBuffer)));

    randBuffer->SetName(L"randBuffer");

    D3D12_UNORDERED_ACCESS_VIEW_DESC randbufferViewDesc = {
        .Format = DXGI_FORMAT_R32G32_UINT,
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D = {0, 0}
    };

    D3D12_CPU_DESCRIPTOR_HANDLE randbufferDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), (uint)DescHeapIds::Rand, sizeOfResourceDesc);
    device->CreateUnorderedAccessView(randBuffer.Get(), nullptr, &randbufferViewDesc, randbufferDescHandle);

    D3D12_RESOURCE_BARRIER setupBarriers[3] = {
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = colorBuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COMMON, .StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS}},
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = framebuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COMMON, .StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS}},
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = hitInfoBuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COMMON, .StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS}}

    };
    commandList->ResourceBarrier(_countof(setupBarriers), setupBarriers);


    this->denoiser->SetScreenSizedResources(this->device, this->colorBuffer, this->framebuffer, this->hitInfoBuffer);

}

void DxrContext::DoResize(float ct)
{
    //if(!this->backbuffer) {return;}
    //Flush();
    //OutputDebugStringF("do resize\n %u, %u\n", this->newWidth, this->newHeight);
    //this->screenHeight = newHeight;
    //this->screenWidth = newWidth;
    //this->backbuffer.Reset();
    //swapChain->ResizeBuffers(2, this->screenWidth, this->screenHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
    //this->CreateScreenSizedResources();
    //this->PopulateAndCopyRandBuffer();//TODO: double check this is correct here
    //this->needResize = false;
    
}
void DxrContext::SetResize(uint width, uint height)
{
    //OutputDebugStringF("set resize\n %u, %u\n", width, height);
    //this->newWidth = width;
    //this->newHeight = height;
    //this->needResize = true;
}

void DxrContext::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE1 descRanges[4]; ////////////////////what
    descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, /*u*/0, 0); //u0 colorbuffer
    descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, /*u*/1, 0); //u1 randbuffer
    descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, /*u*/2, 0); //u2 objectIDbuffer
    descRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, -1, /*t*/1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); //t1 | geombuffer[]


    CD3DX12_ROOT_PARAMETER1 rootParams[3];
    rootParams[0].InitAsConstantBufferView(0, 0);  // b0 | constant buffer
    rootParams[1].InitAsShaderResourceView(0, 0);  // t0 | tlas
    rootParams[2].InitAsDescriptorTable(_countof(descRanges), descRanges); //


    
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSigDesc(_countof(rootParams), rootParams);

    ID3DBlob* pSerializedRootSig;
    TIF(D3D12SerializeVersionedRootSignature(&RootSigDesc, &pSerializedRootSig, nullptr));
    TIF(device->CreateRootSignature(0, pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void DxrContext::BuildPipelineStateObject()
{
    D3D12_DXIL_LIBRARY_DESC libraryDesc = { .DXILLibrary = {.pShaderBytecode = compiledRaytrace,.BytecodeLength = std::size(compiledRaytrace)} };
    D3D12_STATE_SUBOBJECT librarySO = { .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &libraryDesc };

    D3D12_RAYTRACING_SHADER_CONFIG shadercfgDesc = { .MaxPayloadSizeInBytes = sizeof(RayPayload), .MaxAttributeSizeInBytes = 8 }; //8 here is the 2 float barycentric for the default attribute struct for triangle hits
    D3D12_STATE_SUBOBJECT shadercfgSO = { .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &shadercfgDesc };

    D3D12_GLOBAL_ROOT_SIGNATURE rootsigDesc = { .pGlobalRootSignature = rootSignature.Get() };
    D3D12_STATE_SUBOBJECT rootsigSO = { .Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &rootsigDesc };

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelinecfgDesc = { .MaxTraceRecursionDepth = MAX_RECURSION_LAYERS };
    D3D12_STATE_SUBOBJECT pipelinecfgSO = { .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &pipelinecfgDesc };

    vector<D3D12_STATE_SUBOBJECT> subobjects = { librarySO, shadercfgSO, rootsigSO, pipelinecfgSO };
    for (uint i = 0; i < _countof(hgdata); i++)
    {
        subobjects.push_back(hgdata[i].so);
    }
    //D3D12_HIT_GROUP_DESC hitgroupDesc = { .HitGroupExport = L"hg_normalcolor", .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES, .ClosestHitShaderImport = L"ch_normalcolors" };
    //D3D12_STATE_SUBOBJECT hitgroupSO = { .Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &hitgroupDesc };

    D3D12_STATE_OBJECT_DESC stateObjectDesc{ .Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE, .NumSubobjects = (uint) subobjects.size(), .pSubobjects = subobjects.data()};

    TIF(device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&pso)));


    constexpr uint32_t numShaders = 2+_countof(hgdata);

    CD3DX12_HEAP_PROPERTIES shaderBuffProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC shaderBuffDesc = CD3DX12_RESOURCE_DESC::Buffer(numShaders * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    TIF(device->CreateCommittedResource(&shaderBuffProps, D3D12_HEAP_FLAG_NONE, &shaderBuffDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&shaderBuffer)));
    shaderBuffer->SetName(L"shaderBuffer");

    ComPtr<ID3D12StateObjectProperties> stateprops;
    pso->QueryInterface(IID_PPV_ARGS(&stateprops));

    //TODO: do this better/handle more shaders
    char* uploadPointer;
    shaderBuffer->Map(0, nullptr, (void**) & uploadPointer);
    
    memcpy(uploadPointer, stateprops->GetShaderIdentifier(L"RayGeneration"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    uploadPointer += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    memcpy(uploadPointer, stateprops->GetShaderIdentifier(L"Miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    uploadPointer += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

    for (uint i = 0; i < _countof(hgdata); i++)
    {
        memcpy(uploadPointer, stateprops->GetShaderIdentifier(hgdata[i].hgName.data()), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        uploadPointer += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; //D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    }

    shaderBuffer->Unmap(0, nullptr);
}


DxrContext::DxrContext(HWND hwnd, uint initWidth, uint initHeight)
{
    this->screenWidth = initWidth;
    this->screenHeight = initHeight;
    this->newWidth = initWidth;
    this->newHeight = initHeight;
    ComPtr<IDXGIFactory4> factory4;
    TIF(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory4)));
    TIF(factory4->QueryInterface(IID_PPV_ARGS(&factory)));
    factory4->Release();
    TIF(this->factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
    TIF(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device)));
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = 0,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0 };
    TIF(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));

    ComPtr<IDXGISwapChain1> swapChain1;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = DXGI_SWAP_CHAIN_DESC1 {
        .Width = this->screenWidth,
        .Height = this->screenHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .Stereo = false,
        .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = 0,
    };

    TIF(factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    TIF(swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain)));
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = MAX_NUM_DESCRIPTORS,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    };
    TIF(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap)));

    this->sizeOfResourceDesc = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    this->BuildRootSignature();
    this->BuildPipelineStateObject();

    TIF(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    TIF(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    TIF(commandList->Close());

    TIF(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));


    CD3DX12_HEAP_PROPERTIES cbProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBufferStruct));
    TIF(device->CreateCommittedResource(&cbProps, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&constantBuffer)));
    constantBuffer->SetName(L"constantBuffer");
    constantBuffer->Map(0, nullptr, (void**)&this->constants);

    this->denoiser = new Denoiser(this->device);


}


void* DxrContext::MapConstantBuffer()
{
    void* cb;
    constantBuffer->Map(0, nullptr, (void**)&cb);
    return cb;

}

DxrContext::~DxrContext()
{
}

void DxrContext::UploadInstanceDescs()
{
    //TODO: check the list hasn't changed in size?

    void* uploadPointer;
    instanceBuffer->Map(0, nullptr, &uploadPointer);
    memcpy(uploadPointer, instanceDescs.data(), instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    instanceBuffer->Unmap(0, nullptr);
}

void DxrContext::CreateTlasResources(vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs)
{

    this->instanceDescs = instanceDescs;
    CD3DX12_HEAP_PROPERTIES instanceProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC instanceDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    TIF(device->CreateCommittedResource(&instanceProps, D3D12_HEAP_FLAG_NONE, &instanceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&instanceBuffer)));
    instanceBuffer->SetName(L"instancebuffer");

    this->tlasInput = {
        .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
        .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
        .NumDescs = uint32_t(instanceDescs.size()),
        .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
        .InstanceDescs = this->instanceBuffer->GetGPUVirtualAddress()
    };

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
    device->GetRaytracingAccelerationStructurePrebuildInfo(&this->tlasInput, &prebuildInfo);



    //TODO: only recreate if the size changed / got significantly smaller
    //SAFE_RELEASE(this->tlas);
    //SAFE_RELEASE(this->tlasScratch);    

    CD3DX12_HEAP_PROPERTIES tlasProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC tlasScratchDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_RESOURCE_DESC tlasResultDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    TIF(device->CreateCommittedResource(&tlasProps, D3D12_HEAP_FLAG_NONE, &tlasScratchDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&tlasScratch)));
    tlasScratch->SetName(L"tlasScratch");
    TIF(device->CreateCommittedResource(&tlasProps, D3D12_HEAP_FLAG_NONE, &tlasResultDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&tlas)));
    tlas->SetName(L"tlas");
}

void DxrContext::BuildTlas()
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc =
    {
        .DestAccelerationStructureData = this->tlas->GetGPUVirtualAddress(),
        .Inputs = this->tlasInput,
        .SourceAccelerationStructureData = 0,
        .ScratchAccelerationStructureData = this->tlasScratch->GetGPUVirtualAddress()
    };
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    D3D12_RESOURCE_BARRIER tlasBarrier = D3D12_RESOURCE_BARRIER{ .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .UAV = {.pResource = this->tlas.Get()} };

    commandList->ResourceBarrier(1, &tlasBarrier);
}

void DxrContext::ResetCommandList()
{
    this->commandAllocator->Reset();
    this->commandList->Reset(this->commandAllocator.Get(), nullptr);
}

void DxrContext::ExecuteCommandList()
{
    TIF(commandList->Close());
    commandQueue->ExecuteCommandLists(1, CommandListCast(this->commandList.GetAddressOf()));
}

void DxrContext::DispatchRays()
{
    commandList->SetPipelineState1(pso.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());
    commandList->SetDescriptorHeaps(1, descHeap.GetAddressOf());
    D3D12_GPU_DESCRIPTOR_HANDLE descHandle = descHeap->GetGPUDescriptorHandleForHeapStart();
    commandList->SetComputeRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());//TODO: need a constant buffer
    commandList->SetComputeRootShaderResourceView(1, tlas->GetGPUVirtualAddress());
    commandList->SetComputeRootDescriptorTable(2, descHandle);

    //TODO: need a shader library resource
    D3D12_DISPATCH_RAYS_DESC rayDesc = {
        .RayGenerationShaderRecord = {.StartAddress = shaderBuffer->GetGPUVirtualAddress(), .SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
        .MissShaderTable = {.StartAddress = shaderBuffer->GetGPUVirtualAddress() + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
        .HitGroupTable = {.StartAddress = shaderBuffer->GetGPUVirtualAddress() + 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,.SizeInBytes = (D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * _countof(hgdata)), .StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
        .Width = this->screenWidth,
        .Height = this->screenHeight,
        .Depth = 1
    };
    commandList->DispatchRays(&rayDesc);
    
}

uint RoundUp(uint val, uint toRoundTo)
{
    if (val % toRoundTo == 0) return val;

    return ((val / toRoundTo) + 1) * toRoundTo;
}

void DxrContext::PopulateAndCopyRandBuffer()
{
    if (randUploadBuffer) { randUploadBuffer.Reset(); }

    CD3DX12_RESOURCE_DESC randUploadBuffDesc = CD3DX12_RESOURCE_DESC::Buffer(this->screenWidth * this->screenHeight * sizeof(uint64_t));

    D3D12_HEAP_PROPERTIES randUploadBuffProps{
    .Type = D3D12_HEAP_TYPE_UPLOAD,
    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    .CreationNodeMask = 0,
    .VisibleNodeMask = 0
    };
    
    TIF(this->device->CreateCommittedResource(
        &randUploadBuffProps,
        D3D12_HEAP_FLAG_NONE,
        &randUploadBuffDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&randUploadBuffer)));

    uint32* randomPtr;
    TIF(randUploadBuffer->Map(0, nullptr, (void**)&randomPtr));
    std::random_device rd;
    std::mt19937 e2(rd());
    //TODO: investigate method to make random_device thread safe (by having <omp parallel> number of random_devices in an array)
    
    for (uint i = 0; i < screenWidth * screenHeight * 2; i++) //width * height * 2 as each is actually 64 bit, but we're doing it in chunks of 32
    {
        randomPtr[i] = e2();
    }
    randUploadBuffer->Unmap(0, nullptr);

    //TODO: this doesn't like it when the size isn't divisible by 32
    auto footprint = CD3DX12_SUBRESOURCE_FOOTPRINT(randBuffer->GetDesc(), RoundUp(this->screenWidth * sizeof(uint64_t), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D = { 0 };
    placedTexture2D.Offset = 0;
    placedTexture2D.Footprint = footprint;

    auto copyDest = CD3DX12_TEXTURE_COPY_LOCATION(randBuffer.Get(), 0);
    auto copySrc = CD3DX12_TEXTURE_COPY_LOCATION(randUploadBuffer.Get(), placedTexture2D);


    D3D12_RESOURCE_BARRIER barriers[] = {
    D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = randUploadBuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COMMON, .StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE}},
    D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = randBuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_PRESENT, .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST}},
    D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = randUploadBuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE, .StateAfter = D3D12_RESOURCE_STATE_COMMON} },
    D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = randBuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST, .StateAfter = D3D12_RESOURCE_STATE_PRESENT} } };
    commandList->ResourceBarrier(2, &barriers[0]);
    commandList->CopyTextureRegion(
        &copyDest,
        0, 0, 0,
        &copySrc,
        nullptr);
    commandList->ResourceBarrier(2, &barriers[2]);
}

void DxrContext::CopyFramebuffer()
{
    uint a = swapChain->GetCurrentBackBufferIndex();
    TIF(swapChain->GetBuffer(a, IID_PPV_ARGS(&backbuffer)));
    D3D12_RESOURCE_BARRIER barriers[] = { 
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = framebuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS, .StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE}},
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = backbuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_PRESENT, .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST}},
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = framebuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE, .StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS} },
        D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .Transition = {.pResource = backbuffer.Get(), .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST, .StateAfter = D3D12_RESOURCE_STATE_PRESENT} } };
    commandList->ResourceBarrier(2, &barriers[0]);
    commandList->CopyResource(backbuffer.Get(), framebuffer.Get());
    commandList->ResourceBarrier(2, &barriers[2]);
}

void DxrContext::Present()
{
    this->swapChain->Present(0, 0);
    frameNumber++;
}

void DxrContext::Flush()
{
    HANDLE h = nullptr;
    TIF(commandQueue->Signal(fence.Get(), this->fenceValue));
    TIF(fence->SetEventOnCompletion(this->fenceValue, h));
    WaitForSingleObjectEx(h, INFINITE, false);
    this->fenceValue++;
}
void DxrContext::ClearFramebuffer()
{

}

void DxrContext::Render(float ct)
{
    this->ResetCommandList();
    this->ClearFramebuffer();
    this->BuildTlas();
    this->DispatchRays();
    this->denoiser->Denoise(this->commandList);
    //TODO: denoise shader
    this->CopyFramebuffer();
    this->ExecuteCommandList();
    this->Flush();
    this->Present();
}

void DxrContext::alterInstanceTransform(uint index, mat4x4 tx)
{
    mat3x4 oldTransform;
    for (uint i = 0; i < 3; i++)
    {
        for (uint j = 0; j < 4; j++)
        {
            oldTransform[i][j] = instanceDescs[index].Transform[i][j];
        }
    }
    mat4x4 full = transpose(oldTransform);
    full[3][3] = 1.0f;
    full = full * tx; //todo order of this?

    float q = full[3][3];
    mat3x4 newTransform = transpose(full) / q;

    for (uint i = 0; i < 3; i++)
    {
        for (uint j = 0; j < 4; j++)
        {
            instanceDescs[index].Transform[i][j] = newTransform[i][j];
        }
    }
}