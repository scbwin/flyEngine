# flyEngine
flyEngine is an Open Source 3D engine based on the OpenGL and Direct3D graphics APIs. The CPU-part is entirely written in C++, whereas on the GPU GLSL and HLSL are used. Features include Normal Mapping, Terrain Rendering, Procedural Terrain, Procedural Grass, Post Processing (Depth Of Field, Bloom, Lens Flare, Screen Space Reflections, Volumetric Light), basic Frustum Culling and wind animations.

![](https://github.com/fleissna/flyEngine/blob/master/screenshots/MyDX11Window%2014.03.2018%2017_28_13.png)
![](https://github.com/fleissna/flyEngine/blob/master/screenshots/MyDX11Window%2014.03.2018%2017_42_48.png)

## Installation
You have to download/clone and build the dependencies by yourself. Use CMake to resolve them and to generate project files for Visual Studio. flyEngine is built as a static library, make sure to link against it in your application. Two examples are included that demonstrate how to integrate the library, one for OpenGL and another one for DirectX. You can switch between Crytek's Sponza scene and a terrain scene through the SPONZA preprocessor define.

### Software
* Visual Studio 2017 Community Edition 64 Bit (2015 should work as well)
* CMake
### Library Dependencies
* [GLM](https://glm.g-truc.net/0.9.9/index.html)
* [SOIL](https://github.com/kbranigan/Simple-OpenGL-Image-Library)
* [DXUT](https://github.com/Microsoft/DXUT)
* [FX11](https://github.com/Microsoft/FX11)
* [DirectXTex](https://github.com/Microsoft/DirectXTex)
* [DirectXTK](https://github.com/Microsoft/DirectXTK)
* [GLEW](http://glew.sourceforge.net/)
* [Assimp](https://github.com/assimp/assimp)
* [OpenCV](https://github.com/opencv/opencv)
* [AntTweakBar](http://anttweakbar.sourceforge.net/doc/)
* Qt 5.10.1 (Open Source version)

DXUT, FX11, DirectXTex, and DirectXTK repositories contain solution files for Visual Studio 2017. GLM is header-only, SOIL, Assimp and OpenCV are shipped with their own CMake files. Qt offers an installer that contains pre-built binaries. For GLEW and AntTweakBar you can use the pre-build binaries too.

### Contributions
Feel free to contribute if you have any ideas to enhance the engine.
Possible improvements are:
* Rendering API abstraction layer: Write an abstract renderer that encapsulates the logic of the rendering loop and resource allocation. Specific implementations for DirectX/OpenGL/Vulkan should be realized by C++ templates and policy-based design for maximum performance.
* Implement different rendering paths that can be switched at runtime: Forward Renderer, Deferred Renderer, Forward+ Renderer
* Multithreading support
* Physics engine integration (e.g. [Bullet Physics Engine](https://github.com/bulletphysics/bullet3))
* Character animations
* Spatial data structures: The engine should be capable of rendering large outdoor environments for open world games. Possible candidates are Octrees/Quadtrees for static objects and regular grids for dynamic objects. Implement all of them and see what fits best for the application.
* Tessellation for arbitrary objects, not only terrain
* Level of detail system
#### Guidelines
* Stick to the ECS design pattern.
* Do not add any member variables of third-party datatypes to core engine components. E.g. the Material class shouldn't store pointers to DirectX Shader resource views, just store a std::string that contains the path to the texture instead.
* Avoid dynamic memory allocations in the rendering loop if possible.
* Avoid file I/O during the rendering loop. If you have to, do it asynchronously and synchronize if you need to.
* Use Nvidia Nsight for graphics debugging.
* Test your changes and watch for the framerate.

If you have any questions (project setup, engine stuff), send me an e-mail to [p.fleissner@student.tugraz.at](mailto:p.fleissner@student.tugraz.at) or [phips10@gmx.at](mailto:phips10@gmx.at).

Copyright 2018, phipsi f***ing fleissna
