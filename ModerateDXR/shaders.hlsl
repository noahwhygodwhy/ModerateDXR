
// Most of this just copied from 
// https://landelare.github.io/2023/02/18/dxr-tutorial.html#shader.hlsl

#include "shared.h"
//#include "CookTorrance.hlsl"

ConstantBuffer<ConstantBufferStruct>  constants  : register(b0);
RWTexture2D<float16_t4>               colorOut   : register(u0);
RaytracingAccelerationStructure       scene      : register(t0);
StructuredBuffer<Tringle>             geomdata[] : register(t1);

[shader("raygeneration")]
void RayGeneration()
{
    
    float3 pos = constants.camPos;
    float3 forward = normalize(constants.lookAt - constants.camPos);
    float3 worldUp = float3(0, 1, 0);
    float3 right = cross(forward, worldUp);
    float3 up = cross(forward, right);
     
    
    
    uint2 pixelID = DispatchRaysIndex().xy;
    uint2 dims = DispatchRaysDimensions().xy ;
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

    RayPayload payload = (RayPayload)0;
    //payload.masked = float16_t3(1.0, 1.0, 1.0);
    //payload.layer = 0; //why doesn't the api provide a layer ugh
    payload.accum = float16_t3(0.0, 0.0, 0.0);
    //payload.seed = 0;//asuint(float(((pixelID.y * dims.x) + pixelID.x) * 10000) * frac(constants.ct));

    TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
    //colorOut[pixelID] = float16_t4(0.0, 1.0, 0.0, 1.0);
    colorOut[pixelID] = float16_t4(payload.accum.x, payload.accum.y, payload.accum.z, 1.0);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.accum = float16_t3(0, 0, 0);
    return;
    //payload.color = float16_t3(0.0, 0.0, 0.0);
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
    uint instanceID = InstanceID();
    uint primdex = PrimitiveIndex();
    Tringle tri = geomdata[instanceID][primdex];
    //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    float3 a = tri.verts[0].pos;
    float3 b = tri.verts[1].pos;
    float3 c = tri.verts[2].pos;
    
    float3 ba = b - a;
    float3 ca = c - a;
    float3 normal = cross(ba, ca);
    normal = normalize(doTransform(normal, ObjectToWorld4x3()));
    return normal;
}

float3 distanceColor()
{
    float i = frac(RayTCurrent());
    return float3(i, i, i);
}




void unifiedShading(inout RayPayload payload)
{
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














// struct Material
// {
// // #ifdef __cplusplus
// // 	Material(
// // 		float3 color = float3(1.0f, 1.0f, 1.0f),
// // 		float3 emission = float3(0.0f, 0.0f, 0.0f),
// // 		float ni = 1.0f,
// // 		float ns = 1.0,
// // 		float trans = 0.0,
// // 		float metal = 0.0,
// // 		float smooth = 0.0,
// // 		uint indicator = 0)
// // 	{
// // 		this->color = color;
// // 		this->emission = emission;
// // 		this->ni = ni;
// // 		this->ns = ns;
// // 		this->trans = trans;
// // 		this->metal = metal;
// // 		this->smooth = smooth;
// // 		this->indicator = indicator;
// // 	}
    
// // #endif // __cplusplus

//     float3 color; //base color
//     float ni; //index of refracton
//     float3 emission; //light emissition 
//     float ns; //specular exponent
//     float trans; //transparency
//     float metal; //metalness
//     float smooth; //smoothness
//     uint indicator;
// };
/*
void ShadeIt(
inout RayPayload payload,
in float3 normal,
in float3 oldRayDir,
in float3 newRayDir,
uint matIndex
)
{

    // TODO: mat from mat buffer (until we start doing textures but f that, samplers who?)
    Material mat = (Material)0;
    mat.color = float3(1.0, 1.0, 1.0);
    mat.ni = 1.0;
    mat.emission = float3(0.0, 0.0, 0.0);
    mat.ns = 1.0;
    mat.trans = 0.0;
    mat.metal = 0.0;
    mat.smooth = 0.0;
    mat.indicator = 0;

    float tf = abs((1.0 - mat.ni) / (1.0 + mat.ni));
    float3 F0a = abs((1.0 - mat.ni) / (1.0 + mat.ni)).xxx;

    //float3(tf, tf, tf);
    F0a = F0a * F0a;

    float3 F0 = lerp(F0a, mat.color, mat.metal);
    //dvec3 ctSpecular = CookTorance(ray, reflectionRay, minRayResult, downstreamRadiance, minRayResult.shape->mat, currentIOR, F0, kS);
    //TODO: need to keep the current ray
    float3 kS = float3(0, 0, 0);
    float3 cT = max(CookTorance(oldRayDir, newRayDir, normal, mat.ni, F0, kS), 0.0f);
    //blinn phong (ish)
    float3 R = reflect(newRayDir, normal); //   rotateVector(newRay.dir, PI, hr.normal); //TODO: not the right rotate

    float3 V = -oldRayDir;
    float3 N = normal;
    float3 L = newRayDir;

    float3 H = normalize((L + V));

    float diff = max(dot(L, N), 0.0f);
    float spec = max(dot(N, H), 0.0f);
    //float lightsum = diff + spec;
    float3 kD = ((1.0f - kS) * (1.0f - mat.metal)); //*diff;

    payload.accum += mat.emission * payload.masked;
    payload.masked *= mat.color * ((diff * kD) + (cT));
}
*/




// uint wang_hash(inout uint seed)
// {
//     seed = (seed ^ 61) ^ (seed >> 16);
//     seed *= 9;
//     seed = seed ^ (seed >> 4);
//     seed *= 0x27d4eb2d;
//     seed = seed ^ (seed >> 15);
//     return seed;
// }



// float randFloat01(inout uint randomState)
// {
//     uint randUint = wang_hash(randomState);
//     float val = float(randUint) / float(0xFFFFFFFF);
//     return val;
// }

// float3 generateNewRayDir(float3 oldRayDir, in float3 normal, inout uint randomState)
// {
//     //TODO: this was essentially stolen from 
//     //http://raytracey.blogspot.com/2016/11/opencl-path-tracing-tutorial-2-path.html
//     //need to go through and see how the one I wrote was wrong after I confirm this works.
//     float3 w = dot(normal, oldRayDir) < 0.0f ? normal : normal * (-1.0f);

//     /* compute two random numbers to pick a random point on the hemisphere above the hitpoint*/
                    
//     float rand1 = 2.0f * PI * randFloat01(randomState);
//     float rand2 = randFloat01(randomState);

//     float rand2s = sqrt(rand2);

//     /* create a local orthogonal coordinate frame centered at the hitpoint */
//     float3 u = abs(w.x) > 0.1f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f); //axis
//     u = normalize(cross(u, w));
//     float3 v = cross(w, u);
                    
//     return normalize(u * cos(rand1) * rand2s + v * sin(rand1) * rand2s + w * sqrt(1.0f - rand2));
// }



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
    payload.accum = float16_t3(color.x, color.y, color.z);
    return;

    
    //float d = frac(RayTCurrent());


    //payload.accum = float16_t3(d, d, d);
    // return;
    // (void) attrib;
    // // TODO: mat from mat buffer (until we start doing textures but f that, samplers who?)
    // Material mat = (Material)0;
    // mat.color = float3(1.0, 1.0, 1.0);
    // mat.ni = 1.0;
    // mat.emission = float3(0.0, 0.0, 0.0);
    // mat.ns = 1.0;
    // mat.trans = 0.0;
    // mat.metal = 0.0;
    // mat.smooth = 0.0;
    // mat.indicator = 0;

//    float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
//    float3 normal = getWorldSpaceNormal();
    //float3 normal = normalize(cross(ba, ca));

    //TODO: a smooth surface will generate more reflected rays
    //float3 oldRayDir = WorldRayDirection();
    //float3 newRayDir = generateNewRayDir(oldRayDir, normal, payload.seed);

    
    
    

    // Tringle tri = geomdata[InstanceID()][PrimitiveIndex()];
    // //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    // float3 a = tri.verts[0].pos;
    // float3 b = tri.verts[1].pos;
    // float3 c = tri.verts[2].pos;
    
    // float3 ba = b - a;
    // float3 ca = c - a;
    // float3 normal = normalize(cross(ba, ca));
    // normal = mul(normal, ((float3x3) ObjectToWorld4x3()));

    // float3 normal = getWorldSpaceNormal();
    // float3 color = abs(normal);
    
    // payload.accum = float16_t3(color.x, color.y, color.z);
    // return;
    // ShadeIt(
    //     payload,
    //     normal,
    //     oldRayDir,
    //     newRayDir,
    //     0
    // );

    // if(payload.layer >= MAX_RECURSION_LAYERS)
    // {
    //     return;
    // }
    
    // RayDesc ray;
    // ray.Origin = pos;
    // ray.Direction = newRayDir;
    // ray.TMin = 0.001;
    // ray.TMax = 1000;
    // payload.layer++;

    //TraceRay(scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);

    // float3 color = abs(normal);
    // payload.color = float16_t3(color.x, color.y, color.z);
    
    //return;
};

// [shader("closesthit")]
// void ch_red(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
// {
//     (void) attrib;
//     //payload.color = float16_t3(0.0, 0.0, 1.0);
//     //return;
//     payload.color = float16_t3(1, 0, 0);
//     return;
//     //Tringle tri = geomdata[InstanceID()][PrimitiveIndex()];
//     //float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
// }

// [shader("closesthit")]
// void ch_checkerboard(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
// {
//     (void)attrib;
//     //const float  = 10;
    
//     float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

//     int3 xyz = (int3)select(pos < 1, pos -1, pos);
//     bool3 evenSquare = xyz%2;
    
//     bool colorIdx = evenSquare.x ^ evenSquare.y ^ evenSquare.z;// ((xyz.x + xyz.y)%2);
//     // colorIdx = colorIdx | (xyz.z%2);
    
//     // int3 xyz = pos < 1 ? pos : pos-1;
//     // xy.x = pos.x < 0 ? pos.x - 1.0f : pos.x;
//     // xy.y = pos.z < 0 ? pos.z - 1.0f : pos.z;
    
//     // bool colorIdx = 1;//((xy.x + xy.y) % 2) ? true : false;


    

//     float16_t3 color = colorIdx ? float16_t3(0.9, 0.9, 0.9) : float16_t3(0.4, 0.4, 0.4);

//     payload.color = color;
//     return;
// }
 

// [shader("closesthit")]
// void ch_mirror(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
// {
//     (void) attrib;
    
//     float3 pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
//     float3 normal = getWorldSpaceNormal();
//     float3 reflected = reflect(normalize(WorldRayDirection()), normal);
    
//     RayDesc ray;
//     ray.Origin = pos;
//     ray.Direction = reflected;
//     ray.TMin = 0.001;
//     ray.TMax = 1000;
    
//     RayPayload payload2 = (RayPayload) 0;
//     TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload2);
//     payload.color = (payload2.color * 0.7) + float16_t3(0.3, 0.3, 0.3);
//     return;
// }

// [shader("closesthit")]
// void ch_skybox(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
// {
//     (void)attrib;
//     // payload.color = float16_t3(0.211765, 0.780392, 0.94902);
// }
