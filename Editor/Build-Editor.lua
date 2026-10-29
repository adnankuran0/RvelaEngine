project "RvelaEditor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"
   warnings "Off"

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
      "../Vendor/Jolt/include",
      "../Vendor/DirectXTex/include",
      "../Vendor/meshoptimizer/include",
      "../Vendor/recastnavigation/Recast/Include",
      "../Vendor/recastnavigation/Detour/Include",
      "../Vendor/recastnavigation/DetourCrowd/Include",
   }

   libdirs
   {
      "../Vendor/GLFW/lib",
      "../Vendor/lua/lib",
      "../Vendor/Jolt/lib",
      "../Vendor/Assimp/lib",
      "../Vendor/DirectXTex/lib",
      "../Vendor/meshoptimizer/lib",

   }

   links
   {
      "RvelaEngine",
        "glfw3",
        "lua54",
        "assimp-vc143-mt",
        "DirectXTex",
        "meshoptimizer",
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS","GLFW_INCLUDE_NONE" }
       buildoptions { "/utf-8","/MP"  }
       postbuildcommands {
          "{COPYFILE} ../Vendor/Assimp/lib/assimp-vc143-mt.dll %{cfg.targetdir}"
       }

   filter "configurations:Debug"
       defines { "DEBUG"}
       links { "JoltDebug" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE","NDEBUG"  }
       runtime "Release"
       links { "Jolt" }
       optimize "On"
       symbols "On"
       linkoptions { "/LTCG:incremental", "/NODEFAULTLIB:LIBCMT" }

   filter "configurations:Dist"
       defines { "DIST", "NDEBUG" }
       links { "Jolt" }
       runtime "Release"
       optimize "On"
       symbols "Off"

    filter "files:../Vendor/GLAD/src/gl.c"
        enablepch "Off"
