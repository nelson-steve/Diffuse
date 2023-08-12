#include "Application.hpp"

int main(int argc, char** argv[]) {
    Diffuse::Application* app = new Diffuse::Application();
    app->Init();
    //app->Update();
    delete app;

    return 0;
}