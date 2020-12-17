# LightRendering

![](data/Screenshots1.png)
<p align="center">
</p>

This is a reference implementation of forward rendering, deferred rendering, and their tiled variants. Its not meant to be a exhaustive engine or fast implementations, but more of something you can copy to a different demo and build upon if wanted. I may decide to speed up some parts of the implementation at some point but for now its just a reference. The GBuffer formats are very unoptimized as well. The implementation currently only works on solid geometry but I will add transparent geometry as well as MSAA to all the techniques.

# Features

- Run time recompilation of C++ code (you cannot change memory layouts without probably causing a crash)
- Run time recompilation of pipelines (changing descriptor layouts in the shader will probably cause a crash)

# How to Build

- clone with --recurse-submodules
- add assimp to the lib folder or comment out all assimp references in the framework_vulkan code
- Run code/build.bat (you can adjust the commands here for shaders and c++ code like O0 or O2). 
- Open the exe in visual studio as a project and set the working directory to the data folder. You can use the sln for debugging the project.

# How to Run

- Follow above on building the code
- Run the exe from the data directory

# TODO

- Fix resizing bug, seems to require translating the window for resize to adjust itself
- Add transparent objects to all renderers
- Add MSAA support to all renderers
- Add GPU profiling and maybe try out some optimizations
