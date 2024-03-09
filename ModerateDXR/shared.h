#pragma once

#ifdef __cplusplus
#include "common.h"

using float32_t = float;
#endif


struct RayPayload
{
    float color;
};

struct Vert
{
    float33 pos;
    float3 norm;
};
struct Tringle
{
    Vert verts[3];
};