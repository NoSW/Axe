#pragma once
#include <00Core/Config.hpp>
#include <00Core/Window/Window.hpp>
#include <06Pipeline/Pipeline.hpp>

namespace axe::app
{
class App;
i32 MainLoop(i32 argc, char** argv, app::App* app);

class App
{
public:
    //! independent from any user changes to the device and will be called only once during the live span of an application
    virtual bool init()                  = 0;

    //! partner of init()
    virtual void exit()                  = 0;

    //! will be called when user change settings of an application. e.g., quality settings such as MSAA, resolution
    virtual bool load()                  = 0;

    //! partner of load()
    virtual bool unload()                = 0;

    //! everything CPU only ... no Graphics API calls
    virtual bool update(float deltaTime) = 0;

    // only Graphics API draw calls and command buffer generation
    virtual bool draw()                  = 0;

    virtual const char* name() { return typeid(*this).name(); }

    i32 run(i32 argc, char** argv) { return MainLoop(argc, argv, this); }

public:
    std::unique_ptr<pipeline::Pipeline> mpPipeline = nullptr;
    std::unique_ptr<window::Window> mpWindow       = nullptr;
};
}  // namespace axe::app
