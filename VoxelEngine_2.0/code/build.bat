@echo off

REM For Visual Studio Code
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
c:;cd Users\georg\source\repos\VoxelEngine_2.0\VoxelEngine_2.0\code

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -fp:fast -O2 -Oi -WX -W4 -wd4324 -wd4505 -wd4456 -wd4457 -wd4063 -wd4702 -wd4201 -wd4100 -wd4189 -wd4459 -wd4127 -wd4311 -wd4302 -FC -Z7 
set CommonCompilerFlags=-DVOXEL_ENGINE_INTERNAL=1 %CommonCompilerFlags%
set CommonCompilerFlags=-DGLEW_STATIC %CommonCompilerFlags%
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib Winmm.lib opengl32.lib glew32s.lib -ignore:4099

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM Asset file builder build
REM cl %CommonCompilerFlags% -EHsc -D_CRT_SECURE_NO_WARNINGS "..\VoxelEngine_2.0\code\asset_packer.cpp" /link %CommonLinkerFlags%

REM Preprocessor
cl %CommonCompilerFlags% -EHsc -D_CRT_SECURE_NO_WARNINGS "..\VoxelEngine_2.0\code\preprocessor.cpp" /link %CommonLinkerFlags%
pushd ..\VoxelEngine_2.0\code
..\..\build\preprocessor.exe > voxel_engine_preprocessor_generated.h
popd

REM 64-bit build
cl %CommonCompilerFlags% "..\VoxelEngine_2.0\code\win_voxel_engine.cpp" -Fmwin_voxel_engine.map /link -PDB:win_voxel_engine.pdb %CommonLinkerFlags%

popd