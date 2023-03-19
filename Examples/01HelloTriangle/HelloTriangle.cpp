
#include <14App/App.hpp>
#include <iostream>

#include "00Core/Math/Math.hpp"
#include "00Core/Memory/Memory.hpp"
#if _MSC_VER && !__clang__  // visual studio only
#include <vld.h>
#endif
using namespace axe;

class HelloTriangle final : public app::App
{
    bool init() override { return true; }

    void exit() override {}

    bool load() override { return true; }

    bool unload() override { return true; }

    bool update(float deltaTime) override { return true; }

    bool draw() override
    {
        int v6 = axe::math::max(3, 4);
        // auto x  = memory::make_owner<float>((float)3.0);
        // auto ob = memory::make_observer(x);
        return true;
    }
};

int main(int argc, char** argv)
{
    HelloTriangle app;
    void* p = malloc(4);
    int* p2 = new int;
    auto r  = app.run(argc, argv);
    int* p3 = new int;
    return r;
}
