#pragma once

//hlsl2021 compat
#ifdef __cplusplus
#include "common.h"

using float3 = glm::fvec3;
using float4 = glm::fvec4;
using float16_t3 = glm::u16vec3; //just for equivalent size
using float16_t4 = glm::u16vec4;
#endif

#ifdef __cplusplus
#pragma pack(push, 4)
#endif

#define MAX_RECURSION_LAYERS 16
#define PI 3.14159265f
struct ConstantBufferStruct
{
    float3 camPos;
    float ct;
    float3 lookAt;
    float fov;
};

struct RayPayload
{
    float3 accum;
    uint layer;
    float3 mask;
    uint seed;
};


struct Vert
{
    float3 pos;
};
struct Tringle
{
    Vert verts[3];
};



struct Material
{
// #ifdef __cplusplus
// 	Material(
// 		float3 color = float3(1.0f, 1.0f, 1.0f),
// 		float3 emission = float3(0.0f, 0.0f, 0.0f),
// 		float ni = 1.0f,
// 		float ns = 1.0,
// 		float trans = 0.0,
// 		float metal = 0.0,
// 		float smooth = 0.0,
// 		uint indicator = 0)
// 	{
// 		this->color = color;
// 		this->emission = emission;
// 		this->ni = ni;
// 		this->ns = ns;
// 		this->trans = trans;
// 		this->metal = metal;
// 		this->smooth = smooth;
// 		this->indicator = indicator;
// 	}
    
// #endif // __cplusplus

    float3 color; //base color
    float ni; //index of refracton
    float3 emission; //light emissition 
    float ns; //specular exponent
    float trans; //transparency
    float metal; //metalness
    float smooth; //smoothness
    uint indicator;
};




#ifdef __cplusplus
#pragma pack(pop, 4)
#endif

