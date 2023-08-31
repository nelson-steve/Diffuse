#include "Application.hpp"

#include "Renderer.hpp"

#include <iostream>



namespace Diffuse {

    static Renderer* renderer;
    static Mesh* mesh;

    void Application::Init() {
        mesh = new Mesh("assets/suzanne.obj");
        m_graphics = new GraphicsDevice();
        renderer = new Renderer(m_graphics);

        //if (m_graphics->Init(m_config)) {
        //    std::cout << "SUCCESS";
        //}
        //else {
        //    std::cout << "Failure setting up Vulkan";
        //}
    }
    void Application::Update()
    {
        while (!m_graphics->GetWindow()->WindowShouldClose()) {
            m_graphics->GetWindow()->PollEvents();
            renderer->RenderMesh(*mesh);
            //m_graphics->Draw();
        }
    }
    void Application::Destroy()
    {
        delete renderer;
        delete mesh;
        m_graphics->CleanUp();
        delete m_graphics;
    }
}