cmake_minimum_required(VERSION 3.0)
project(CGRA350 LANGUAGES CXX CUDA)

enable_language(CUDA)

if (WIN32)
	MESSAGE(STATUS "Is windows")
	SET(CMAKE_CXX_FLAGS "/openmp /Ot")
else ()
	MESSAGE(STATUS "Is linux")
	add_definitions(-DLINUX)
endif ()

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(libs/vendor/glfw)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2") #/W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2") # -Wall -Wextra -Wpedantic -std=c++11")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

Option(GUI OFF)
Option(RTX30XX OFF)
Option(CompileRPNN OFF)

if (GUI)
	add_definitions(-DGUI)
    #set(GLEW_VERBOSE ON)
    #set(glfw3_DIR ${PROJECT_SOURCE_DIR}/ext/glfw-3.4/build/lib/cmake/glfw3)
    #find_package(GLEW REQUIRED HINTS ${PROJECT_SOURCE_DIR}/libs/vendor/glew-2.1.0/build/lib/cmake/glew)
    #find_package(glfw3)
    #add_subdirectory(Imgui)
	#INCLUDE_DIRECTORIES(Imgui_INCLUDE_DIR)
endif()

if (CompileRPNN)
    add_definitions(-DCRPNN)
endif()

include_directories(libs/vendor/glad/include/
        libs/vendor/glfw/include/
        libs/vendor/glm/
        libs/vendor/stb/
        libs/vendor/imgui/  # bc imgui backend files must refer to "imgui.h"
        libs/vendor/)       # for imgui files being imgui/... rather than just the header file

set(VENDORS_SOURCES libs/vendor/glad/src/glad.c
                    libs/vendor/stb/stb_image.h
                    libs/vendor/stb/stb_image_write.h
                    libs/vendor/imgui/imgui.cpp
                    libs/vendor/imgui/imgui.h
                    libs/vendor/imgui/imgui_internal.h
                    libs/vendor/imgui/imgui_draw.cpp
                    libs/vendor/imgui/imgui_widgets.cpp
                    libs/vendor/imgui/imgui_tables.cpp
					libs/vendor/imgui/backends/imgui_impl_dx9.h
                    libs/vendor/imgui/backends/imgui_impl_dx9.cpp
					libs/vendor/imgui/backends/imgui_impl_win32.h
                    libs/vendor/imgui/backends/imgui_impl_win32.cpp
                    libs/vendor/imgui/backends/imgui_impl_glfw.h
                    libs/vendor/imgui/backends/imgui_impl_glfw.cpp
                    libs/vendor/imgui/backends/imgui_impl_opengl3.h
                    libs/vendor/imgui/backends/imgui_impl_opengl3.cpp)

set(PROJECT_HEADERS src/main/cgra350final.h
                    src/main/constants.h
                    src/main/app_context.h
                    src/utils/image_io.h
                    src/ui/ui.h
                    src/graphics/buffers.h
                    src/graphics/shaders.h
                    src/graphics/camera.h
                    src/graphics/textures.h
                    src/graphics/window.h
                    src/graphics/meshes.h
                    src/graphics/renderers.h)

set(PROJECT_SOURCES src/main/cgra350final.cpp
                    src/main/app_context.cpp
                    src/utils/image_io.cpp
                    src/ui/ui.cpp
                    src/graphics/buffers.cpp
                    src/graphics/shaders.cpp
                    src/graphics/camera.cpp
                    src/graphics/textures.cpp
                    src/graphics/window.cpp
                    src/graphics/meshes.cpp
                    src/graphics/renderers.cpp)

file(GLOB PROJECT_SHADERS ${PROJECT_SOURCE_DIR}/resources/shaders/*.frag
                          ${PROJECT_SOURCE_DIR}/resources/shaders/*.vert
                          ${PROJECT_SOURCE_DIR}/resources/shaders/*.geom)

set(PROJECT_CONFIGS CMakeLists.txt
                    .gitattributes
                    .gitignore)
                    # .gitmodules)

source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Vendors" FILES ${VENDORS_SOURCES})
source_group("Shaders" FILES ${PROJECT_SHADERS})

message("PROJECT_SOURCES = ${PROJECT_SOURCES}")
message("VENDORS_SOURCES = ${VENDORS_SOURCES}")

file(GLOB VR_SOURCES src/volumerendering/*)

source_group("VolumeRendering" FILES ${VR_SOURCES}) 

add_definitions(-DGLFW_INCLUDE_NONE
                -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES}  # ${PROJECT_HEADERS}
                               ${PROJECT_SHADERS} ${PROJECT_CONFIGS}
                               ${VENDORS_SOURCES} ${VR_SOURCES} src/main/cgra350final.cpp)
target_link_libraries(${PROJECT_NAME} glfw
                      ${GLFW_LIBRARIES} ${GLAD_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
    CUDA_SEPARABLE_COMPILATION ON
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD 17)
set_source_files_properties(src/main/cgra350final.cpp PROPERTIES LANGUAGES CUDA)
set_source_files_properties(${VR_SOURCES} PROPERTIES LANGUAGE CUDA)
target_compile_definitions(${PROJECT_NAME} PRIVATE _HAS_STD_BYTE=0)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/resources/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        DEPENDS ${PROJECT_SHADERS})

if (RTX30XX)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-use_fast_math -arch=sm_86 -maxrregcount=168>)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-use_fast_math -arch=sm_75 -maxrregcount=168>)
endif()

#if (GUI)
#    target_link_libraries(${PROJECT_NAME} GLEW::GLEW glfw Imgui) 
#    add_dependencies(${PROJECT_NAME} Imgui)
#endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})