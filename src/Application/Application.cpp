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
    //static std::shared_ptr<Camera> g_camera;
    //static std::shared_ptr<SceneCamera> g_scene_camera;
    static std::shared_ptr<EditorCamera> g_editor_camera;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;
    void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        g_editor_camera->SetMouseOffset(xoffset, yoffset);
        g_editor_camera->MouseMoved();
    }

    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        g_editor_camera->OnMouseScroll(yoffset);
    }

    void Application::Init() {
        m_graphics = new GraphicsDevice();
        {
            // Creating scene
            g_scene = std::make_shared<Scene>();

            // creating scene camera
            //g_scene_camera = std::make_shared<SceneCamera>();
            //g_scene_camera->p_camera = std::make_shared<Camera>();

            g_editor_camera = std::make_shared<EditorCamera>(60.0f, 1920.0f / 1080.0f, 0.01f, 10000.0f, m_graphics->GetWindow()->window());

            // Createing scene object
            std::shared_ptr<SceneObject> object1 = std::make_shared<SceneObject>();
            object1->p_model.Load("../assets/damaged_helmet/DamagedHelmet.gltf", m_graphics);

            std::shared_ptr<SceneObject> object2 = std::make_shared<SceneObject>();
            object2->p_model.Load("../assets/FlightHelmet/glTF/FlightHelmet.gltf", m_graphics);

            std::shared_ptr<SceneObject> object3 = std::make_shared<SceneObject>();
            object3->p_model.Load("../assets/revolver/revolver.gltf", m_graphics);
            
            // Creating skybox 
            std::shared_ptr<Skybox> skybox = std::make_shared<Skybox>();
            skybox->p_model.Load("../assets/Box.gltf", m_graphics);

            // Adding scene objects
            g_scene->AddSceneObect(object3);
            //g_scene->AddSceneObect(object2);
            //g_scene->AddSceneCamera(g_scene_camera);
            g_scene->AddEditorCamera(g_editor_camera);
            g_scene->AddSkybox(skybox);

            m_graphics->Setup(g_scene);
            // Create renderer
            g_renderer = std::make_shared<Renderer>(m_graphics);

            glfwSetCursorPosCallback(m_graphics->GetWindow()->window(), mouse_callback);
            glfwSetScrollCallback(m_graphics->GetWindow()->window(), scroll_callback);
        }
    }
    void Application::Update()
    {
        auto current_time = std::chrono::high_resolution_clock::now();

        while (!m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();

            auto new_time = std::chrono::high_resolution_clock::now();
            float frame_time = std::chrono::duration<float, std::chrono::seconds::period>(new_time - current_time).count();
            current_time = new_time;

            g_renderer->RenderScene(g_scene, g_editor_camera, frame_time);
            //g_renderer->RenderScene(g_scene, g_scene_camera->p_camera.get(), frame_time);
            //g_scene_camera->p_camera->Update(frame_time, m_graphics->GetWindow()->window());
            g_editor_camera->OnUpdate(frame_time, m_graphics->GetWindow()->window());
            //std::cout << "frame time: " << 1000.0f / frame_time << std::endl;
        }
    }
    void Application::Destroy()
    {
        m_graphics->CleanUp();
        delete m_graphics;
    }
}