#include "Application.hpp"

#include "Renderer.hpp"
#include "Model.hpp"

#include <iostream>

namespace Diffuse {

    static Renderer* renderer;
    static Model* model;

    void Application::Init() {
        m_graphics = new GraphicsDevice();
        //model = new Model("../assets/damaged_helmet/DamagedHelmet.gltf", m_graphics);
        //model = new Model("../assets/Avocado/Avocado.gltf", m_graphics);
        model = new Model("../assets/Sponza/Sponza/glTF/Sponza.gltf", m_graphics);
        m_graphics->CreateDescriptorSet(*model);
        renderer = new Renderer(m_graphics);
    }
    void Application::Update()
    {
        while (!m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();
            renderer->RenderModel(model);
        }
    }
    void Application::Destroy()
    {
        delete renderer;
        delete model;
        m_graphics->CleanUp();
        delete m_graphics;
    }
}