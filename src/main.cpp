#include "Application.hpp"

#include <iostream>

int main(int argc, char** argv[]) {
    Diffuse::Application* app = new Diffuse::Application();
    app->Init();
    app->Update();
    app->Destroy();
    delete app;

    return 0;
}