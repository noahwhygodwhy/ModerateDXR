#include "Mesh.hpp"
#include "shared.h"
//#include <assimp/>



void Mesh::CreateResources(ComPtr<ID3D12Device10> device)
{
    CD3DX12_HEAP_PROPERTIES geomProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC geomDesc = CD3DX12_RESOURCE_DESC::Buffer(this->verts.size() * sizeof(vec3) * 2); // * 2 cause verts, then norms
    TIF(device->CreateCommittedResource(&geomProps, D3D12_HEAP_FLAG_NONE, &geomDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&geomBuffer)));
    geomBuffer->SetName(L"geomBuffer");
    void* uploadPointer;
    geomBuffer->Map(0, nullptr, &uploadPointer);
    memcpy(uploadPointer, this->verts.data(), this->verts.size() * sizeof(vec3));

    void* normalPointer = (void*) (((char*)uploadPointer) + (this->verts.size()*sizeof(vec3)));
    memcpy(normalPointer, this->norms.data(), this->norms.size() * sizeof(vec3));

    geomBuffer->Unmap(0, nullptr);

    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC trianglesDesc =
    {
        .Transform3x4 = 0,
        .IndexFormat = DXGI_FORMAT_UNKNOWN,
        .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
        .IndexCount = 0,
        .VertexCount = (uint32_t)this->verts.size(),
        .IndexBuffer = 0,
        .VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE {
            .StartAddress = this->geomBuffer->GetGPUVirtualAddress(),
            .StrideInBytes = sizeof(vec3)
        }
    };

    this->geometryDesc = {
        .Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
        .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE,
        .Triangles = trianglesDesc
    };

    this->blasInput = {
        .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
        .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
        .NumDescs = 1,
        .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
        .pGeometryDescs = &geometryDesc
    };

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
    device->GetRaytracingAccelerationStructurePrebuildInfo(&this->blasInput, &prebuildInfo);

    CD3DX12_HEAP_PROPERTIES blasProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC blasScratchDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_RESOURCE_DESC blasResultDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    TIF(device->CreateCommittedResource(&blasProps, D3D12_HEAP_FLAG_NONE, &blasScratchDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&blasScratch)));
    blasScratch->SetName(L"blasScratch");
    TIF(device->CreateCommittedResource(&blasProps, D3D12_HEAP_FLAG_NONE, &blasResultDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&blas)));
    blas->SetName(L"blas");



    
}

D3D12_RESOURCE_BARRIER Mesh::BuildBLAS(ComPtr<ID3D12Device10> device, ComPtr<ID3D12GraphicsCommandList4> commandList)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc =
    {
        .DestAccelerationStructureData = this->blas->GetGPUVirtualAddress(),
        .Inputs = this->blasInput,
        .SourceAccelerationStructureData = 0,
        .ScratchAccelerationStructureData = this->blasScratch->GetGPUVirtualAddress()
    };

    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    return D3D12_RESOURCE_BARRIER{ .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV, .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, .UAV = {.pResource = this->blas.Get()} };
}

void Mesh::processMesh(aiMesh* mesh, const aiScene* scene)
{

    //TODO: make an index buffer
    vector<fvec3> unindexedVerts;
    vector<fvec3> unindexedNorms;
    
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        fvec3 newVert = fvec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        unindexedVerts.push_back(newVert);

        fvec3 newNormal = glm::normalize(fvec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));
        unindexedNorms.push_back(newNormal);
    }

    for (uint j = 0; j < mesh->mNumFaces; j++)
    {
        aiFace f = mesh->mFaces[j];
        for (uint k = 0; k < f.mNumIndices; k++)
        {
            verts.push_back(unindexedVerts[f.mIndices[k]]);
            norms.push_back(unindexedNorms[f.mIndices[k]]);
        }
    }
    if (mesh->mMaterialIndex >= 0)
    {
        // TODO: 
    }
}

D3D12_BUFFER_SRV Mesh::getGeomBufferInfo()
{
    D3D12_BUFFER_SRV retVal = {
        .FirstElement = 0,
        .NumElements = (uint32_t) (verts.size()/3) * 2,
        .StructureByteStride = sizeof(Tringle),
        .Flags = D3D12_BUFFER_SRV_FLAG_NONE
    };
    return retVal;
}

// Stolen from learnopengl.com
void Mesh::processNode(aiNode* node, const aiScene* scene)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        processMesh(scene->mMeshes[node->mMeshes[i]], scene);
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh::Mesh(string filepath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
	processNode(scene->mRootNode, scene);
}

Mesh::~Mesh()
{
}