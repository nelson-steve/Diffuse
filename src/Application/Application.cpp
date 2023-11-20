#include "Application.hpp"

#include "Renderer.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include "Scene.hpp"

#include <chrono>
#include <iostream>

namespace Diffuse {

    static std::shared_ptr<Renderer> g_renderer;
    static std::shared_ptr<Scene> g_scene;

    void Application::Init() {
        m_graphics = new GraphicsDevice();
        g_scene = std::make_shared<Scene>();
        //m_graphics->m_models.push_back(model);
        //m_graphics->m_models.push_back(skybox);
        m_graphics->Setup();

        g_renderer = std::make_shared<Renderer>(m_graphics);

        // Creating a new scene
        std::shared_ptr<SceneObect> object = std::make_shared<SceneObect>();
        
        // Creating a model that will be passed to scene as a scene object
        object->p_model.Load("../assets/damaged_helmet/DamagedHelmet.gltf", m_graphics);
        object->p_position = glm::vec3(0.0f);
        object->p_scale = glm::vec3(1.0f);
        object->p_rotation = glm::vec3(0.0f);
        g_scene->AddSceneObect(object);

        // Creating a new scene
        std::shared_ptr<SceneCamera> scene_camera = std::make_shared<SceneCamera>();
        g_scene->AddSceneCamera(scene_camera);
    }
    void Application::Update()
    {
        auto current_time = std::chrono::high_resolution_clock::now();

        while (!m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();



            auto new_time = std::chrono::high_resolution_clock::now();
            float frame_time = std::chrono::duration<float, std::chrono::seconds::period>(new_time - current_time).count();
            current_time = new_time;

            g_renderer->RenderScene(g_scene);
            //camera->Update(frame_time, m_graphics->GetWindow()->window());


            //m_graphics->Draw(camera, frame_time);
            //std::cout << "frame time: " << 1000.0f / frame_time << std::endl;
        }
    }
    void Application::Destroy()
    {
        m_graphics->CleanUp();
        delete m_graphics;
    }
}