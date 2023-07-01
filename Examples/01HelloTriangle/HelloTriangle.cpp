
#include <14App/App.hpp>

#include "00Core/Math/Math.hpp"
#include "00Core/Memory/Memory.hpp"

#include "03Resource/Image/Image.hpp"

#include "00Core/Utils/Utils.hpp"

#include <iostream>

using namespace axe;

class HelloTriangle final : public app::App
{
    bool init() override
    {
        return true;
    }

    void exit() override
    {
    }

    bool load() override { return true; }

    bool unload() override { return true; }

    bool update(float deltaTime) override { return true; }

    bool draw() override
    {
        // int* p = new int;
        int s;
        // AXE_INFO("{:#018x} {:#018x} {:#018X}", (u64)p, (u64)&s, (u64)(void*)foo);

        constexpr int v6 = math::max(3, 4);
        // auto x  = memory::make_owner<float>((float)3.0);
        // auto ob = memory::make_observer(x);
        return true;
    }
};

int main(int argc, char** argv)
{
    HelloTriangle app;
    return app.run(argc, argv);
}
