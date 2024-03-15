#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <string>
#include <format>
#include <chrono>

#include <windows.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <windowsx.h>
#include <wrl.h>
#include <dxgi1_6.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;
using namespace glm;
using namespace Microsoft::WRL;

constexpr uint MAX_NUM_DESCRIPTORS = 100;

inline void OutputDebugStringF(const char* fmt, ...)
{
    char buffer[1 << 16];
    va_list args;
    va_start(args, fmt);
    int rc = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    OutputDebugStringA(buffer);
}

inline void ThrowIfFailed(HRESULT hr, uint line, const char* file)
{
    if (FAILED(hr)) 
    {
        char buff[500];
        snprintf(buff, sizeof(buff), "HR 0X%08X at line %u of %s\n", hr, line, file);
        OutputDebugStringA(buff);
        throw std::runtime_error(string(buff));
    }
}

inline void ThrowIfError(HRESULT hr, const char* errorMessage, uint line, const char* file)
{
    if (FAILED(hr))
    {
        char buff[500];
        snprintf(buff, sizeof(buff), "HR 0X%08X at line %u of %s\n%s\n", hr, line, file, errorMessage);
        OutputDebugStringA(buff);
        throw std::runtime_error(string(buff));
    }
}


#define TIF(hr) ThrowIfFailed(hr, __LINE__, __FILE__)
#define TIE(hr, txt) ThrowIfError(hr, txt, __LINE__, __FILE__);

#define SAFE_RELEASE(p) if (p) (p)->Release() //unless it doesn't have release ugh
