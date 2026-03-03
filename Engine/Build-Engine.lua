project "RvelaEngine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   defines
   {
        "_CONSOLE",
       "JPH_DEBUG_RENDERER",
       "JPH_OBJECT_STREAM",
       "JPH_USE_SSE4_1",
       "JPH_USE_SSE4_2"
   }

   files { "Source/**.h", "Source/**.cpp", "../Vendor/GLAD/src/**.c" }

   pchheader "rvelapch.h"
    pchsource "Source/rvelapch.cpp"

   includedirs
   {
      "Source",
      "../Vendor",
      "../Vendor/GLFW/include",
      "../Vendor/GLAD/include",
      "../Vendor/glm",
      "../Vendor/spdlog/include",
      "../Vendor/stb_image",
      "../Vendor/entt",
      "../Vendor/tiny_obj_loader",
      "../Vendor/Assimp/include",
      "../Vendor/nlohmann",
      "../Vendor/uuid_v4",
      "../Vendor/robin_map/include",
      "../Vendor/sol",
      "../Vendor/lua/include",
      "../Vendor/Jolt/include"
      
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/Assimp/lib",
      "../Vendor/lua/lib",
      "../Vendor/Jolt/lib"

   }

   links
   {
      "glfw3",
      "opengl32",
      "assimp-vc143-mt",
      "lua54",
      
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "GLFW_INCLUDE_NONE" }
       buildoptions { "/utf-8", "/MP" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       links { "JoltDebug" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE", "NDEBUG"}
       links { "Jolt" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"

    filter "files:../Vendor/GLAD/src/gl.c"
        flags { "NoPCH" }