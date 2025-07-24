project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

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
      "../Vendor/uuid_v4"
      
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/Assimp/lib"
   }

   links
   {
      "glfw3",
      "opengl32",
      "assimp-vc143-mt"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "GLFW_INCLUDE_NONE" }
       buildoptions { "/utf-8" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
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