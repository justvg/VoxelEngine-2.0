## Description

*The code has NOT been polished and is provided "as is". There are a lot of code that are redundant and there are tons of improvements that can be made.*

This is the second attempt to write a voxel engine for RPG game. The first version is here https://github.com/justvg/VoxelEngine/. <br/>
This time I want to write the whole engine without using any libraries. At the moment I use only GLEW and <math.h>, and I want to get rid of them eventually. Also this time I think about the architecture of the engine a little bit more. <br/>
<br/>
<br/>
Inspired by handmadehero.org and Cube World game.
<br/>
<br/>
The engine implements the following (and more):
- Chunk system management
- Entity system
- SIMD math lib
- Collision detection (sweep and prune: ellipsoid/polygon soup; ray/polygon soup)
- Job system
- Asset packer
- CUB, BMP, WAV file loaders
- Fonts
- Simple sound mixing
- Rendering
- Skeletal animation
- Stable CSM; Soft shadows
- Ambient occlusion
- Block particle system
- View frustum culling
- Debug system (profiling, debug drawing, record and playback feature)
- Simple preprocessor
- Game modes (title screen, world)

## Screenshots
![Screenshot](https://i.imgur.com/IqC7Lse.png)
![Screenshot](https://i.imgur.com/Mo1JhnH.png)
![Screenshot](https://i.imgur.com/LaslVgS.png)
