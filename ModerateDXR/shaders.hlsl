
// Most of this just copied from 
// https://landelare.github.io/2023/02/18/dxr-tutorial.html#shader.hlsl

#include "shared.h"

ConstantBuffer<ConstantBufferStruct>  constants  : register(b0);
RWTexture2D<float16_t4>               colorOut   : register(u0);
RaytracingAccelerationStructure       scene      : register(t0);
StructuredBuffer<Tringle>             geomdata[] : register(t1);


//TODO: redo this, i don't like it
[shader("raygeneration")]
void RayGeneration()
{
    
    float3 pos = constants.camPos;
    float3 forward = normalize(constants.lookAt - constants.camPos);
    float3 worldUp = float3(0, 1, 0);
    float3 right = cross(forward, worldUp);
    float3 up = cross(right, forward);
     
    
    
    uint2 pixelID = DispatchRaysIndex().xy;
    uint2 dims = DispatchRaysDimensions().xy ;
    float2 halfDims = dims / 2.0f;
    float halfFOV = constants.fov / 2.0f;
        
    float distanceToPlane = halfDims.x / tan(halfFOV);
    
    float3 centerOfPlane = pos + (forward * distanceToPlane);
    
    
    //float2 offsetFromCenter = float2(dims.x, dims.y) - halfDims;
    float2 offsetFromCenter = float2(pixelID) - halfDims;
    
    
    float3 lookAtPoint = centerOfPlane + (right * offsetFromCenter.x) + (up * offsetFromCenter.y);

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = normalize(lookAtPoint - pos);
    ray.TMin = 0.001;
    ray.TMax = 1000;

    RayPayload payload = (RayPayload)0;

    TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
    //colorOut[pixelID] = float16_t4(0.0, 1.0, 0.0, 1.0);
    colorOut[pixelID] = float16_t4(payload.color.x, payload.color.y, payload.color.z, 1.0);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float16_t3(0.0, 0.0, 0.0);
}


float3 getWorldSpaceNormal()
{
    Tringle tri = geomdata[InstanceID()][PrimitiveIndex()];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float3 a = tri.verts[0].pos;
    float3 b = tri.verts[1].pos;
    float3 c = tri.verts[2].pos;
    
    float3 ba = b - a;
    float3 ca = c - a;
    float3 normal = normalize(cross(ba, ca));
    return normal;//    mul(normal, ((float3x3) ObjectToWorld4x3()));
}


[shader("closesthit")]
void ch_normalcolors(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    (void) attrib;


    Tringle tri = geomdata[InstanceID()][PrimitiveIndex()];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float3 a = tri.verts[0].pos;
    float3 b = tri.verts[1].pos;
    float3 c = tri.verts[2].pos;
    
    float3 ba = b - a;
    float3 ca = c - a;
    float3 normal = normalize(cross(ba, ca));
    normal = mul(normal, ((float3x3) ObjectToWorld4x3()));
    float3 color = abs(normal);
    payload.color = float16_t3(color.x, color.y, color.z);
    return;
}

[shader("closesthit")]
void ch_red(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    (void) attrib;
    //payload.color = float16_t3(0.0, 0.0, 1.0);
    //return;
    payload.color = float16_t3(1, 0, 0);
    return;
    //Tringle tri = geomdata[InstanceID()][PrimitiveIndex()];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}

[shader("closesthit")]
void ch_mirror(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    (void) attrib;
    
    
    float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 normal = getWorldSpaceNormal();
    float3 reflected = reflect(normalize(WorldRayDirection()), normal);
    
    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = reflected;
    ray.TMin = 0.001;
    ray.TMax = 1000;
    
    RayPayload payload2 = (RayPayload) 0;
    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload2);
    payload.color = (payload2.color * 0.95) + float16_t3(0.05, 0.05, 0.05);
    return;

   // (void) attrib;
    //payload.color = float16_t3(0.0, 0.0, 1.0);
    //return;
    //payload.color = float16_t3(1, 0, 0);
    //return;
    //Tringle tri = geomdata[InstanceID()][PrimitiveIndex()];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}