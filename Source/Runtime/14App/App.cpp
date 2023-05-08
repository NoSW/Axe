#include "14App/App.hpp"
#include "06Pipeline/Forward.hpp"
namespace axe::app
{

i32 MainLoop(i32 argc, char** argv, app::App* app)
{
    /* init logging */
    {
        log::init();
    }
    /* init Window */
    {
        app->pWindow = std::make_unique<window::Window>();
        struct window::WindowDesc windowDesc
        {
            .title = app->name()
        };
        bool succ = app->pWindow->init(windowDesc);

        AXE_ASSERT(succ);
    }

    /* init Backend */
    {
        pipeline::PipelineDesc pipelineDesc;
        pipelineDesc.appName = app->name();
        pipelineDesc.pWindow = app->pWindow.get();
        app->mpPipeline      = std::make_unique<pipeline::Forward>();
        bool succ            = app->mpPipeline->init(pipelineDesc);
        AXE_ASSERT(succ);
    }
    {
        bool succ = app->mpPipeline->load(pipeline::LoadFlag::ALL);
        AXE_ASSERT(succ);
    }
    /* init app */
    {
        bool succ = app->init();
        AXE_ASSERT(succ);
        succ = app->load();
        AXE_ASSERT(succ);
    }

    /* begin loop */
    {
        while (!app->pWindow->isQuit())
        {
            if (app->pWindow->isResized())
            {
                if (!app->mpPipeline->unload(pipeline::LoadFlag::RESIZE))
                {
                    AXE_ERROR("Failed to unload pipeline");
                    return 1;
                };

                if (!app->mpPipeline->load(pipeline::LoadFlag::RESIZE))
                {
                    AXE_ERROR("Failed to load pipeline");
                    return 1;
                }
            }
            {
                app->pWindow->update(30.0f);
            }
            {
                app->mpPipeline->update();
                if (!app->pWindow->isMinimized())
                {
                    app->mpPipeline->draw();
                }
            }
            {
                app->update(30.0f);
                if (!app->pWindow->isMinimized())
                {
                    app->draw();
                }
            }
        }
    }

    /* exit app */
    {
        bool succ = app->unload();
        AXE_ASSERT(succ);
        app->exit();
    }

    /* exit window */
    {
        app->pWindow->exit();
    }

    /* exit backend */
    {
        bool succ = app->mpPipeline->unload(pipeline::LoadFlag::ALL);
        AXE_ASSERT(succ);
        app->mpPipeline->exit();
    }

    /* exit logging */
    {
        log::exit();
    }

    return 0;
}

}  // namespace axe::app