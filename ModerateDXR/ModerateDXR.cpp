#include "common.h"
#include "shared.h"
#include "DxrContext.hpp"
#include "pixtras.hpp"

#include "Raytracable.hpp"
#include "Mesh.hpp"
#include "Instance.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//static DxrContext* ctx;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{





    HMODULE pixLib = LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());



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
    
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ctx);



    vector<Raytracable*> models;
    vector< D3D12_RESOURCE_BARRIER> blasBarriers;
    vector<Instance*> instances;
    vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;

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

    Instance iCapy(capy, transpose(scale(translate(rotate(mat4x4(1), radians(45.0f), fvec3(0, 1, 0)), fvec3(0, -3, 0)), fvec3(0.5, 0.5, 0.5))));
    instances.push_back(&iCapy);
    Instance iB(cube, glm::transpose(glm::translate(mat4x4(1), fvec3(0, 0, -2))), 2);
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

    for (Instance* i : instances) instanceDescs.push_back(i->GetInstanceDesc());

    ctx->commandList->ResourceBarrier((uint32_t)blasBarriers.size(), blasBarriers.data());
    ctx->ExecuteCommandList();
    ctx->Flush();

    ctx->constants->camPos = float3(0, 1, 5);
    ctx->constants->fov = glm::radians(90.0f);
    ctx->constants->lookAt = float3(0, 0, 0);
    ctx->constants->ct = 0.0f;

    //TODO: if the number of instances changes, this will need to be redone
    ctx->CreateTlasResources(instanceDescs);


    ctx->startTimePoint = std::chrono::high_resolution_clock::now();
    ctx->lastFrameTimePoint = std::chrono::high_resolution_clock::now();

    MSG msg;
    msg.message = WM_NULL;
    while (msg.message != WM_QUIT)
    { 
        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            OutputDebugStringF("%u\t0x%08x\n", msg.message, msg.message);

            if (msg.message == WM_SIZE)
            {
                DispatchMessage(&msg);
                break;
                ;// OutputDebugStringF("width%u, height%u\n", LODWORD(msg.lParam), HIDWORD(msg.lParam));
            }
            DispatchMessage(&msg);
        }
        PostMessage(hwnd, WM_PAINT, 0, 0);

        RECT rect;
        //Getrect
        GetClientRect(hwnd, &rect);
        UINT width = rect.right - rect.left;
        UINT height = rect.bottom - rect.top;
        ctx->SetResize(width, height);
        auto nowTimePoint = std::chrono::high_resolution_clock::now();

        auto cTDurr = std::chrono::duration<double>(nowTimePoint - ctx->startTimePoint);
        auto dTDurr = std::chrono::duration<double>(nowTimePoint - ctx->lastFrameTimePoint);

        ctx->lastFrameTimePoint = nowTimePoint;

        float ct = float(cTDurr.count());
        float dt = float(dTDurr.count());

        iCapy.transform = glm::rotate(iCapy.transform, radians(90.0f) * dt, fvec3(0, 1, 0));
        instanceDescs[0] = iCapy.GetInstanceDesc();


        ctx->UploadInstanceDescs(instanceDescs);
        ctx->Render(ct);
    }
    return (int) msg.wParam;
}


static bool firstSize = true;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    DxrContext* ctx = (DxrContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message)
    {
    case WM_CREATE:
        OutputDebugStringF("create %u, height %u\n", LODWORD(lParam), HIDWORD(lParam));
        break;
    case WM_SIZE:
        {
        if (firstSize) //god i hate this
        {
            firstSize = false;
            break;
        }
        RECT r;
        GetClientRect(hwnd, &r);
        uint w = r.right - r.left;
        uint h = r.bottom - r.top;
        //if(ctx)
            //ctx->SetResize(w, h);
        //OutputDebugStringF("size width %u, height %u\n", LODWORD(lParam), HIDWORD(lParam));
        }
        break;
    case WM_PAINT:
        OutputDebugStringF("paint %u, height %u\n", LODWORD(lParam), HIDWORD(lParam));
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