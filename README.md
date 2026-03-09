Homogeneous is currently a ray-casting based voxel renderer written by C++ with OpenGL.

# Install
Support Platform: Windows
- Clone this repo
- run `mkdir build && cd build`
- run `cmake .. && cmake --build .`
- run `cd Debug && Homogeneous.exe`

# Currently aiming to do

- [x] .VOX format supports.
- [x] Ray casting rendering for voxels.
- [x] Octree-based voxel structuralization & corresponding ray-casting shader.
- [ ] MagicaVoxel-like Greedy meshing for voxels, seperating ray casting for other volume rendering(maybe volume fogs, etc.) for better performance on opaque objects.
- [ ] Perlin-noise terrain generation.

# Demo
Flat light:

<img width="642" height="407.5" alt="96093f0d87afaf60a7fa5d95f0a06b16" src="https://github.com/user-attachments/assets/c5c09cbd-b883-4fdf-813a-ff77180721cf" />

Raycasting Shadow:

<img width="642" height="407.5" alt="dffb3640-2b67-4a6b-ba34-36bbf44767ed" src="https://github.com/user-attachments/assets/dbb18e47-5de1-46af-a1a5-7b934f2d7868" />

Raycasting AO:

<img width="642" height="407.5" alt="5c6bcf47437834677c041e1d410e595e" src="https://github.com/user-attachments/assets/9ccd5507-cbd9-4a40-a87a-b5704f80002d" />
