#pragma once

#include "GraphicsDevice.hpp"

namespace Diffuse {
    class Application {
    public:
        void Init();
        void Update();
        void Destroy();
     private:
        GraphicsDevice* m_graphics;
        Config m_config;
    };
}