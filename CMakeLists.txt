cmake_minimum_required(VERSION 3.2)

project(Diffuse)

set (CMAKE_CXX_STANDARD 20)

set(SOURCES
    Application/Application.cpp
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)


# add_subdirectory(deps/glfw-3.3.6)

find_package(Vulkan REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/dependencies/include, ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

message(${PROJECT_SOURCE_DIR})