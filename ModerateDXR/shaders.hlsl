
// Most of this just copied from 
// https://landelare.github.io/2023/02/18/dxr-tutorial.html#shader.hlsl

#include "shared.h"
#include "CookTorrance.hlsl"

ConstantBuffer<ConstantBufferStruct>  constants  : register(b0);
globallycoherent RWTexture2D<float16_t4>               colorOut   : register(u0);
globallycoherent RWTexture2D<uint64_t>                 randBuff   : register(u1);
RaytracingAccelerationStructure       scene      : register(t0);
StructuredBuffer<Tringle>             geomdata[] : register(t1);

inline uint MWC64X(in uint2 pixelID)
{
    uint64_t state = randBuff[pixelID];
    uint c = uint((state) >> 32);
    uint x = uint((state) & 0xFFFFFFFF);
    state = uint64_t(x * ((uint64_t) 4294883355U) + c);
    randBuff[pixelID] = state;
    return x ^ c;
}

inline float randFloat01(in uint2 pixelID)
{
    //return 0.5;
    return (float(MWC64X(pixelID)) / float(0xFFFFFFFF));
}
inline float randFloat11(in uint2 pixelID)
{
    return (randFloat01(pixelID) * 2.0) - 1.0;
}

inline float randFloat0505(in uint2 pixelID)
{
    return (randFloat01(pixelID)) - 0.5;
}

//Don't use this if it's a procedural geometry
uint numTriangles()
{
    uint numtris, stride;
    geomdata[NonUniformResourceIndex(InstanceID())].GetDimensions(numtris, stride);
    return numtris/2;
}

[shader("raygeneration")]
void RayGeneration()
{
    
    float3 pos = constants.camPos;
    float3 forward = normalize(constants.lookAt - constants.camPos);
    float3 worldUp = float3(0, 1, 0);
    float3 right = cross(forward, worldUp);
    float3 up = cross(forward, right);
     
    uint2 pixelID = NonUniformResourceIndex(DispatchRaysIndex()).xy;
    uint2 dims = NonUniformResourceIndex(DispatchRaysDimensions()).xy;
    float2 halfDims = dims / 2.0f;
    float halfFOV = constants.fov / 2.0f;
        
    float distanceToPlane = halfDims.x / tan(halfFOV);
    
    float3 centerOfPlane = pos + (forward * distanceToPlane);
    
    float2 offsetFromCenter = float2(pixelID) - halfDims;
    
    

    float3 lookAtPoint = centerOfPlane + (right * offsetFromCenter.x) + (up * offsetFromCenter.y);
    RayDesc ray;
    ray.Direction = normalize(lookAtPoint - pos);
    ray.Origin = pos;
    ray.TMin = 0.001;
    ray.TMax = 1000;



    // for(uint i = 0; i < NUM_SAMPLES; i++)
    // {
    // const uint loBits = (uint32_t)(randBuff[pixelID]&0xFFFFFFFF);
    // const uint hiBits = (uint32_t)((randBuff[pixelID]>>32)&0xFFFFFFFF);
    // const float loFloat = (((float)loBits)/((float)0xFFFFFFFF));
    // const float hiFloat = (((float)hiBits)/((float)0xFFFFFFFF));
    // colorOut[pixelID].x = (float16_t) hiFloat;
    // colorOut[pixelID].y = (float16_t) loFloat;
    // colorOut[pixelID].z = 0.1f;
    //return;
    float3 accum = float3(0, 0, 0);
    for(uint i = 0; i < NUM_SAMPLES; i++)
    {
        float3 fuzzyLookAtPoint = centerOfPlane + (right * offsetFromCenter.x + randFloat0505(pixelID)) + (up * offsetFromCenter.y + randFloat0505(pixelID));
        ray.Direction = normalize(fuzzyLookAtPoint - pos);

        RayPayload payload = (RayPayload)0;
        payload.mask = float16_t3(1.0, 1.0, 1.0);
        payload.layer = 0; //why doesn't the api provide a layer ugh
        payload.accum = float16_t3(0.0, 0.0, 0.0);
        payload.insideGlass = 0;
        payload.padd = 0;

        TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
    // //TODO: this clamp being required means somethign is *wrong*
        accum += clamp(payload.accum, float3(0, 0, 0), float3(1, 1, 1));
        //accum += payload.accum;
    }
    accum /= float3(NUM_SAMPLES, NUM_SAMPLES, NUM_SAMPLES);

    colorOut[pixelID] = float16_t4(accum, 1.0);

    // if(constants.frameNumber % NUM_SAMPLES == 0)
    // {
    //     colorOut[pixelID] = float16_t4(0, 0, 0, 1);
    // }
    // //TODO: this clamp being required means somethign is *wrong*
    // float3 sampl3 = clamp(payload.accum, float3(0, 0, 0), float3(1, 1, 1)) / float(NUM_SAMPLES);
    // colorOut[pixelID] += float16_t4(sampl3.x, sampl3.y, sampl3.z, 0.0);
    // colorOut[pixelID].w = 1.0;
    // }

    
    
    //payload.accum.b = float(payload.layer) / float(MAX_RECURSION_LAYERS);
    
    //colorOut[pixelID] = float16_t4(0.0, 1.0, 0.0, 1.0);
}

float3 doTransform(float3 vec3, float4x3 mat4x3)
{
    float4 vec4 = float4(vec3.x, vec3.y, vec3.z, 1.0);

    float4x4 mat4x4 = float4x4(
        mat4x3[0][0],mat4x3[0][1], mat4x3[0][2], 0,
        mat4x3[1][0],mat4x3[1][1], mat4x3[1][2], 0,
        mat4x3[2][0],mat4x3[2][1], mat4x3[2][2], 0,
        mat4x3[3][0],mat4x3[3][1], mat4x3[3][2], 1
    );
    mat4x4 = transpose(mat4x4);

    float4 retVal = mul(vec4, mat4x4);

    return retVal.xyz;
}


float3 getWorldSpaceNormal(float2 bary)
{
    uint offset = numTriangles();
    Tringle tri = geomdata[NonUniformResourceIndex(InstanceID())][NonUniformResourceIndex(PrimitiveIndex())];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float baryA = bary.x;
    float baryB = bary.y;
    float baryC = 1.0 - (baryA + baryB);

    float3 normal = (tri.verts[0].norm * baryC) +
                    (tri.verts[1].norm * baryA) +
                    (tri.verts[2].norm * baryB);

    normal = normalize(mul(normal, ObjectToWorld4x3()));
    return normal; 
}

// float3 getWorldSpaceNormal()
// {
//     uint offset = numTriangles();
//     Tringle tri = geomdata[NonUniformResourceIndex(InstanceID())][NonUniformResourceIndex(PrimitiveIndex())];
//     //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
//     float3 a = tri.verts[0].pos;
//     float3 b = tri.verts[1].pos;
//     float3 c = tri.verts[2].pos;
    
//     float3 ba = b - a;
//     float3 ca = c - a;
//     float3 normal = cross(ba, ca);
//     normal = normalize(mul((float3x3)ObjectToWorld4x3(), normal));
//     return normal;
//     //normal = normalize(doTransform(normal, ObjectToWorld4x3()));
//    /// return normal;
// }



[shader("miss")]
void Miss(inout RayPayload payload)
{
    if(payload.layer == 0) { payload.accum = float3(0, 0, 0); }
    return;
    
}

float3 getColor(in Material mat, in float3 pos)
{
    switch(mat.indicator)
    {
        case 1: //checkered
            {
            int2 xy = int2(floor(pos.xz));
            uint colorIdx = (xy.x+xy.y)%2 == 0;

            //int3 xyz = (int3)floor(select(pos < 1, pos-1, pos));
            //bool3 evenSquare = xyz%2;
        
            //uint colorIdx = evenSquare.x ^ evenSquare.y ^ evenSquare.z;// ((xyz.x + xyz.y)%2);
            return colorIdx ? float3(1.0, 1.0, 1.0)-mat.color : mat.color;
            
            }
            break;
        default:
            return mat.color;
    }
}


float3 generateNewRayDir(float3 oldRayDir, in float3 normal)
{
    (void)oldRayDir;
    //TODO: this was essentially stolen from 
    //http://raytracey.blogspot.com/2016/11/opencl-path-tracing-tutorial-2-path.html
    //need to go through and see how the one I wrote was wrong after I confirm this works.

    //TODO: this refracts sometime when it shouldn't?

    //float3 w = dot(normal, oldRayDir) < 0.0f ? normal : normal * (-1.0f);
    float3 w = normal;

    /* compute two random numbers to pick a random point on the hemisphere above the hitpoint*/
    
    float rand1 = 2.0f * PI * randFloat01(NonUniformResourceIndex(DispatchRaysIndex()).xy);
    float rand2 = randFloat01(NonUniformResourceIndex(DispatchRaysIndex()).xy);

    float rand2s = sqrt(rand2);

    /* create a local orthogonal coordinate frame centered at the hitpoint */
    float3 u = abs(w.x) > 0.1f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f); //axis
    u = normalize(cross(u, w));
    float3 v = cross(w, u);
    v = normalize(v);                
    return normalize(u * cos(rand1) * rand2s + v * sin(rand1) * rand2s + w * sqrt(1.0f - rand2));
}

void UnifiedShading(
inout RayPayload payload,
in Material mat,
in float2 bary
)
{
    float3 normal = getWorldSpaceNormal(bary);
    //payload.accum = abs(normal);
    //return;
    float3 pos = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());

    //TODO: multiple rays for tree branching motecarlo for more efficient tracing?
    // Maybe only branch after first hit..?
    float3 oldRayDir = WorldRayDirection();
    
    float transDecider = randFloat01(NonUniformResourceIndex(DispatchRaysIndex()).xy);
    float mirrorDecider = randFloat01(NonUniformResourceIndex(DispatchRaysIndex()).xy);
    float3 newRayDir;

    bool doingTransparencyStuff = false;

    if(payload.insideGlass)
    {
        payload.insideGlass = false;
        newRayDir = refract(oldRayDir, -normal, 1.0/mat.ior);
        doingTransparencyStuff = true;
    }
    else if (mat.trans >= transDecider)
    {
        // pos -= normal*0.0001f;
        payload.insideGlass = true;
        newRayDir = refract(oldRayDir, normal, 1);//mat.ior);
        doingTransparencyStuff = true;
    }
    else if(mat.smooth >= mirrorDecider)
    {
        newRayDir = reflect(oldRayDir, normal);
    }
    else
    {
        newRayDir = generateNewRayDir(oldRayDir, normal);
    }

    if(!doingTransparencyStuff)
    { // Cook Torance shading model
    
        float3 color = getColor(mat, pos);

        //float tf = abs((1.0 - mat.ior) / (1.0 + mat.ior));
        float3 F0a = abs((1.0 - mat.ior) / (1.0 + mat.ior)).xxx;

        F0a = F0a * F0a;

        float3 F0 = lerp(F0a, color, mat.metal);
        float3 kS = float3(0, 0, 0);
        float3 cT = max(CookTorance(oldRayDir, newRayDir, normal, mat.ior, F0, kS), 0.0f);
        //float3 R = reflect(newRayDir, normal);


        float3 V = -oldRayDir;
        float3 N = normal;
        float3 L = newRayDir;

        float3 H = normalize((L + V));

        float diff = max(dot(L, N), 0.0f);
        //float spec = max(dot(N, H), 0.0f);
        //float lightsum = diff + spec;
        float3 kD = ((1.0f - kS) * (1.0f - mat.metal));

        payload.accum += mat.emission * payload.mask;
        payload.mask *= color * ((diff * kD) + (cT));
    }
    { // new ray stuff
        payload.layer++;
        if (payload.layer >= MAX_RECURSION_LAYERS || any(mat.emission > 0))
        {
            //payload.accum.x =
            return;
        }
        
        RayDesc ray;
        ray.Origin = pos;
        ray.Direction = newRayDir;
        ray.TMin = 0.001;
        ray.TMax = 1000;
        
        TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
    }
}


[shader("closesthit")]
void ch_white(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(1, 1, 1);
    mat.smooth = 0.0f;
    mat.emission = float3(0, 0, 0);
    mat.trans = 0;
    mat.ns = 1;
    mat.ior = 1;
    mat.metal = 0;
    mat.indicator = 0;

    UnifiedShading(payload, mat, attrib.barycentrics);
}

[shader("closesthit")]
void ch_shinyred(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(1, 0.1, 0.1);
    mat.smooth = 0.5f;
    mat.emission = float3(0, 0, 0);
    mat.trans = 0;
    mat.ns = 1;
    mat.ior = 1;
    mat.metal = 0.1;
    mat.indicator = 0;

    UnifiedShading(payload, mat, attrib.barycentrics);
}

[shader("closesthit")]
void ch_light(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(1, 1, 1);
    mat.smooth = 0.9f;
    mat.emission = float3(1, 1, 1); //TODO: brightness?
    mat.trans = 0;
    mat.ns = 1;
    mat.ior = 1;
    mat.metal = 0.1;
    mat.indicator = 0;

    UnifiedShading(payload, mat, attrib.barycentrics);
}

[shader("closesthit")]
void ch_metal(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(1, 1, 1);
    mat.smooth = 0.6f;
    mat.emission = float3(0, 0, 0); //TODO: brightness?
    mat.trans = 0;
    mat.ns = 1;
    mat.ior = 1;
    mat.metal = 0.9;
    mat.indicator = 0;

    UnifiedShading(payload, mat, attrib.barycentrics);
}

[shader("closesthit")]
void ch_mirror(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(1, 1, 1);
    mat.smooth = 0.99f;
    mat.emission = float3(0, 0, 0); //TODO: brightness?
    mat.trans = 0;
    mat.ns = 1;
    mat.ior = 1;
    mat.metal = 1.0;
    mat.indicator = 0;

    UnifiedShading(payload, mat, attrib.barycentrics);
}

[shader("closesthit")]
void ch_checkered(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(1, 1, 1);
    mat.smooth = 0.1f;
    mat.emission = float3(0, 0, 0); //TODO: brightness?
    mat.trans = 0;
    mat.ns = 1;
    mat.ior = 1;
    mat.metal = 0.1;
    mat.indicator = 1;

    UnifiedShading(payload, mat, attrib.barycentrics);
}

[shader("closesthit")]
void ch_glass(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    Material mat = (Material)0;
    mat.color = float3(0.2, 0.8, 0.8);
    mat.smooth = 0.1f;
    mat.emission = float3(0, 0, 0); //TODO: brightness?
    mat.trans = 0.9;
    mat.ns = 1;
    mat.ior = 1.52;
    mat.metal = 0.1;
    mat.indicator = 0;

    UnifiedShading(payload, mat, attrib.barycentrics);
}