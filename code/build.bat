@echo off

set CodeDir=..\code
set DataDir=..\data
set LibsDir=..\libs
set OutputDir=..\build_win32
set VulkanIncludeDir="C:\VulkanSDK\1.2.135.0\Include\vulkan"
set VulkanBinDir="C:\VulkanSDK\1.2.135.0\Bin"
set AssimpIncludeDir="%LibsDir%\assimp-5.0.1\include"
set AssimpLibDir=%LibsDir%\assimp-5.0.1\lib\RelWithDebInfo

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -EHsc -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4127 -wd4201 -wd4100 -wd4189 -wd4505 -Z7 -FC
set CommonCompilerFlags=-I %VulkanIncludeDir% %CommonCompilerFlags%
set CommonCompilerFlags=-I %LibsDir% -I %AssimpIncludeDir% %CommonCompilerFlags%
REM Check the DLLs here
REM %AssimpLibDir%\assimp-vc142-mt.lib
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib Winmm.lib opengl32.lib DbgHelp.lib d3d12.lib dxgi.lib d3dcompiler.lib 

IF NOT EXIST %OutputDir% mkdir %OutputDir%

pushd %OutputDir%

del *.pdb > NUL 2> NUL

REM USING GLSL IN VK USING GLSLANGVALIDATOR
call glslangValidator -DGBUFFER_VERT=1 -S vert -e main -g -V -o %DataDir%\shader_deferred_gbuffer_vert.spv %CodeDir%\deferred_shaders.cpp
call glslangValidator -DGBUFFER_FRAG=1 -S frag -e main -g -V -o %DataDir%\shader_deferred_gbuffer_frag.spv %CodeDir%\deferred_shaders.cpp
call glslangValidator -DPOINT_LIGHT_VERT=1 -S vert -e main -g -V -o %DataDir%\shader_deferred_point_light_vert.spv %CodeDir%\deferred_shaders.cpp
call glslangValidator -DPOINT_LIGHT_FRAG=1 -S frag -e main -g -V -o %DataDir%\shader_deferred_point_light_frag.spv %CodeDir%\deferred_shaders.cpp
call glslangValidator -DDIRECTIONAL_LIGHT_VERT=1 -S vert -e main -g -V -o %DataDir%\shader_deferred_directional_light_vert.spv %CodeDir%\deferred_shaders.cpp
call glslangValidator -DDIRECTIONAL_LIGHT_FRAG=1 -S frag -e main -g -V -o %DataDir%\shader_deferred_directional_light_frag.spv %CodeDir%\deferred_shaders.cpp

call glslangValidator -DVERTEX_SHADER=1 -S vert -e main -g -V -o %DataDir%\shader_forward_vert.spv %CodeDir%\shader_forward.cpp
call glslangValidator -DFRAGMENT_SHADER=1 -S frag -e main -g -V -o %DataDir%\shader_forward_frag.spv %CodeDir%\shader_forward.cpp

call glslangValidator -DGRID_FRUSTUM=1 -S comp -e main -g -V -o %DataDir%\shader_tiled_forward_grid_frustum.spv %CodeDir%\tiled_forward_shaders.cpp
call glslangValidator -DLIGHT_CULLING=1 -S comp -e main -g -V -o %DataDir%\shader_tiled_forward_light_culling.spv %CodeDir%\tiled_forward_shaders.cpp
call glslangValidator -DTILED_FORWARD_VERT=1 -S vert -e main -g -V -o %DataDir%\shader_tiled_forward_vert.spv %CodeDir%\tiled_forward_shaders.cpp
call glslangValidator -DTILED_FORWARD_FRAG=1 -S frag -e main -g -V -o %DataDir%\shader_tiled_forward_frag.spv %CodeDir%\tiled_forward_shaders.cpp

call glslangValidator -DGRID_FRUSTUM=1 -S comp -e main -g -V -o %DataDir%\shader_tiled_deferred_grid_frustum.spv %CodeDir%\tiled_deferred_shaders.cpp
call glslangValidator -DLIGHT_CULLING=1 -S comp -e main -g -V -o %DataDir%\shader_tiled_deferred_light_culling.spv %CodeDir%\tiled_deferred_shaders.cpp
call glslangValidator -DGBUFFER_VERT=1 -S vert -e main -g -V -o %DataDir%\shader_tiled_deferred_gbuffer_vert.spv %CodeDir%\tiled_deferred_shaders.cpp
call glslangValidator -DGBUFFER_FRAG=1 -S frag -e main -g -V -o %DataDir%\shader_tiled_deferred_gbuffer_frag.spv %CodeDir%\tiled_deferred_shaders.cpp
call glslangValidator -DTILED_DEFERRED_LIGHTING_VERT=1 -S vert -e main -g -V -o %DataDir%\shader_tiled_deferred_lighting_vert.spv %CodeDir%\tiled_deferred_shaders.cpp
call glslangValidator -DTILED_DEFERRED_LIGHTING_FRAG=1 -S frag -e main -g -V -o %DataDir%\shader_tiled_deferred_lighting_frag.spv %CodeDir%\tiled_deferred_shaders.cpp

call glslangValidator -DFRAGMENT_SHADER=1 -S frag -e main -g -V -o %DataDir%\shader_copy_to_swap_frag.spv %CodeDir%\shader_copy_to_swap.cpp

REM USING HLSL IN VK USING DXC
REM set DxcDir=C:\Tools\DirectXShaderCompiler\build\Debug\bin
REM %DxcDir%\dxc.exe -spirv -T cs_6_0 -E main -fspv-target-env=vulkan1.1 -Fo ..\data\write_cs.o -Fh ..\data\write_cs.o.txt ..\code\bw_write_shader.cpp

REM 64-bit build
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% %CodeDir%\light_rendering_demo.cpp -Fmlight_rendering_demo.map -LD /link %CommonLinkerFlags% -incremental:no -opt:ref -PDB:light_rendering_demo_%random%.pdb -EXPORT:Init -EXPORT:Destroy -EXPORT:SwapChainChange -EXPORT:CodeReload -EXPORT:MainLoop
del lock.tmp
call cl %CommonCompilerFlags% -DDLL_NAME=light_rendering_demo -Felight_rendering_demo.exe %LibsDir%\framework_vulkan\win32_main.cpp -Fmlight_rendering_demo.map /link %CommonLinkerFlags%

popd
