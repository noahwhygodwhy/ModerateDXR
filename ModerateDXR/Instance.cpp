#include "Instance.hpp"

Instance::Instance(const Raytracable& object, fmat3x4 transform, uint hgIndex)
{
	this->transform = mat4x4(transform);

	for (uint x = 0; x < 3; x++)
	{
		for (uint y = 0; y < 4; y++)
		{
			this->instanceDesc.Transform[x][y] = transform[x][y];
		}
	}
	this->instanceDesc.InstanceID = object.heapDescIndex-1;
	this->instanceDesc.InstanceMask = 0x1;
	this->instanceDesc.InstanceContributionToHitGroupIndex = hgIndex;
	this->instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	this->instanceDesc.AccelerationStructure = object.getBlasAddress();
}

D3D12_RAYTRACING_INSTANCE_DESC Instance::GetInstanceDesc()
{
	return this->instanceDesc;
}

Instance::~Instance()
{

}