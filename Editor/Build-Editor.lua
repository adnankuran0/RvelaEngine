project "RvelaEditor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp","../Vendor/ImGui/*.cpp","../Vendor/ImGui/*.c", "../Vendor/GLAD/src/**.c", }

   includedirs
   {
      "Source",
      "../Vendor/GLFW/include",
      "../Vendor/GLAD/include",
	  "../Engine/Source",
      "../Vendor/entt",
      "../Vendor/spdlog/include",
      "../Vendor",
      "../Vendor/tiny_obj_loader",
      "../Vendor/glm",
      "../Vendor/Assimp/include",
      "../Vendor/ImGui",
      "../Vendor/nlohmann",
      "../Vendor/robin_map/include"
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
   }

   links
   {
      "RvelaEngine",
        "glfw3",
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS","GLFW_INCLUDE_NONE" }
       buildoptions { "/utf-8" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
       --postbuildcommands 
       --{ "{COPY} ../Vendor/Assimp/lib/assimp-vc143-mt.dll ../Binaries/" .. OutputDir .. "/Editor/" }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       postbuildcommands {
      "{COPY} ../Vendor/Assimp/lib/assimp-vc143-mt.dll ../Binaries/" .. OutputDir .. "/%{prj.name}/"
}

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"

    filter "files:../Vendor/GLAD/src/gl.c"
        flags { "NoPCH" }