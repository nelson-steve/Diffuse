#include "Application.hpp"

#include "Renderer.hpp"
#include "Model.hpp"

#include <iostream>

namespace Diffuse {

    static Renderer* renderer;
    //static std::vector<Mesh*> meshes;
    static Model* model;

    void Application::Init() {
        //meshes.push_back(new Mesh("assets/suzanne.obj"));
        //meshes.push_back(new Mesh("assets/cone.obj"));
        m_graphics = new GraphicsDevice();
        model = new Model("../assets/Avocado/Avocado.gltf", m_graphics);

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
            renderer->RenderMesh(model);
            //renderer->RenderMeshes(meshes);
            //m_graphics->Draw();
        }
    }
    void Application::Destroy()
    {
        delete renderer;
        delete model;
        //for (auto& mesh : meshes) {
        //    delete mesh;
        //}
        m_graphics->CleanUp();
        delete m_graphics;
    }
}