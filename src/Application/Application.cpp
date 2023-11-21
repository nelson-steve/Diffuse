#include "Application.hpp"

#include "Model.hpp"
#include "Camera.hpp"
#include "Scene.hpp"
#include "Renderer.hpp"

#include <chrono>
#include <iostream>

namespace Diffuse {

    static std::shared_ptr<Renderer> g_renderer;
    static std::shared_ptr<Scene> g_scene;
    static std::shared_ptr<Camera> g_camera;

    void Application::Init() {
        m_graphics = new GraphicsDevice();
        g_scene = std::make_shared<Scene>();
        g_camera = std::make_shared<Camera>();
        //m_graphics->m_models.push_back(model);
        //m_graphics->m_models.push_back(skybox);

        // Creating a new scene
        std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>();
        
        // Creating a model that will be passed to scene as a scene object
        object->p_model.Load("../assets/damaged_helmet/DamagedHelmet.gltf", m_graphics);
        object->p_position = glm::vec3(0.0f);
        object->p_scale = glm::vec3(1.0f);
        object->p_rotation = glm::vec3(0.0f);
        g_scene->AddSceneObect(object);

        std::shared_ptr<Skybox> skybox = std::make_shared<Skybox>();
        skybox->p_model.Load("../assets/Box.gltf", m_graphics);

        // Creating a new scene
        std::shared_ptr<SceneCamera> scene_camera = std::make_shared<SceneCamera>();
        g_scene->AddSceneCamera(scene_camera);
        g_scene->AddSkybox(skybox);

        m_graphics->Setup(g_scene);
        g_renderer = std::make_shared<Renderer>(m_graphics);
    }
    void Application::Update()
    {
        auto current_time = std::chrono::high_resolution_clock::now();

        while (!m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();



            auto new_time = std::chrono::high_resolution_clock::now();
            float frame_time = std::chrono::duration<float, std::chrono::seconds::period>(new_time - current_time).count();
            current_time = new_time;

            g_renderer->RenderScene(g_scene, g_camera.get(), frame_time);
            g_camera->Update(frame_time, m_graphics->GetWindow()->window());


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