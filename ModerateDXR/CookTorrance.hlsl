// #include "shared.h"


// float Xchi(float x)
// {
//     int i = (int) (x > 0);
//     return (1.0f * (float) (i)) + (-1.0f * ((float) (1 - i)));

// }

// float D(float3 m, float3 n, float3 h, float a)
// {
//     float numerator = a * a * Xchi(dot(h, n));

//     float mndot2 = pow(dot(m, n), 2);

//     float otherPart = (1.0f - mndot2) / mndot2;
//     float denominator = PI * pow(pow(dot(m, n), 2) * ((a * a) + otherPart), 2);

//     return numerator / denominator;
// }


// float fDialectric(float prevIOR, float newIOR, float3 i, float3 o)
// {
//     float f0 = (prevIOR - newIOR) / (prevIOR + newIOR);
//     f0 *= f0;


//     return f0 + ((1.0f - f0) * pow(1.0f - dot(i, o), 5));

// }
// float3 fresnelSchlick(float cosT, float3 f0)
// {
//     return f0 + (1.0f - f0) * pow(1.0f - cosT, 5);
// }

// #define fK 4.0f //absorbtion coefficient

// float fConductor(float matIOR, float3 i, float3 o)
// {
//     float numerator = pow(matIOR - 1.0, 2) + (4.0 * matIOR * pow(1.0 - dot(i, o), 5)) + pow(fK, 2);
//     float denom = pow(matIOR + 1.0, 2) + pow(fK, 2);

//     return numerator / denom;

// }


// float Gp(float3 v, float3 n, float3 h, float a)
// {


//     float vdh = dot(v, h);
//     float chi = Xchi(vdh / dot(v, n));
//     vdh *= vdh;
//     float right = (1.0f - vdh) / vdh;
//     return (chi * 2.0f) / (1.0f + sqrt(1.0f + (a * a * right)));
// }
// float G(float3 wi, float3 wo, float3 n, float3 h, float a)
// {
//     return Gp(wi, n, h, a) * Gp(wo, n, h, a);
// }


// float3 CookTorance(
// 	in float3 oldRayDir,
// 	in float3 newRayDir,
// 	in float3 normal,
// 	//Material mat,
// 	float prevIOR,
// 	const float3 F0,
// 	inout float3 kS)
// {


//     //float2 uv = minRayResult.uv.xy;
	
//     float roughness = 0.4;// 1.0f - mat.smooth;


//     float3 vi = -oldRayDir;
//     float3 vo = newRayDir;
// 	//dvec3 vh = normalize(vi + vo);
//     float3 vn = normal;

// 	/*if (dot(vn, vo) < 0) { //if going into transparent material
// 		vo = -vo;
// 		vo = vo - ((2.0f * vo * vn) / pow(length(vo), 2) * vo);
// 	} else if(dot(vn, vi) < 0) { //if coming out of transparent material
// 		vi = -vi;
// 		vi = vi - ((2.0f * vi * vn) / pow(length(vi), 2) * vi);
// 	}*/
//     float3 vh = normalize(vi + vo);


//     float ndi = dot(vn, vi);

//     float cosT = dot(vo, vn);
//     float sinT = sqrt(1 - cosT * cosT);

// 	// Calculate fresnel
//     float3 fresnel = fresnelSchlick(dot(vh, vi), F0);


// 	// Geometry term
//     float geometry = G(vi, vo, vn, vh, roughness); // GGX_PartialGeometryTerm(viewVector, normal, halfVector, roughness)* GGX_PartialGeometryTerm(sampleVector, normal, halfVector, roughness);
												   
// 												   // Calculate the Cook-Torrance denominator
//     float denominator = 4.0f * (ndi * dot(vh, vn) + 0.05f);
//     kS = fresnel;
	
// 	// Accumulate the radiance
//     return geometry * fresnel * sinT / denominator;
// }