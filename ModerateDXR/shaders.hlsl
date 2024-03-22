#include "shared.h"

ConstantBuffer<ConstantBufferStruct>  constants  : register(b0);
RaytracingAccelerationStructure       scene      : register(t0);
RWTexture2D<float16_t4>               colorOut   : register(u0);
StructuredBuffer<Tringle>             geomdata[] : register(t1);

[shader("raygeneration")]
void RayGeneration()
{
    
    const float3 pos = constants.camPos;
    const float3 forward = normalize(constants.lookAt - constants.camPos);
    const float3 worldUp = float3(0, 1, 0);
    const float3 right = cross(forward, worldUp);
    const float3 up = cross(forward, right);
    
    const uint2 pixelID = DispatchRaysIndex().xy;
    const uint2 dims = DispatchRaysDimensions().xy ;

    const float2 halfDims = dims / 2.0f;
    const float halfFOV = constants.fov / 2.0f;
        
    const float distanceToPlane = halfDims.x / tan(halfFOV);
    const float3 centerOfPlane = pos + (forward * distanceToPlane);
    const float2 offsetFromCenter = float2(pixelID) - halfDims;
    const float3 lookAtPoint = centerOfPlane + (right * offsetFromCenter.x) + (up * offsetFromCenter.y);

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = normalize(lookAtPoint - pos);
    ray.TMin = 0.001;
    ray.TMax = 1000;

    RayPayload payload = (RayPayload)0;

    TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);

    colorOut[pixelID] = float16_t4(payload.color.x, payload.color.y, payload.color.z, 1.0);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float16_t3(0.0, 0.0, 0.0);
}

float3 distanceColor()
{
    float i = frac(RayTCurrent());
    return float3(i, i, i);
}

//colors 
void unifiedShading(inout RayPayload payload)
{
    uint instance_id = InstanceID();
    uint prim_index = PrimitiveIndex();
    uint geom_index = GeometryIndex();
    if(geom_index == 500)
    {
        return;
    }
    Tringle tri = geomdata[instance_id][prim_index];
    
    float3 a = tri.verts[0].pos;
    float3 b = tri.verts[1].pos;
    float3 c = tri.verts[2].pos;
    
    float3 ba = b - a;
    float3 ca = c - a;
    float3 normal = normalize(cross(ba, ca));
    normal = mul(normal, ((float3x3) ObjectToWorld4x3()));
    float3 color = abs(normal);
    payload.color = float16_t3(color.x, color.y, color.z);
}


[shader("closesthit")]
void ch_hitgroup_a(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    unifiedShading(payload);
}

[shader("closesthit")]
void ch_hitgroup_b(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    unifiedShading(payload);
}

[shader("closesthit")]
void ch_hitgroup_c(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    payload.color = distanceColor();
}













