// main.cpp
#include <iostream>
#include <aiecad/core/Application.hpp>
#include <aiecad/core/Specifications.hpp>  // AppSpecification

int main(int /*argc*/, char ** /*argv*/) {
    aiecad::ApplicationSpecification spec{
        .name = "AIECAD",
        .sandboxRoot = {},
        .devMode = false,
        .maximized = false,
        .window_spec = {
            .title = "AIECAD",
            .width = 1600,
            .height = 900,
            .resizable = true,
            .vSync = true,
            .dpiScale = 1.f,
            .specVersion = 0
        }
    };

    try {
        aiecad::Application app{ spec };
        return app.run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
