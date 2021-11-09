cmake_minimum_required(VERSION 3.10)

project(vkplayground)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(Freetype REQUIRED)

file(GLOB_RECURSE sources src/*.cpp src/*.h)
file(GLOB_RECURSE opt_sources opt/*.cpp opt/*.h)
file(GLOB_RECURSE shaders shaders/*.vert shaders/*.frag)

add_subdirectory(external/VulkanMemoryAllocator)
add_subdirectory(external/libspng)
target_compile_options(spng PRIVATE -O3)

add_library(optimized_components STATIC ${opt_sources})
target_include_directories(optimized_components PRIVATE include/)
target_include_directories(optimized_components PRIVATE external/VulkanMemoryAllocator-Hpp/)
target_compile_options(optimized_components PRIVATE -O3)

add_executable(vkplayground ${sources})

target_include_directories(vkplayground PRIVATE include/)

target_link_libraries(vkplayground PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(vkplayground PRIVATE VulkanMemoryAllocator)
target_link_libraries(vkplayground PRIVATE glfw)
target_link_libraries(vkplayground PRIVATE spdlog::spdlog)
target_link_libraries(vkplayground PRIVATE dl)
target_link_libraries(vkplayground PRIVATE spng)
target_link_libraries(vkplayground PRIVATE Freetype::Freetype)
target_link_libraries(vkplayground PRIVATE optimized_components)

target_include_directories(vkplayground PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(vkplayground PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(vkplayground PRIVATE external/VulkanMemoryAllocator-Hpp/)
target_include_directories(vkplayground PRIVATE external/libspng/spng)

find_program(GLSLC glslc)
find_program(GLSLang_Validator glslangValidator)
if(NOT GLSLC AND NOT GLSLang_Validator)
	message(FATAL_ERROR "No GLSL compiler found!")
endif()

if(GLSLang_Validator)
	set(GLSL_COMPILER ${GLSLang_Validator})
	set(GLSL_COMPILER_FLAGS "-V")
else()
	set(GLSL_COMPILER ${GLSLC})
	set(GLSL_COMPILER_FLAGS "")
endif()
message(STATUS "Found GLSL compiler: ${GLSL_COMPILER}")

add_subdirectory(shaders/)

add_dependencies(vkplayground shaders)

add_library(imgui STATIC)
target_include_directories(imgui PUBLIC external/imgui)
target_sources(imgui PRIVATE 
  external/imgui/imgui.h
  external/imgui/imgui.cpp

  external/imgui/imgui_demo.cpp
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/imgui_tables.cpp

  external/imgui/backends/imgui_impl_vulkan.cpp
  external/imgui/backends/imgui_impl_glfw.cpp
  )
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)
target_link_libraries(vkplayground PUBLIC imgui)
