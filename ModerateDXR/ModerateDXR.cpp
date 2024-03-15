#include "common.h"
#include "shared.h"
#include "DxrContext.hpp"
#include "pixtras.hpp"

#include "Raytracable.hpp"
#include "Mesh.hpp"
#include "Instance.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static DxrContext* ctx;
static ConstantBufferStruct* constants;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{




    HMODULE pixLib;

    pixLib = LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());



    const uint defaultWidth = 800;
    const uint defaultHeight = 600;

    HWND hwnd = nullptr;
    WNDCLASSEX classDescriptor = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = NULL,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = NULL,
        .lpszMenuName = L"",
        .lpszClassName = L"ModerateDxr"
    };
    
    const LPWSTR registeredClass = (LPWSTR)RegisterClassEx(&classDescriptor);

    hwnd = CreateWindow(registeredClass, L"", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 0, 0, defaultWidth, defaultHeight, NULL, NULL, hInstance, NULL);

    ctx = new DxrContext(hwnd, defaultWidth, defaultHeight);

    vector<Raytracable*> models;
    vector< D3D12_RESOURCE_BARRIER> blasBarriers;
    //auto y = foo;
    //void(*z)(Raytracable) = foo;
    //auto x = [scene](void(*fn)(Raytracable r)) {std::for_each(scene.begin(), scene.end(), fn(r)); };

    Mesh cube = Mesh("cube.obj");
    models.push_back(&cube);
    Mesh sphere = Mesh("sphere.obj");
    models.push_back(&sphere);


    for (Raytracable* r : models) r->CreateResources(ctx->device);
    for (Raytracable* r : models) r->AddGeomSRV(ctx->device, ctx->descHeap);

    ctx->ResetCommandList();
    OutputDebugStringF("building blas5\n");
    for (Raytracable* r : models) blasBarriers.push_back(r->BuildBLAS(ctx->device, ctx->commandList));

    vector<Instance*> instances;
    Instance iA(sphere, mat3x4(1));
    instances.push_back(&iA);
    Instance iB(sphere, glm::transpose(glm::translate(mat4x4(1), fvec3(1, 1, 1))), 2);
    instances.push_back(&iB);

    Instance instanceCube(
        cube,
        glm::transpose(
            glm::scale(
                glm::translate(mat4x4(1.0f), fvec3(0, -4.5, 0)),
                fvec3(100, 1, 100)
            )
        ),
    3);
    instances.push_back(&instanceCube);




    vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    for (Instance* i : instances) instanceDescs.push_back(i->GetInstanceDesc());

    
    ctx->commandList->ResourceBarrier((uint32_t)blasBarriers.size(), blasBarriers.data());
    ctx->ExecuteCommandList();
    ctx->Flush();
    //scan

    constants = (ConstantBufferStruct*)ctx->MapConstantBuffer();

    constants->camPos = float3(0, 0, 5);
    constants->fov = glm::radians(90.0f);
    constants->lookAt = float3(0, 0, 0);
    constants->ct = 0.0f;


    //TODO: if the number of instances changes, this will need to be redone
    ctx->CreateTlasResources(instanceDescs);


    MSG msg;
    msg.message = WM_NULL;
    while (msg.message != WM_QUIT)
    {

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    case WM_SIZING:
        //TODO:
        break;
    case WM_PAINT:
        //TODO: ct?
        ctx->Render(0);
        break;
    case WM_QUIT:
    case WM_DESTROY:
        //OutputDebugStringF("exiting?\n");
        exit(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}