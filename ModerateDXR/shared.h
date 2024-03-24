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
#define NUM_SAMPLES 32//8096
#define MAX_RECURSION_LAYERS 12
#define PI 3.14159265f
struct ConstantBufferStruct
{
    float3 camPos;
    float ct;
    float3 lookAt;
    float fov;
    uint frameNumber;
};

struct RayPayload
{
    float3 accum;
    uint layer;
    float3 mask;
    uint32_t padd : 31;
    uint32_t insideGlass : 1;
};

struct Vert
{
    float3 pos;
    float3 norm;
};

struct Tringle
{
    Vert verts[3];
};

struct Material
{
    float3 color; //base color
    float smooth; //smoothness
    float3 emission; //light emissition 
    float trans; //transparency
    float ns; //specular exponent
    float ior; //index of refracton
    float metal; //metalness
    uint indicator;
};

#ifdef __cplusplus
#pragma pack(pop, 4)
#endif

