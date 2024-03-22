## What

This is the branch for a reproducing case for a bug I'm trying to figure out.

For some reason, within a hitgroup, instanceID seems to be shader wave wide, leading to geometry data accessed via instance ID to not be correct. If I change the instance desc's hitgroups to be different, it works just fine, but when they're in the same hitgroup, it breaks.

This is colored by the normals calculated from vertices accessed by geometryData\[InstanceID()\]\[PrimitiveIndex()\]
![image](https://github.com/noahwhygodwhy/ModerateDXR/assets/9063267/9f300b8a-bcc3-43db-bc54-93982f9fef81)

## Getting Started

After cloning, the vs project should just be ready to build/run.

## What pt 2

The main function is in ModerateDXR.cpp. It 
1. loads pix
2. sets up the basic window
3. creates a directX context
    - adapter/device
    - commandqueue/swapchain
    - descheap/rootsig/pipeline
    - framebuffer/constantbuffer
    - commandlist/allocator/fence
5. imports models from obj files
    - cube:   12 triangle cube
    - sphere: 80 triangle icosphere
7. builds BLAS's for said models
8. inits/uploads instance descs
10. then in a loop:
    - builds TLAS
    - dispatchRay()s
    - copies output buffer to swapchain's backbuffer
    - presents 
