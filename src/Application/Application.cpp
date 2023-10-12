#include "Application.hpp"

#include "Renderer.hpp"
#include "Model.hpp"
#include "Camera.hpp"

#include <chrono>
#include <iostream>

namespace Diffuse {

    static Renderer* renderer;
    static Camera* camera;
    static Model* model;

    void Application::Init() {

        camera = new Camera();
        m_graphics = new GraphicsDevice();
        model = new Model("../assets/damaged_helmet/DamagedHelmet.gltf", m_graphics);
        //model = new Model("../assets/Avocado/Avocado.gltf", m_graphics);
        //model = new Model("../assets/Sponza/Sponza/glTF/Sponza.gltf", m_graphics);
        m_graphics->Setup(*model);
        renderer = new Renderer(m_graphics);
    }
    void Application::Update()
    {
        auto current_time = std::chrono::high_resolution_clock::now();

        while (!m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();

            auto new_time = std::chrono::high_resolution_clock::now();
            float frame_time = std::chrono::duration<float, std::chrono::seconds::period>(new_time - current_time).count();
            current_time = new_time;
            camera->Update(frame_time, m_graphics->GetWindow()->window());
            renderer->RenderModel(camera, frame_time, model);
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