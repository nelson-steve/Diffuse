#include "Application.h"

#include <iostream>
#include <vulkan/vulkan.h>

int main(int argc, char** argv[]) {
    std::cout<<"Hello world";
    
    VkInstance instance;
    vkCreateInstance(nullptr, nullptr, nullptr);

    Application* app = new Application();
    app->Init();
    delete app;

    return 0;
}