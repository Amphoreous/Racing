newoption
{
    trigger = "graphics",
    value = "OPENGL_VERSION",
    description = "version of OpenGL to build raylib against",
    allowed = {
        { "opengl11", "OpenGL 1.1"},
        { "opengl21", "OpenGL 2.1"},
        { "opengl33", "OpenGL 3.3"},
        { "opengl43", "OpenGL 4.3"},
        { "openges2", "OpenGL ES2"},
        { "openges3", "OpenGL ES3"}
    },
    default = "opengl33"
}

function download_progress(total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Download progress (" .. percent .. "%/100%)")
end

function check_raylib()
    os.chdir("external")
    if(os.isdir("raylib-master") == false) then
        if(not os.isfile("raylib-master.zip")) then
            print("Raylib not found, downloading from github")
            local result_str, response_code = http.download("https://github.com/raysan5/raylib/archive/refs/heads/master.zip", "raylib-master.zip", {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract("raylib-master.zip", os.getcwd())
        os.remove("raylib-master.zip")
    end
    os.chdir("../")
end

function get_latest_box2d_version()
    -- Attempt to get the latest version from GitHub API
    -- If this fails due to network restrictions, fallback to known latest version
    local latest_version = "3.1.1" -- Known latest version as fallback
    
    print("Attempting to fetch latest Box2D version...")
    local result_str, response_code = http.get("https://api.github.com/repos/erincatto/box2d/releases/latest")
    if response_code == 200 and result_str then
        -- Parse JSON to extract tag_name
        local tag_match = string.match(result_str, '"tag_name"%s*:%s*"v([^"]+)"')
        if tag_match then
            latest_version = tag_match
            print("Latest Box2D version found: " .. latest_version)
        else
            print("Could not parse version from API response, using fallback: " .. latest_version)
        end
    else
        print("Could not fetch latest version from GitHub API (response code: " .. tostring(response_code) .. "), using fallback: " .. latest_version)
    end
    
    return latest_version
end

function check_box2d()
    os.chdir("external")
    
    -- Get the latest version dynamically
    local box2d_version = get_latest_box2d_version()
    local box2d_folder = "box2d-" .. box2d_version
    local box2d_zip = box2d_folder .. ".zip"
    
    if(os.isdir(box2d_folder) == false) then
        if(not os.isfile(box2d_zip)) then
            print("Box2D v" .. box2d_version .. " not found, downloading from github")
            local download_url = "https://github.com/erincatto/box2d/archive/refs/tags/v" .. box2d_version .. ".zip"
            local result_str, response_code = http.download(download_url, box2d_zip, {
                progress = download_progress,
                headers = { "From: Premake", "Referer: Premake" }
            })
        end
        print("Unzipping to " ..  os.getcwd())
        zip.extract(box2d_zip, os.getcwd())
        os.remove(box2d_zip)
    else
        print("Box2D v" .. box2d_version .. " already exists")
    end
    
    -- Update the global box2d_dir variable to point to the correct version
    box2d_dir = "external/" .. box2d_folder
    
    os.chdir("../")
end

function build_externals()
     print("calling externals")
     check_raylib()
     if (downloadBox2D) then
         check_box2d()
     end
end

function platform_defines()
    filter {"configurations:Debug or Release"}
        defines{"PLATFORM_DESKTOP"}

    filter {"configurations:Debug_RGFW or Release_RGFW"}
        defines{"PLATFORM_DESKTOP_RGFW"}

    filter {"options:graphics=opengl43"}
        defines{"GRAPHICS_API_OPENGL_43"}

    filter {"options:graphics=opengl33"}
        defines{"GRAPHICS_API_OPENGL_33"}

    filter {"options:graphics=opengl21"}
        defines{"GRAPHICS_API_OPENGL_21"}

    filter {"options:graphics=opengl11"}
        defines{"GRAPHICS_API_OPENGL_11"}

    filter {"options:graphics=openges3"}
        defines{"GRAPHICS_API_OPENGL_ES3"}

    filter {"options:graphics=openges2"}
        defines{"GRAPHICS_API_OPENGL_ES2"}

    filter {"system:macosx"}
        disablewarnings {"deprecated-declarations"}

    filter {"system:linux"}
        defines {"_GLFW_X11"}
        defines {"_GNU_SOURCE"}
-- This is necessary, otherwise compilation will fail since
-- there is no CLOCK_MONOTOMIC. raylib claims to have a workaround
-- to compile under c99 without -D_GNU_SOURCE, but it didn't seem
-- to work. raylib's Makefile also adds this flag, probably why it went
-- unnoticed for so long.
-- It compiles under c11 without -D_GNU_SOURCE, because c11 requires
-- to have CLOCK_MONOTOMIC
-- See: https://github.com/raysan5/raylib/issues/2729

    filter{}
end

-- if you don't want to download raylib, then set this to false, and set the raylib dir to where you want raylib to be pulled from, must be full sources.
downloadRaylib = true
raylib_dir = "external/raylib-master"

-- if you don't want to download box2d, then set this to false, and set the box2d dir to where you want box2d to be pulled from, must be full sources.
downloadBox2D = true
box2d_dir = "external/box2d-3.1.1"  -- This will be updated dynamically by check_box2d()

workspaceName = 'MyGame'
baseName = path.getbasename(path.getdirectory(os.getcwd()));

--if (baseName ~= 'raylib-quickstart') then
    workspaceName = baseName
--end

if (os.isdir('build_files') == false) then
    os.mkdir('build_files')
end

if (os.isdir('external') == false) then
    os.mkdir('external')
end


workspace (workspaceName)
    location "../"
    configurations { "Debug", "Release", "Debug_RGFW", "Release_RGFW"}
    platforms { "x64", "x86", "ARM64"}

    defaultplatform ("x64")

    filter "configurations:Debug or Debug_RGFW"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release or Release_RGFW"
        defines { "NDEBUG" }
        optimize "On"

    filter { "platforms:x64" }
        architecture "x86_64"

    filter { "platforms:Arm64" }
        architecture "ARM64"

    filter {}

    targetdir "bin/%{cfg.buildcfg}/"

if (downloadRaylib or downloadBox2D) then
    build_externals()
end

    startproject(workspaceName)

    project (workspaceName)
        kind "ConsoleApp"
        location "build_files/"
        targetdir "../bin/%{cfg.buildcfg}"

        filter {"system:windows", "configurations:Release or Release_RGFW", "action:gmake*"}
            kind "WindowedApp"
            buildoptions { "-Wl,--subsystem,windows" }

        filter {"system:windows", "configurations:Release or Release_RGFW", "action:vs*"}
            kind "WindowedApp"
            entrypoint "mainCRTStartup"

        filter "action:vs*"
            debugdir "$(SolutionDir)"

        filter{}

        vpaths 
        {
            ["Header Files/*"] = { "../include/**.h",  "../include/**.hpp", "../src/**.h", "../src/**.hpp"},
            ["Source Files/*"] = {"../src/**.c", "src/**.cpp"},
            ["Windows Resource Files/*"] = {"../src/**.rc", "src/**.ico"},
        }
        
        files {"../src/**.c", "../src/**.cpp", "../src/**.h", "../src/**.hpp", "../include/**.h", "../include/**.hpp"}
        
        filter {"system:windows", "action:vs*"}
            files {"../src/*.rc", "../src/*.ico"}

        filter{}
        
        includedirs { "../src" }
        includedirs { "../include" }

        links {"raylib"}
        
        if (downloadBox2D) then
            links {"box2d"}
        end

        cdialect "C17"
        cppdialect "C++17"

        includedirs {raylib_dir .. "/src" }
        includedirs {raylib_dir .."/src/external" }
        includedirs { raylib_dir .."/src/external/glfw/include" }
        
        if (downloadBox2D) then
            includedirs {box2d_dir .. "/include" }
        end
        flags { "ShadowedVariables"}
        platform_defines()

        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            dependson {"raylib"}
            links {"raylib.lib"}
            if (downloadBox2D) then
                dependson {"box2d"}
                links {"box2d.lib"}
            end
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }

        filter "system:windows"
            defines{"_WIN32"}
            links {"winmm", "gdi32", "opengl32"}
            libdirs {"../bin/%{cfg.buildcfg}"}

        filter "system:linux"
            links {"pthread", "m", "dl", "rt", "X11"}

        filter "system:macosx"
            links {"OpenGL.framework", "Cocoa.framework", "IOKit.framework", "CoreFoundation.framework", "CoreAudio.framework", "CoreVideo.framework", "AudioToolbox.framework"}

        filter{}
        

    project "raylib"
        kind "StaticLib"
    
        platform_defines()

        location "build_files/"

        language "C"
        targetdir "../bin/%{cfg.buildcfg}"

        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
            buildoptions { "/Zc:__cplusplus" }
        filter{}

        includedirs {raylib_dir .. "/src", raylib_dir .. "/src/external/glfw/include" }
        vpaths
        {
            ["Header Files"] = { raylib_dir .. "/src/**.h"},
            ["Source Files/*"] = { raylib_dir .. "/src/**.c"},
        }
        files {raylib_dir .. "/src/*.h", raylib_dir .. "/src/*.c"}

        removefiles {raylib_dir .. "/src/rcore_*.c"}

        filter { "system:macosx", "files:" .. raylib_dir .. "/src/rglfw.c" }
            compileas "Objective-C"

        filter{}

if (downloadBox2D) then
    project "box2d"
        kind "StaticLib"
        
        location "build_files/"
        
        language "C"
        targetdir "../bin/%{cfg.buildcfg}"
        
        -- Use C11 standard for static_assert support
        cdialect "C11"
        
        filter "action:vs*"
            defines{"_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS"}
            characterset ("Unicode")
            -- MSVC compatibility: Force C compilation
            buildoptions { "/TC" }
            -- For MSVC, define _Static_assert as a no-op since MSVC doesn't fully support C11
            defines { "_Static_assert(x,y)=" }
        
        filter "system:not windows"
            -- For non-Windows systems, ensure C11 support and define required macros
            buildoptions { "-std=c11" }
            defines { "_GNU_SOURCE", "_POSIX_C_SOURCE=200809L" }
        
        filter{}
        
        includedirs {box2d_dir, box2d_dir .. "/include", box2d_dir .. "/src", box2d_dir .. "/extern/simde" }
        vpaths
        {
            ["Header Files"] = { box2d_dir .. "/include/**.h", box2d_dir .. "/src/**.h"},
            ["Source Files/*"] = { box2d_dir .. "/src/**.c"},
        }
        files {box2d_dir .. "/include/**.h", box2d_dir .. "/src/**.c", box2d_dir .. "/src/**.h"}
        
        filter{}
end
