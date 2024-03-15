#pragma once
#include "common.h"
#include "Raytracable.hpp"

class Instance
{
public:
	Instance(const Raytracable& object, fmat3x4 transform, uint hgIndex = 0);
	~Instance();
	D3D12_RAYTRACING_INSTANCE_DESC GetInstanceDesc();
private:
	fmat4x4 transform;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc;

};


