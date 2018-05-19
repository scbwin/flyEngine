# flyEngine
flyEngine is an Open Source 3D engine based on the OpenGL and Direct3D graphics APIs. The CPU-part is entirely written in C++, whereas on the GPU GLSL and HLSL are used. Features include Normal Mapping, Terrain Rendering, Procedural Terrain, Procedural Grass, Post Processing (Depth Of Field, Bloom, Lens Flare, Screen Space Reflections, Volumetric Light), Hierarchical View Frustum Culling, wind animations, parallax and relief mapping.

![](https://github.com/fleissna/flyEngine/blob/master/screenshots/screenshot0.png)
![](https://github.com/fleissna/flyEngine/blob/master/screenshots/screenshot1.png)
![](https://github.com/fleissna/flyEngine/blob/master/screenshots/screenshot2.png)

## Installation
You have to download/clone and build the dependencies by yourself. Use CMake to resolve them and to generate project files for Visual Studio. flyEngine is built as a static library, make sure to <s>link against it in your application</s> include it with CMake's ```find_package```. A few examples are included that demonstrate how to integrate the library. There are currently some pre-processor defines that allow to switch between different scenes.

### Software
* Visual Studio 2017 Community Edition 64 Bit (2015 should work as well)
* CMake
### Library Dependencies
* [GLM](https://glm.g-truc.net/0.9.9/index.html)
* [Assimp](https://github.com/assimp/assimp)
* [SOIL](https://github.com/kbranigan/Simple-OpenGL-Image-Library)
* [GLEW](http://glew.sourceforge.net/)
* Qt 5.10.1 (Open Source version)
* [AntTweakBar](http://anttweakbar.sourceforge.net/doc/)
#### Optional
* [Bullet Physics](https://github.com/bulletphysics/bullet3)
* [DXUT](https://github.com/Microsoft/DXUT)
* [FX11](https://github.com/Microsoft/FX11)
* [DirectXTex](https://github.com/Microsoft/DirectXTex)
* [DirectXTK](https://github.com/Microsoft/DirectXTK)
* [OpenCV](https://github.com/opencv/opencv)

DXUT, FX11, DirectXTex, and DirectXTK repositories contain solution files for Visual Studio 2017. GLM is header-only, SOIL, Assimp and OpenCV are shipped with their own CMake files. Qt offers an installer that contains pre-built binaries. For GLEW and AntTweakBar you can use the pre-build binaries too.

## Conventions
flyEngine uses a right-handed coordinate system. X is right, Y is up and the camera points towards negative Z. Column-major matrices are used throughout the engine, regardless of the underlying rendering API. Matrix multiplication order goes from right to left, a vector being the right-most term.

## Contributions
Feel free to contribute if you have any ideas to enhance the engine.
Possible improvements are:
* <s>Rendering API abstraction layer: Write an abstract renderer that encapsulates the logic of the rendering loop and resource allocation. Specific implementations for DirectX/OpenGL/Vulkan should be realized by C++ templates and policy-based design for maximum performance.</s>
* Implement different rendering paths that can be switched at runtime: Forward Renderer, Deferred Renderer, Forward+ Renderer
* Multithreading support
* <s>Physics engine integration (e.g. [Bullet Physics Engine](https://github.com/bulletphysics/bullet3))</s>
* Character animations
* Spatial data structures: The engine should be capable of rendering large outdoor environments for open world games. Possible candidates are Octrees/<s>Quadtrees</s> for static objects and regular grids for dynamic objects. Implement all of them and see what fits best for the application.
* Tessellation for arbitrary objects, not only terrain
* Level of detail system
## Guidelines
* Development goes into "Renderer" and "OpenGLAPI", the classes "RenderingSystemOpenGL" and "RenderingSystemDX11" are considered deprecated, but are further maintained for compatibility. 
* Stick to the ECS design pattern.
* Do not add any member variables of third-party datatypes to core engine components. E.g. the Material class shouldn't store pointers to DirectX Shader resource views, just store a std::string that contains the path to the texture instead.
* Avoid dynamic memory allocations in the rendering loop if possible.
* Avoid file I/O during the rendering loop. If you have to, do it asynchronously and synchronize if you need to.
* Use Nvidia Nsight for graphics debugging.
* Test your changes and always watch out for frame timings.

If you have any questions (project setup, engine stuff), send me an e-mail to [p.fleissner@student.tugraz.at](mailto:p.fleissner@student.tugraz.at) or [phips10@gmx.at](mailto:phips10@gmx.at).

Copyright 2018, phipsi "frustum" fleissna
