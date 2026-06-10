#include "AppConfig.hpp"
#include "Renderer.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv)
{
    try {
        Renderer renderer;
        renderer.render(parseAppConfig(argc, argv));
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
