#include "Graphics.hpp"

namespace Diffuse {
    class Application {
    public:
        void Init();
        void Update();
        void Destroy();
    private:
        Graphics* m_graphics;
        Config m_config;
    };
}