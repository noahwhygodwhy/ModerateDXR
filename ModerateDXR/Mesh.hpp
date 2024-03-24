#pragma once
#include "common.h"
#include "shared.h"
#include "Raytracable.hpp"

class Mesh : public Raytracable
{
private:
	//vector<uint32_t> indices;
	vector<fvec3> verts;
	vector<fvec3> norms;
public:
	Mesh(string filepath);
	~Mesh();
	D3D12_RESOURCE_BARRIER BuildBLAS(ComPtr<ID3D12Device10> device, ComPtr<ID3D12GraphicsCommandList4> commandList);
	D3D12_BUFFER_SRV getGeomBufferInfo();
	void CreateResources(ComPtr<ID3D12Device10> device);
private:
	void processNode(aiNode* node, const aiScene* scene);
	void processMesh(aiMesh* mesh, const aiScene* scene);
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInput;
};
