#include "common.h"
#include "shared.h"
#include "DxrContext.hpp"
#include "pixtras.hpp"

#include "Raytracable.hpp"
#include "Mesh.hpp"
#include "Instance.hpp"

#include <functional>



/* 
TODO list

better sampling handling
denoiser + ^
async build/dispatch
textures
spectrum rays
image(/video?) output
skeleton lmao
*/


struct WindowUserData
{
    DxrContext* ctx;
    function<void(HWND hwnd)>* wm_paint_fn;

};


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//static DxrContext* ctx;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    OutputDebugStringF("Starting\n");
#if _DEBUG
    HMODULE pixLib = LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
#endif
    const uint defaultWidth = 1280;
    const uint defaultHeight = 1280;

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
    HWND hwnd = CreateWindow(registeredClass, L"", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE, 0, 0, defaultWidth, defaultHeight, NULL, NULL, hInstance, NULL);

    DxrContext* ctx = new DxrContext(hwnd, defaultWidth, defaultHeight);
    



    vector<Raytracable*> models;
    vector< D3D12_RESOURCE_BARRIER> blasBarriers;
    vector<Instance*> instances;


    //Mesh sustoe = Mesh("sustoe")

    Mesh cube = Mesh("cube.obj");
    models.push_back(&cube);
    Mesh sphere = Mesh("sphere.obj");
    models.push_back(&sphere);
    Mesh capy = Mesh("capybara.obj");
    models.push_back(&capy);

    for (Raytracable* r : models) r->CreateResources(ctx->device);
    for (Raytracable* r : models) r->AddGeomSRV(ctx->device, ctx->descHeap);

    ctx->ResetCommandList();

    for (Raytracable* r : models) blasBarriers.push_back(r->BuildBLAS(ctx->device, ctx->commandList));

    Instance iCapy(
        capy, 
        Transform{ .translate = fvec3(0, 0, 0), .rotateDegrees = 30 }(),
        HG_GLASS, //hit group
        2);//instance id
    instances.push_back(&iCapy);

    //Instance iCapy2(
    //    capy,
    //    Transform{ .translate = fvec3(5, 5, 0), .rotateDegrees = 45}(),
    //    HG_MIRROR, //hit group
    //    2);//instance id
    //instances.push_back(&iCapy2);

    Instance iLightCube(
        cube,
        Transform{ .translate = fvec3(-6, 6, 0), .rotateDegrees = 45, .rotateAxis = fvec3(0, 0, 1), .scale = fvec3(2, 0.1, 2) }(),
        HG_LIGHT,
        0);
    instances.push_back(&iLightCube);

    Instance iFloor(
        cube,
        Transform{ .translate = fvec3(0, -1, 0), .rotateDegrees = 0, .rotateAxis = fvec3(0, 1, 0), .scale = fvec3(20, 1, 20)}(),
        HG_CHECKERED, //hit group
        0);//instance id
    instances.push_back(&iFloor);

    for (Instance* i : instances) ctx->instanceDescs.push_back(i->GetInstanceDesc());
    ctx->commandList->ResourceBarrier((uint32_t)blasBarriers.size(), blasBarriers.data());
    ctx->CreateScreenSizedResources();
    ctx->PopulateAndCopyRandBuffer();
    ctx->ExecuteCommandList();
    ctx->Flush();

    ctx->constants->camPos = float3(-4, 8, 4);
    ctx->constants->fov = glm::radians(90.0f);
    ctx->constants->lookAt = float3(0, 2.5, 0);
    ctx->constants->ct = 0.0f;

    //TODO: if the number of instances changes, this will need to be redone
    ctx->CreateTlasResources(ctx->instanceDescs);


    ctx->startTimePoint = std::chrono::high_resolution_clock::now();
    ctx->lastFrameTimePoint = std::chrono::high_resolution_clock::now();
    function<void(HWND hwnd)> wm_paint_fn = [ctx, &instances](HWND hwnd) {
        auto nowTimePoint = std::chrono::high_resolution_clock::now();


        auto cTDurr = std::chrono::duration<double>(nowTimePoint - ctx->startTimePoint);
        auto dTDurr = std::chrono::duration<double>(nowTimePoint - ctx->lastFrameTimePoint);

        ctx->lastFrameTimePoint = nowTimePoint;

        float ct = float(cTDurr.count());
        float dt = float(dTDurr.count());


        //ctx->constants->camPos = float3(sin(ct) * 6, 6, cos(ct) * 6);
        //ctx->PopulateAndCopyRandBuffer();
        ctx->constants->ct = ct;
        ctx->constants->frameNumber++;
        ctx->alterInstanceTransform(0, glm::rotate(mat4(1), radians(90.0f) * dt, fvec3(0, 1, 0)));;
        //iCapy.transform = glm::rotate(iCapy.transform, radians(90.0f) * dt, fvec3(0, 1, 0));
        // 
        //auto g = ctx->instanceDescs[0].Transform;
        //glm::mat3x4(ctx->instanceDescs[0].Transform);
        ctx->UploadInstanceDescs();
        if (ctx->constants->frameNumber % NUM_SAMPLES == 0)
        {
            //OutputDebugStringF("done with that frame\n");
            //system("pause");
        }
        ctx->Render(ct);
        //system("pause");
        };



    WindowUserData wud;
    wud.ctx = ctx;
    wud.wm_paint_fn = &wm_paint_fn;
    //LRESULT(__stdcall * y)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) = WndProc;
    //auto wm_paint_fn = (void*)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {};

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&wud);

    MSG msg;
    msg.message = WM_NULL;
    while (msg.message != WM_QUIT)
    { 
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        PostMessage(hwnd, WM_PAINT, 0, 0);
    }
    return (int) msg.wParam;
}


static bool firstSize = true;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    //DxrContext* ctx = (DxrContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    WindowUserData* wud = (WindowUserData*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    //TODO: make this userdata a lambda lmao

    switch (message)
    {
    case WM_CREATE:
        //OutputDebugStringF("create %u, height %u\n", LODWORD(lParam), HIDWORD(lParam));
        break;
    case WM_SIZE:
        break;
    case WM_EXITSIZEMOVE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            UINT width = rect.right - rect.left;
            UINT height = rect.bottom - rect.top;
            //wud->ctx->SetResize(width, height);
        }
        break;
    case WM_PAINT:
        wud->wm_paint_fn->operator()(hwnd);
        break;
    case WM_QUIT:
    case WM_DESTROY:
        //OutputDebugStringF("exiting?\n");
        exit(0);
        break;
    default:
        break;

    }
    return DefWindowProc(hwnd, message, wParam, lParam);
    //return 0;
}