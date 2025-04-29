project "Editor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp","Vendor/ImGui/*.cpp","Vendor/ImGui/*.c" }

   includedirs
   {
      "Source",
      "Vendor",
      "../Vendor/GLFW/include",
      "../Vendor/GLEW/include",
	  -- Include Core
	  "../Engine/Source",
      "../Vendor/entt",
      "../Vendor/tiny_obj_loader",
      "../Vendor/glm"
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/GLEW/lib"
   }

   links
   {
      "Engine",
        "glfw3",
      "glew32s"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS","GLFW_INCLUDE_NONE" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
       postbuildcommands {
        "{COPY} ../Vendor/Assimp/lib/assimp-vc143-mt.dll ../Binaries/" .. OutputDir .. "/Editor/" }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       postbuildcommands {
        "{COPY} ../Vendor/Assimp/lib/assimp-vc143-mt.dll ../Binaries/" .. OutputDir .. "/Editor/"}

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"

   