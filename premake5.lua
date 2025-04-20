workspace "Glimmer"
    architecture "x64"
    configurations {"Debug", "Release"}
    startproject "Glimmer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDirs = {}
IncludeDirs["GLFW"] = "thirdparty/GLFW/include"
IncludeDirs["GLAD"] = "thirdparty/GLAD/include"
IncludeDirs["STB"] = "thirdparty/stb"
IncludeDirs["GLM"] = "thirdparty/glm"
IncludeDirs["GL"] = "thirdparty/GL"
IncludeDirs["FastGLTF"] = "thirdparty/fastgltf"
IncludeDirs["simdJSON"] = "thirdparty/simdjson"
IncludeDirs["imGUI"] = "thirdparty/imgui"
IncludeDirs["MeshOpt"] = "thirdparty/meshoptimizer"


LibraryDirs = {}
LibraryDirs["GLFW"] = "thirdparty/GLFW/lib-vc2022"
LibraryDirs["MeshOpt"] = "thirdparty/meshoptimizer"
 
project "Glimmer"
    location "src"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.cpp",
        "src/**.c",
        "src/**.hpp",
        "src/shaders/**",
        "thirdparty/imgui/**.cpp",
        "thirdparty/imgui/**.c",
        "thirdparty/fastgltf/**.cpp",
        "thirdparty/fastgltf/**.c",
        "thirdparty/simdjson/**.cpp",
        "thirdparty/simdjson/**.c",
        "thirdparty/stb.cpp",
        "thirdparty/glad.c"
    }

    vpaths
    {
        ["Shaders"] = {"src/shaders/**"},
        ["FastGLTF"] = {"src/fastgltf/**"},
        ["simdJSON"] = {"src/simdjson/**"}, 
        ["ThirdParty/ImGUI"] = {"thirdparty/imgui/**"},
        ["ThirdParty/FastGLTF"] = {"thirdparty/fastgltf/**"},
        ["ThirdParty/SIMDJSON"] = {"thirdparty/simdjson/**"},
        ["ThirdParty"] = {"thirdparty/stb.cpp", "thirdparty/glad.c"}
    }

    includedirs
    {
        "src",
        "%{IncludeDirs.GLFW}",
        "%{IncludeDirs.GLAD}",
        "%{IncludeDirs.STB}",
        "%{IncludeDirs.GLM}",
        "%{IncludeDirs.GL}",
        "%{IncludeDirs.FastGLTF}",
        "%{IncludeDirs.simdJSON}",
        "%{IncludeDirs.imGUI}",
        "%{IncludeDirs.MeshOpt}"
    }

    libdirs
    {
        "%{LibraryDirs.GLFW}",
        "%{LibraryDirs.MeshOpt}",
    }
    
    links
    {
        "glfw3.lib",
        "opengl32.lib",
        "meshoptimizer.lib"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions {"/MP"}
        defines
        {
            "GLIMMER_PLATFORM_WINDOWS",
            "XR_USE_GRAPHICS_API_OPENGL"
        }
    
    filter "configurations:Debug"
        defines "GLIMMER_DEBUG"
        runtime "Debug"
        symbols "on"

        postbuildcommands
        {
        }

    filter "configurations:Release"
        defines { "NDEBUG", "_ITERATOR_DEBUG_LEVEL=0" }
        runtime "Release"
        optimize "on"
        buildoptions {"/Ob0"}

        postbuildcommands
        {
        }