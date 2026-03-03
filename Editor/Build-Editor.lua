project "RvelaEditor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp","../Vendor/ImGui/*.cpp","../Vendor/ImGui/*.c", "../Vendor/GLAD/src/**.c", }

   defines
   {
        "_CONSOLE",
       "JPH_DEBUG_RENDERER",
       "JPH_OBJECT_STREAM",
       "JPH_USE_SSE4_1",
       "JPH_USE_SSE4_2"
   }

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
      "../Vendor/robin_map/include",
      "../Vendor/lua/include",
      "../Vendor/Jolt/include"
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/lua/lib",
      "../Vendor/Jolt/lib"
   }

   links
   {
      "RvelaEngine",
        "glfw3",
        "lua54",
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS","GLFW_INCLUDE_NONE" }
       buildoptions { "/utf-8","/MP"  }

   filter "configurations:Debug"
       defines { "DEBUG"}
       links { "JoltDebug" }
       runtime "Debug"
       symbols "On"
       --postbuildcommands 
       --{ "{COPY} ../Vendor/Assimp/lib/assimp-vc143-mt.dll ../Binaries/" .. OutputDir .. "/Editor/" }

   filter "configurations:Release"
       defines { "RELEASE","NDEBUG"  }
       runtime "Release"
       links { "Jolt" }
       optimize "On"
       symbols "On"
       --postbuildcommands {
      --"{COPY} ../Vendor/Assimp/lib/assimp-vc143-mt.dll ../Binaries/" .. OutputDir .. "/%{prj.name}/"}

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"

    filter "files:../Vendor/GLAD/src/gl.c"
        flags { "NoPCH" }