project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   pchheader "rvelapch.h"
    pchsource "Source/rvelapch.cpp"

   includedirs
   {
      "Source",
      "../Vendor/GLFW/include",
      "../Vendor/GLEW/include",
      "../Vendor/glm",
      "../Vendor/plog",
      "../Vendor/stb_image",
      "../Vendor/entt",
      "../Vendor/tiny_obj_loader",
      "../Vendor/Assimp/include"
      
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/GLEW/lib",
      "../Vendor/Assimp/lib"
   }

   links
   {
      "glfw3",
      "glew32s",
      "opengl32",
      "assimp-vc143-mt"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "GLFW_INCLUDE_NONE", "GLEW_STATIC" }

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