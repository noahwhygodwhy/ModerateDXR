#pragma once
#include "common.h"
#include "shared.h"


constexpr float resizeDebounce = 0.1f;






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

#define HG_WHITE     0
#define HG_SHINYRED  1
#define HG_LIGHT     2
#define HG_METAL     3
#define HG_MIRROR    4
#define HG_CHECKERED 5
#define HG_GLASS     6

const static HitGroupData hgdata[] =
{
	HitGroupData(L"white"),
	HitGroupData(L"shinyred"),
	HitGroupData(L"light"),
	HitGroupData(L"metal"),
	HitGroupData(L"mirror"),
	HitGroupData(L"checkered"),
	HitGroupData(L"glass"),
};

//This is specifically meant for d3d12 dxr instance descs
struct Transform
{
	fvec3 translate;
	float rotateDegrees;
	fvec3 rotateAxis = fvec3(0, 1, 0);
	fvec3 scale = fvec3(1, 1, 1);
	fmat4x4 operator()()
	{
		fmat4x4 retVal = mat4(1);
		retVal = glm::translate(retVal, this->translate);
		retVal = glm::rotate(retVal, glm::radians(rotateDegrees), rotateAxis);
		retVal = glm::scale(retVal, this->scale);
		retVal = glm::transpose(retVal);
		return retVal;
	}
};

class DxrContext
{
public:
	DxrContext(HWND hwnd, uint initWidth, uint initHeight);
	~DxrContext();
	void* MapConstantBuffer();
	void SetResize(uint initWidth, uint initHeight);
	void CreateScreenSizedResources();
	void ClearFramebuffer();
	void PopulateAndCopyRandBuffer();
	void BuildTlas();
	void DispatchRays();
	void CopyFramebuffer();
	void Present();
	void ResetCommandList();
	void ExecuteCommandList();
	void Flush();
	void Render(float ct);
	void CreateTlasResources(vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs);
	void UploadInstanceDescs();
	ComPtr<ID3D12Device10> device;
	ComPtr<ID3D12GraphicsCommandList4> commandList;
	ComPtr<ID3D12DescriptorHeap> descHeap;
	ComPtr<ID3D12DescriptorHeap> cpuDescHeap;
	ConstantBufferStruct* constants;
	vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	std::chrono::steady_clock::time_point startTimePoint;
	std::chrono::steady_clock::time_point lastFrameTimePoint;


	void alterInstanceTransform(uint instanceIndex, mat4x4 tx);

private:
	bool needResize = false;
	uint newWidth;
	uint newHeight;
	void DoResize(float ct);
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
	uint64_t fenceValue = 0;

	ComPtr<ID3D12CommandQueue> commandQueue; //TODO: two queues for doing frame n+1's rtas build while frame n's raytracing (more memory usage? :\)
	ComPtr<ID3D12CommandAllocator> commandAllocator;

	ComPtr<ID3D12Resource> constantBuffer;
	ComPtr<ID3D12Resource> shaderBuffer;
	ComPtr<ID3D12Resource> instanceBuffer;
	ComPtr<ID3D12Resource> backbuffer;
	
	uint sizeOfResourceDesc;

	ComPtr<ID3D12Resource> randUploadBuffer;
//#define NUM_RESOURCES_BEFORE_GEOM 2
	ComPtr<ID3D12Resource> randBuffer;
	ComPtr<ID3D12Resource> framebuffer;
// the above are the two resources before the geom buffers in the desc heap
	ComPtr<ID3D12Resource> tlas;
	ComPtr<ID3D12Resource> tlasScratch;

	HANDLE swapChainEvent;


	void BuildRootSignature();
	void BuildPipelineStateObject();
};

//inline mat3x4 fromInstanceTransform(float val[3][4])
//{
//	mat3x4 retVal;
//	retVal[0][0] = val[0][0];
//	retVal[0][1] = val[0][1];
//	retVal[0][2] = val[0][2];
//	retVal[0][3] = val[0][3];
//
//	retVal[1][0] = val[1][0];
//	retVal[1][1] = val[1][1];
//	retVal[1][2] = val[1][2];
//	retVal[1][3] = val[1][3];
//
//	retVal[2][0] = val[2][0];
//	retVal[2][1] = val[2][1];
//	retVal[2][2] = val[2][2];
//	retVal[2][3] = val[2][3];
//	return retVal;
//}
//
//inline array<array<float, 4>, 3> toInstanceTransform(mat3x4 val)
//{
//	array<array<float, 4>, 3> retVal;
//	retVal[0][0] = val[0][0];
//	retVal[0][1] = val[0][1];
//	retVal[0][2] = val[0][2];
//	retVal[0][3] = val[0][3];
//
//	retVal[1][0] = val[1][0];
//	retVal[1][1] = val[1][1];
//	retVal[1][2] = val[1][2];
//	retVal[1][3] = val[1][3];
//
//	retVal[2][0] = val[2][0];
//	retVal[2][1] = val[2][1];
//	retVal[2][2] = val[2][2];
//	retVal[2][3] = val[2][3];
//	return retVal;
//}