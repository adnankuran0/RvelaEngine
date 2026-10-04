-- premake5.lua
workspace "Rvela"
    architecture "x64"
    configurations { "Debug", "Release", "Dist" }
    startproject "RvelaEditor"

    filter "system:windows"
        buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus", "/MP", "/bigobj", "/Zc:inline" }
        linkoptions { "/CGTHREADS:8", "/ignore:4099", "/ignore:4098" }
        defines { 'RVELA_ROOT_DIR="' .. _MAIN_SCRIPT_DIR:gsub('\\', '/') .. '"' }

    filter "configurations:Dist"
        linktimeoptimization "On"

    filter "configurations:Debug"
        linkoptions { "/DEBUG:FASTLINK", "/NODEFAULTLIB:LIBCMT", "/NODEFAULTLIB:LIBCMTD" }

    filter "configurations:Release"
        linkoptions { "/LTCG:incremental", "/NODEFAULTLIB:LIBCMT" }

    filter {}

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

include "Editor/Build-Editor.lua"
include "Engine/Build-Engine.lua"
include "Runtime/Build-Runtime.lua"