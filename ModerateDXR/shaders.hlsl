
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

const float FLOAT_MAX = 3.402823466e+38F;//

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
    ray.Origin = pos;
    ray.Direction = normalize(lookAtPoint - pos);
    ray.TMin = 0.001;
    ray.TMax = 1000;



    // for(uint i = 0; i < NUM_SAMPLES; i++)
    // {
    RayPayload payload = (RayPayload)0;
    payload.mask = float16_t3(1.0, 1.0, 1.0);
    payload.layer = 0; //why doesn't the api provide a layer ugh
    payload.accum = float16_t3(0.0, 0.0, 0.0);
    payload.padd = 0;

    // const uint loBits = (uint32_t)(randBuff[pixelID]&0xFFFFFFFF);
    // const uint hiBits = (uint32_t)((randBuff[pixelID]>>32)&0xFFFFFFFF);
    // const float loFloat = (((float)loBits)/((float)0xFFFFFFFF));
    // const float hiFloat = (((float)hiBits)/((float)0xFFFFFFFF));
    // colorOut[pixelID].x = (float16_t) hiFloat;
    // colorOut[pixelID].y = (float16_t) loFloat;
    // colorOut[pixelID].z = 0.1f;
    //return;

    TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
    if(constants.frameNumber % NUM_SAMPLES == 0)
    {
        colorOut[pixelID] = float16_t4(0, 0, 0, 1);
    }
    //TODO: this clamp being required means somethign is *wrong*
    float3 sampl3 = clamp(payload.accum, float3(0, 0, 0), float3(1, 1, 1)) / float(NUM_SAMPLES);
    colorOut[pixelID] += float16_t4(sampl3.x, sampl3.y, sampl3.z, 0.0);
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


float3 getWorldSpaceNormal()
{
    Tringle tri = geomdata[NonUniformResourceIndex(InstanceID())][NonUniformResourceIndex(PrimitiveIndex())];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float3 a = tri.verts[0].pos;
    float3 b = tri.verts[1].pos;
    float3 c = tri.verts[2].pos;
    
    float3 ba = b - a;
    float3 ca = c - a;
    float3 normal = cross(ba, ca);
    normal = normalize(mul((float3x3)ObjectToWorld4x3(), normal));
    //normal = normalize(doTransform(normal, ObjectToWorld4x3()));
    return normal;
}


[shader("miss")]
void Miss(inout RayPayload payload)
{
    if(payload.layer == 0) { payload.accum = float3(1, 0, 0); }
    return;
    
}

void ShadeIt(
inout RayPayload payload,
in float3 normal,
in float3 oldRayDir,
in float3 newRayDir,
in Material mat
)
{
    float3 lightDir = normalize(newRayDir);
    float NdotL = dot(normal, normalize(lightDir));
    float diffuse = NdotL;

    float3 viewDir = -normalize(oldRayDir);
    float3 H = normalize(lightDir + viewDir);
    //TODO: better solution for this, if these two are the same, then it results in a NAN
    if(all(oldRayDir == lightDir))
    {
        H = viewDir;
    }
    
    float NdotH = dot(normal, H);
    float specular = NdotH;
    float3 nothin = payload.mask;
    if(nothin.x == 50.03f || nothin.y == 50.03f || nothin.z == 50.03f)
    {
        return;
    }
    payload.accum += mat.emission * nothin;
    payload.mask *= mat.color * (diffuse + specular);
}


void ShadeItCookTorance(
inout RayPayload payload,
in float3 normal,
in float3 oldRayDir,
in float3 newRayDir,
in Material mat
)
{
    float tf = abs((1.0 - mat.ni) / (1.0 + mat.ni));
    float3 F0a = abs((1.0 - mat.ni) / (1.0 + mat.ni)).xxx;

    //float3(tf, tf, tf);
    F0a = F0a * F0a;

    float3 F0 = lerp(F0a, mat.color, mat.metal);
    //dvec3 ctSpecular = CookTorance(ray, reflectionRay, minRayResult, downstreamRadiance, minRayResult.shape->mat, currentIOR, F0, kS);
    //TODO: need to keep the current ray
    float3 kS;
    float3 cT = max(CookTorance(oldRayDir, newRayDir, normal, mat.ni, F0, kS), 0.0f);
    //blinn phong (ish)?
    float3 R = reflect(newRayDir, normal); //   rotateVector(newRay.dir, PI, hr.normal); //TODO: not the right rotate


    float3 V = -oldRayDir;
    float3 N = normal;
    float3 L = newRayDir;

    float3 H = normalize((L + V));

    float diff = max(dot(L, N), 0.0f);
    float spec = max(dot(N, H), 0.0f);
    float lightsum = diff + spec;
    float3 kD = ((1.0f - kS) * (1.0f - mat.metal)); //*diff;

    payload.accum += mat.emission * payload.mask;
    payload.mask *= mat.color * ((diff * kD) + (cT));
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

[shader("closesthit")]
void ch_white(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    (void) attrib;

    Material mat = (Material)0;
    mat.color = float3(1, 1, 1);
    mat.smooth = 0.0f;
    mat.emission = float3(0, 0, 0);
    mat.trans = 0;
    mat.ns = 1;
    mat.ni = 1;
    mat.metal = 0;
    mat.indicator = 0;

    UnifiedShading(mat)

    float3 normal = getWorldSpaceNormal();
    float3 pos = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());

    //TODO: a smooth surface will generate more reflected rays
    float3 oldRayDir = WorldRayDirection();
    float3 newRayDir = generateNewRayDir(oldRayDir, normal);
    
    ShadeItCookTorance(
    //ShadeIt(
        payload,
        normal,
        oldRayDir,
        newRayDir,
        0
    );
    
    payload.layer++;// = payload.layer+1;//++;
    //uint layer = payload.layer;
    if (payload.layer >= MAX_RECURSION_LAYERS)
    {
        //payload.accum = float3(1, 0, 0);
        return;
    }
    
    RayDesc ray;
    ray.Origin = pos;// + (normal*0.001f);
    ray.Direction = newRayDir;
    ray.TMin = 0.001;
    ray.TMax = 1000;
    
    TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
};