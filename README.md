# Axe
Axe is a cross multi-platform game engine(a slowly-developed weekend project, not for production), aiming at code readability and learning:
 - c++11/17/20, OOP, STL, template, multi-threading, reflection, CMake cross-platform(Windows, Linux, ...)
 - underlying details about modern graphics apis (Vulkan, D3D12) and shader language: hlsl, glsl
 - software architecture of modern game engine (abstraction, layering, compile/edit/runtime/loop -time designs)
 - various graphics features: forward, deferred pipeline, shadow, GI, ...
 - ...

> Learn by coding!

## Platform

|Platform|Compiler| Graphics API|
|---|---|---|
|Windows| Clang>=15, MSVC>=17(2022)| Vulkan>=1.3, Direct3D 12|
|Linux| Clang>=15, GCC>=11 | Vulkan>=1.3|


## Requirements
- [CMake>=3.20](https://cmake.org/download/)
- [Python>=3.9](https://www.python.org/downloads/)
- [VulkanSDK>=1.3](https://vulkan.lunarg.com/) ([start on linux](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started.html), [start on windows](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html))
- Windows 10/11 SDK containing [Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics) if on Windows

## How to build

run the python script that wraps cmake&build commands: (NOTE: `cmake` added to PATH is required)

```shell
# Use --[msvc(default)|clang|gcc] to specify an available compiler, e.g.,
> python3 generate_project.py --clang
```

```shell
> python3 generate_project.py --gcc
```

```shell
> python3 generate_project.py --msvc # VS2022 is required 
```

Axe still uses #include style, instead of c++20 module :(. See ["*When can we begin to use modules?*"](https://github.com/royjacobson/modules-report) for more details. 

## Features

<img src="Documents/Images/Arch.png" alt="Game Engine Architecture" width=360 align="right"/>

Axe is basically designed according to the architecture shown right. Thanks to the powerful standard libraries, and lots of high-quality open source third-party libraries, it can be built directly from the core layer. Runtime of Axe has # well-designed layers of one-way dependency:

- **00Core**
   - (WIP) High performance math library for graphics based on [glm](https://github.com/g-truc/glm)
   - (WIP) High performance memory management base on [mimalloc](https://github.com/microsoft/mimalloc)
   - (WIP) Cross-platform logging system based on [spdlog](https://github.com/gabime/spdlog)
   - (WIP) Cross-platform windows management based on [SDL](https://github.com/libsdl-org/SDL)
   - (WIP) Reflection of C++ based on ...
- **02Rhi**(Rendering Hardware Interface)
  - (WIP) wrap multi modern rendering apis with [WebGPU](https://www.w3.org/TR/webgpu/) style, supporting D3D12, Vulkan
  - (WIP) Dynamic Descriptor Management.
  - (WIP) Memory Management based on [VulkanMemoryAllocation](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator).
  - (WIP) Multi-threaded Command Buffer Generation
- **03Resource**
  - (WIP) Multi-threaded and asynchronous resource loading
  - (WIP) Cross-compile Shader based on [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler), [glslang](https://github.com/KhronosGroup/glslang)/[shaderc](https://github.com/google/shaderc) and [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross).
  - (WIP) Universal scene description based on [USD](https://github.com/PixarAnimationStudios/USD) of pixar
  - (WIP) Universal material description based on [MaterialX](https://github.com/AcademySoftwareFoundation/MaterialX)
- **04RenderGraph**
- **06Pipeline**
  - (WIP) Modern deferred rendering pipeline based on visibility buffer
  - (WIP) Global Illumination system with an unlimited number of light sources and minimal memory footprint
  - (WIP) Integrated with shadertoy seamlessly
- **08System**
  - (WIP) Animation System based on ...
  - (WIP) Physics System based on ...
  - (WIP) ECS(Entity-Component-System) base on [entt](https://github.com/skypjack/entt)
  - (WIP) GUI system based on [Dear imgui](https://github.com/ocornut/imgui)
- **10Scene**
  - (WIP) Partitioning, Culling ...
- **12GamePlay**
  - (WIP) Lua Scripting System based on ...
  - (WIP) Camera ...
- **14App**
  - .

See [Documents/Runtime.md](/Documents/Runtime.md) and [Documents/AssetSystem.md](/Documents/AssetSystem.md) for more details.

## Dependencies

See [Documents/ThirdParty.md](/Documents/ThirdParty.md) for more details.

## License
MIT License