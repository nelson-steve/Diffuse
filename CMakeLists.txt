cmake_minimum_required(VERSION 3.2)

project(Diffuse)

set (CMAKE_CXX_STANDARD 20)

set(SOURCES
    src/Application/Application.cpp
    src/Graphics/tiny_gltf.cpp
    src/Graphics/GraphicsDevice.cpp
    src/Renderer/Model.cpp
    src/Renderer/Texture2D.cpp
    src/Renderer/Renderer.cpp
    src/Renderer/Camera.cpp
    src/Graphics/Window.cpp
    src/Graphics/Swapchain.cpp
    src/Graphics/VulkanUtilities.cpp
    src/Graphics/Buffer.cpp
    src/Utils/ReadFile.cpp
    src/main.cpp
)

set(HEADERS
    include/Application.hpp
    include/Model.hpp
    include/Texture2D.hpp
    include/GraphicsDevice.hpp
    include/Swapchain.hpp
    include/Renderer.hpp
    include/Camera.hpp
    include/Window.hpp
    include/VulkanUtilities.hpp
    include/Buffer.hpp
    include/ReadFile.hpp
    dependencies/tiny_gltf/json.hpp
    dependencies/tiny_gltf/tiny_gltf.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/dependencies/tiny_gltf
        ${PROJECT_SOURCE_DIR}/dependencies/ktx/include
)


set(glm_DIR ${PROJECT_SOURCE_DIR}/dependencies/glm/cmake/glm)
set(KTX_DIR ${PROJECT_SOURCE_DIR}/dependencies/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c)

add_library(base STATIC ${BASE_SRC} ${KTX_SOURCES})

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${Vulkan_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${PROJECT_SOURCE_DIR}/dependencies/include)
include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${PROJECT_SOURCE_DIR}/dependencies/ktx/include)
include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${PROJECT_SOURCE_DIR}/dependencies/ktx/other_include)

message(${PROJECT_SOURCE_DIR})
message(${Vulkan_LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/dependencies/lib/glfw3.lib)
target_link_libraries(${PROJECT_NAME} glm::glm)
target_link_libraries(${PROJECT_NAME} base)
