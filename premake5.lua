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
IncludeDirs["FastGLTF"] = "thirdparty/fastgltf"
IncludeDirs["simdJSON"] = "thirdparty/simdjson"
IncludeDirs["imGUI"] = "thirdparty/imgui"

LibraryDirs = {}
LibraryDirs["GLFW"] = "thirdparty/GLFW/lib-vc2022"
 
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
        "src/shaders/**"
    }

    vpaths
    {
        ["Headers"] = {"src/**.h", "!src/fastgltf/**", "!src/simdjson/**", "!src/imgui/**" },
        ["Sources"] = {"src/**.cpp", "src/**.c", "!src/fastgltf/**", "!src/simdjson/**", "!src/imgui/**"},
        ["Shaders"] = {"src/shaders/**"},
        ["FastGLTF"] = {"src/fastgltf/**"},
        ["simdJSON"] = {"src/simdjson/**"}, 
        ["imGUI"] = {"src/imgui/**"}
    }

    includedirs
    {
        "%{IncludeDirs.GLFW}",
        "%{IncludeDirs.GLAD}",
        "%{IncludeDirs.STB}",
        "%{IncludeDirs.GLM}",
        "%{IncludeDirs.FastGLTF}",
        "%{IncludeDirs.simdJSON}",
        "%{IncludeDirs.imGUI}"
    }

    libdirs
    {
        "%{LibraryDirs.GLFW}"
    }

    links
    {
        "glfw3.lib"
    }

    filter "system:windows"
        systemversion "latest"
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