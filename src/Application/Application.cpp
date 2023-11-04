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
    static Model* skybox;

    void Application::Init() {

        camera = new Camera();
        m_graphics = new GraphicsDevice();
        model = new Model();
        skybox = new Model();
        //model->Load("../assets/FlightHelmet/glTF/FlightHelmet.gltf", m_graphics);
        //model->Load("../assets/damaged_helmet/DamagedHelmet.gltf", m_graphics);
        model->Load("../assets/revolver/revolver.gltf", m_graphics);
        //model->Load("../assets/ToyCar/glTF/ToyCar.gltf", m_graphics);
        skybox->Load("../assets/Box.gltf", m_graphics);
        m_graphics->m_models.push_back(model);
        m_graphics->m_models.push_back(skybox);
        m_graphics->Setup();
        //model = new Model("../assets/Avocado/Avocado.gltf", m_graphics);
        //model = new Model("../assets/Sponza/Sponza/glTF/Sponza.gltf", m_graphics);
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
            //renderer->RenderModel(camera, frame_time, model);

            m_graphics->Draw(camera);
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