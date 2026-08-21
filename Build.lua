-- premake5.lua
workspace "Rvela"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "RvelaEditor"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
   buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }
   defines { 'RVELA_ROOT_DIR="' .. _MAIN_SCRIPT_DIR:gsub('\\', '/') .. '"' }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

include "Editor/Build-Editor.lua"
include "Engine/Build-Engine.lua"