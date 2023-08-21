#include "Application.hpp"

#include <iostream>

namespace Diffuse {
    void Application::Init() {
        m_graphics = new Graphics();
        m_config.enable_validation_layers = false;
        if (m_graphics->Init(m_config)) {
            std::cout << "SUCCESS";
        }
        else {
            std::cout << "Failure setting up Vulkan";
        }
    }
    void Application::Update()
    {
        while (m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();
            m_graphics->Draw();
        }
    }
    void Application::Destroy()
    {
        m_graphics->CleanUp(m_config);
        delete m_graphics;
    }
}