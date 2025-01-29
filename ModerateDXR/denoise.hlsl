#include "shared.h"


globallycoherent RWTexture2D<float4> colorIn : register(u0);
StructuredBuffer<HitInfo> hitInfo            : register(t0);
RWTexture2D<float16_t4> colorOut             : register(u1);






bool inBounds(in uint2 dims, in uint2 coord)
{
    return (all(coord >= uint2(0, 0)) && all(coord < dims));
}

uint getGausianVal(in uint2 xy)
{
    const uint gaussian[5] = {1, 4, 6, 4, 1};
    return (gaussian[xy.x] * gaussian[xy.y]);
}

uint32_t bits(uint number)
{
    if(number == 32)
    {
        return -1;
    }
    return (1u << number)-1;
}




bool shouldBlur(in HitInfo myHitInfo, in uint2 dims, in uint2 coord, in uint flatIndex)
{
    //return true;
    return inBounds(dims, coord);//; && 
    //         hitInfo[flatIndex].instanceID == myHitInfo.instanceID && 
    //         hitInfo[flatIndex].didRefraction == myHitInfo.didRefraction && 
    //         hitInfo[flatIndex].matID == myHitInfo.matID;
}



float3 blur(uint2 pixelID, uint2 dims)
{
    uint w = dims.x;
    uint h = dims.y;

    uint flatIndex = (pixelID.y * w) + pixelID.x;

    float3 newColor = float3(0, 0, 0);
    uint gaussianSum = 0;

    HitInfo myHitInfo = hitInfo[flatIndex];

    bool anyblurs = false;
    for(uint x = 0; x < 5; x++)
    {
        for(uint y = 0; y < 5; y++)
        {
            uint2 coord = pixelID + uint2(x-2, y-2);
            if(shouldBlur(myHitInfo, dims, coord, flatIndex))
            {
                uint myGaus = getGausianVal(uint2(x, y));
                gaussianSum += myGaus;
                newColor += (float(myGaus)) * colorIn[coord].xyz;
            }
            else
            {
                anyblurs = true;
            }
        }
    }
    newColor = newColor / float(gaussianSum);
    return newColor;
}

[numthreads(8, 8, 1)]
void Denoise(uint3 threadID : SV_DispatchThreadID)
{
    uint2 pixelID = threadID.xy;
    /*float3 finalColor = float3(0, 0, 0);
    for (uint i = 0; i < FREQUENCIES; i++)
    {
        // TODO:
        // "denoising" lmao
        finalColor += (freqVals[i][pixelID] / FREQUENCIES);
    }*/
    
    //TODO: check if pixelID is in bounds




    uint w, h;
    colorIn.GetDimensions(w, h);
    if (any(pixelID >= uint2(w, h)))
    {
        return;
    }


    uint2 dims = uint2(w, h);

    float3 newColor = blur(pixelID, dims);

    // uint flatIndex = (pixelID.y * w) + pixelID.x;

    // float3 newColor = float3(0, 0, 0);
    // uint gaussianSum = 0;

    // HitInfo myHitInfo = hitInfo[flatIndex];

    // bool anyblurs = false;
    // for(uint x = 0; x < 5; x++)
    // {
    //     for(uint y = 0; y < 5; y++)
    //     {
    //         uint2 coord = pixelID + uint2(x-2, y-2);
    //         if(shouldBlur(myHitInfo, dims, coord, flatIndex))
    //         {
    //             uint myGaus = getGausianVal(uint2(x, y));
    //             gaussianSum += myGaus;
    //             newColor += (float(myGaus)) * colorIn[coord].xyz;
    //         }
    //         else
    //         {
    //             anyblurs = true;
    //         }
    //     }
    // }
    // newColor = newColor / float(gaussianSum);
    // if(anyblurs)
    // {
    //     newColor = float3(float(pixelID.x) / float(w), float(pixelID.y) / float(h), 0.0);
    // }



    colorOut[pixelID] = float16_t4(newColor.x, newColor.y, newColor.z, 1.0f); //float16_t4(finalColor, finalColor, finalColor, 1.0);
    // if(pixelID.x %2)
    // {
    //     colorOut[pixelID].x = 1.0f;
    // }
}