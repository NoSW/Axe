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
        app->mpWindow = std::make_unique<window::Window>();
        struct window::WindowDesc windowDesc
        {
            .mTitle = app->name()
        };
        bool succ = app->mpWindow->init(windowDesc);

        AXE_ASSERT(succ);
    }
    /* init Backend */
    {
        pipeline::PipelineDesc pipelineDesc;
        pipelineDesc.mAppName = app->name();
        pipelineDesc.mpWindow = app->mpWindow.get();
        app->mpPipeline       = std::make_unique<pipeline::Forward>();
        bool succ             = app->mpPipeline->init(pipelineDesc);
        AXE_ASSERT(succ);
    }
    {
        bool succ = app->mpPipeline->load(pipeline::LOAD_FLAG_ALL);
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
        while (!app->mpWindow->isQuit())
        {
            if (app->mpWindow->isResized())
            {
                if (!app->mpPipeline->unload(pipeline::LOAD_FLAG_RESIZE))
                {
                    AXE_ERROR("Failed to unload pipeline");
                    return 1;
                };

                if (!app->mpPipeline->load(pipeline::LOAD_FLAG_RESIZE))
                {
                    AXE_ERROR("Failed to load pipeline");
                    return 1;
                }
            }
            {
                app->mpWindow->update(30.0f);
            }
            {
                app->mpPipeline->update();
                if (!app->mpWindow->isMinimized())
                {
                    app->mpPipeline->draw();
                }
            }
            {
                app->update(30.0f);
                if (!app->mpWindow->isMinimized())
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
        app->mpWindow->exit();
    }

    /* exit backend */
    {
        bool succ = app->mpPipeline->unload(pipeline::LoadFlag::LOAD_FLAG_ALL);
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