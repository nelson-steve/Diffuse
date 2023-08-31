cmake_minimum_required(VERSION 3.2)

project(Diffuse)

set (CMAKE_CXX_STANDARD 20)

set(SOURCES
    src/Application/Application.cpp
    src/Graphics/tiny_obj_loader.cpp
    src/Graphics/GraphicsDevice.cpp
    src/Renderer/Renderer.cpp
    src/Graphics/Window.cpp
    src/Graphics/VulkanUtilities.cpp
    src/Utils/ReadFile.cpp
    src/main.cpp
)

set(HEADERS
    include/Application.hpp
    include/GraphicsDevice.hpp
    include/Renderer.hpp
    include/Window.hpp
    include/VulkanUtilities.hpp
    include/ReadFile.hpp
    dependencies/tiny_obj_loader/tiny_obj_loader.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/dependencies/tiny_obj_loader
)


set(glm_DIR ${PROJECT_SOURCE_DIR}/dependencies/glm/cmake/glm)

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${Vulkan_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${PROJECT_SOURCE_DIR}/dependencies/include)

message(${PROJECT_SOURCE_DIR})
message(${Vulkan_LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/dependencies/lib/glfw3.lib)
target_link_libraries(${PROJECT_NAME} glm::glm)
