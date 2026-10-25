
# Rvela Engine

Rvela is a 3D game engine written in C++. This project is mainly designed for learning purposes.


![Logo](Resources/Editor/logo.png)

## Screenshots
![Screenshot1](Resources/Editor/screenshot1.png)  
![Screenshot2](Resources/Editor/screenshot2.png)  

## Features

- **Rendering**: OpenGL PBR pipeline, Cascaded Shadows , Screenspace effects (SSR, SSGI, SSAO, FXAA)
- **Materials & Shaders**: Custom shader support, ARM textures, Decals
- **ECS & Prefabs**: EnTT architecture, Prefab system
- **Physics**: Rigid bodies, character controller and colliders - Jolt Physics
- **Animation**: Skeletal mesh skinning, animation playback and libraries
- **Scripting**: Lua scripting with hot reload
- **Audio**: 3D spatial audio - miniaudio
- **UI System**: Canvas and interactive UI elements (Images, Text, Buttons, Sliders)
- **Asset Pipeline**: Texture compression - DirectXTex, mesh optimization - meshoptimizer

## Used Dependencies
- [ImGui](https://github.com/ocornut/imgui)  
- [Assimp](https://github.com/assimp/assimp)  
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)  
- [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)  
- [Entt](https://github.com/skypjack/entt)  
- [GLAD](https://github.com/Dav1dde/glad)  
- [GLFW](https://www.glfw.org/)  
- [GLM](https://github.com/g-truc/glm)  
- [stb_image](https://github.com/nothings/stb)  
- [JoltPhysics](https://github.com/jrouwe/JoltPhysics)  
- [Lua](https://github.com/lua/lua)  
- [Sol2](https://github.com/ThePhD/sol2)  
- [meshoptimizer](https://github.com/zeux/meshoptimizer)  
- [DirectXTex](https://github.com/microsoft/DirectXTex)  
- [miniaudio](https://github.com/mackron/miniaudio)  
- [nlohmann_json](https://github.com/nlohmann/json)  
- [tsl-robin-map](https://github.com/Tessil/robin-map)  
- [spdlog](https://github.com/gabime/spdlog)  
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)  
- [uuid_v4](https://github.com/crashoz/uuid_v4)   


## Build

To get the project up and running, you'll need to follow the setup instructions for your platform.

#### For Windows
1. Navigate to the `Scripts` folder in the project directory.
2. Run the `Setup-Windows.bat` batch file.

#### For Linux

1. Navigate to the `Scripts` folder in the project directory.

2. Make the `Setup-Linux.sh` script executable:
   
   ```bash
   chmod +x Setup-Linux.sh
3. Run the script to install necessary dependencies and set up the project:
   ```bash
   ./Setup-Linux.sh

