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
IncludeDirs["OpenXR"] = "thirdparty/OpenXR/include"
IncludeDirs["OpenXRCommon"] = "thirdparty/OpenXR/include/Common"
IncludeDirs["OpenXRLoader"] = "thirdparty/OpenXR/include/loader"


LibraryDirs = {}
LibraryDirs["GLFW"] = "thirdparty/GLFW/lib-vc2022"
LibraryDirs["OpenXR"] = "thirdparty/OpenXR/include/lib"
LibraryDirs["OpenXRLoader"] = "thirdparty/OpenXR/include/loader"
 
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
        "thirdparty/glad.c",
        "thirdparty/OpenXR/include/Common/**"
    }

    vpaths
    {
        ["Headers/Application"] = {"src/Application/**.h"},
        ["Headers/Model"] = {"src/Model/**.h"},
        ["Sources/Application"] = {"src/Application/**.cpp"},
        ["Sources/Model"] = {"src/Model/**.cpp"},
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
        "%{IncludeDirs.OpenXR}",
        "%{IncludeDirs.OpenXRCommon}"
    }

    libdirs
    {
        "%{LibraryDirs.GLFW}",
        "%{LibraryDirs.OpenXR}",
        "%{LibraryDirs.OpenXRLoader}"
    }

    links
    {
        "glfw3.lib",
        "openxr-gfxwrapper.lib",
        "openxr_loaderd.lib"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions {"/MP"}
        defines
        {
            "GLIMMER_PLATFORM_WINDOWS"
        }
    
    filter "configurations:Debug"
        defines "GLIMMER_DEBUG"
        runtime "Debug"
        symbols "on"

        postbuildcommands
        {

        }

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "on"
        buildoptions {"/Ob0"}

        postbuildcommands
        {
            
        }