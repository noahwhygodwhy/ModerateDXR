#include "common.h"
#include "shared.h"
#include "DxrContext.hpp"
#include "pixtras.hpp"

#include "Raytracable.hpp"
#include "Mesh.hpp"
#include "Instance.hpp"

#include <functional>
//#include "../imgui/imgui.h"
//#include "../imgui/backends/imgui_impl_win32.h"
//#include "../imgui/backends/imgui_impl_dx12.h"



//void startImgui(HWND hwnd, DxrContext ctx)
//{
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO();
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
//
//    // Setup Platform/Renderer backends
//    ImGui_ImplWin32_Init(YOUR_HWND);
//
//    ImGui_ImplDX12_Init(YOUR_D3D_DEVICE, NUM_FRAME_IN_FLIGHT, YOUR_RENDER_TARGET_DXGI_FORMAT,
//        YOUR_SRV_DESC_HEAP,
//        // You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
//        YOUR_CPU_DESCRIPTOR_HANDLE_FOR_FONT_SRV,
//        YOUR_GPU_DESCRIPTOR_HANDLE_FOR_FONT_SRV);
//}


struct WindowUserData
{
    DxrContext* ctx;
    function<void(HWND hwnd)>* wm_paint_fn;

};


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//static DxrContext* ctx;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{

#if _DEBUG
    HMODULE pixLib = LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
#endif
    const uint defaultWidth = 800;
    const uint defaultHeight = 600;

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

#define cubeInstanceId 0

#define ch_hitgroup_a 0
#define ch_hitgroup_b 1
#define ch_hitgroup_c 2



    Mesh cube = Mesh("cube.obj");
    models.push_back(&cube);
    Mesh sphere = Mesh("sphere.obj");
    models.push_back(&sphere);

    for (Raytracable* r : models) r->CreateResources(ctx->device);
    for (Raytracable* r : models) r->AddGeomSRV(ctx->device, ctx->descHeap);

    ctx->ResetCommandList();

    for (Raytracable* r : models) blasBarriers.push_back(r->BuildBLAS(ctx->device, ctx->commandList));


    ctx->commandList->ResourceBarrier((uint32_t)blasBarriers.size(), blasBarriers.data());
    ctx->ExecuteCommandList();
    ctx->Flush();

    ctx->constants->camPos = float3(5, 5, 5);
    ctx->constants->fov = glm::radians(30.0f);
    ctx->constants->lookAt = float3(0, 0.5, 0);
    ctx->constants->ct = 0.0f;


    Instance instance1(
        cube,
        glm::transpose(glm::translate(mat4x4(1.0f), fvec3(0.0f, 0.0f, 0.0f))),
        ch_hitgroup_a, //hit group 
        0);//instance id

    Instance instance2(
        cube,
        glm::transpose(glm::translate(glm::rotate(mat4x4(1.0f), radians(45.0f), vec3(1, 1, 1)), fvec3(1.0f, 0.1f, 0.1f))),
        ch_hitgroup_b, //hit group 
        0);//instance id
    Instance instance3(
        sphere,
        glm::transpose(glm::translate(glm::rotate(mat4x4(1.0f), radians(45.0f), vec3(3, 1, 3)), fvec3(1.0f, 0.1f, 0.1f))),
        ch_hitgroup_b, //hit group 
        1);//instance id


    instances.push_back(&instance1);
    instances.push_back(&instance2);
    instances.push_back(&instance3);

    for (Instance* i : instances) ctx->instanceDescs.push_back(i->GetInstanceDesc());

    //TODO: if the number of instances changes, this will need to be redone
    ctx->CreateTlasResources(ctx->instanceDescs);
    ctx->UploadInstanceDescs(ctx->instanceDescs);

    ctx->startTimePoint = std::chrono::high_resolution_clock::now();
    ctx->lastFrameTimePoint = std::chrono::high_resolution_clock::now();

    function<void(HWND hwnd)> wm_paint_fn = [ctx, &instances](HWND hwnd) {

        auto nowTimePoint = std::chrono::high_resolution_clock::now();


        auto cTDurr = std::chrono::duration<double>(nowTimePoint - ctx->startTimePoint);
        auto dTDurr = std::chrono::duration<double>(nowTimePoint - ctx->lastFrameTimePoint);

        ctx->lastFrameTimePoint = nowTimePoint;

        float ct = float(cTDurr.count());
        float dt = float(dTDurr.count());
        ctx->constants->ct = ct;
        ctx->Render(ct);
    };

    WindowUserData wud;
    wud.ctx = ctx;
    wud.wm_paint_fn = &wm_paint_fn;

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
    WindowUserData* wud = (WindowUserData*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message)
    {
    case WM_PAINT:
        wud->wm_paint_fn->operator()(hwnd);
        break;
    case WM_QUIT:
    case WM_DESTROY:
        exit(0);
        break;
    default:
        break;

    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}