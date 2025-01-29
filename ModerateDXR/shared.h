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

#define FREQUENCIES 48
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
    uint32_t padd : 30;
    uint32_t recordHit : 1;
    uint32_t insideGlass : 1;
};

struct HitInfo
{
    uint32_t instanceID :24;
    uint32_t matID :8;
    uint32_t geometryIndex :24;
    uint32_t didRefraction :1;
    uint32_t padd0 :7;
};


struct Vert
{
    float3 pos;
    float3 norm;
    uint diffuseTextureIdx;
    uint normalTextureIdx;
    uint specularTextureIdx;
    uint metalicTextureIdx;
    uint roughnessTextureIdx;
    uint alphaTextureIdx;
    uint emmisiveTextureIdx;

};

struct Tringle
{
    Vert verts[3];
};
static uint x = sizeof(Tringle)/sizeof(uint);

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

