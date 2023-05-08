#pragma once
#include <02Rhi/Rhi.hpp>

namespace axe::core
{
class Window;
}

namespace axe::pipeline
{

struct PipelineDesc
{
    window::Window* pWindow = nullptr;
    std::string_view appName;
};

enum LoadFlag
{
    LOAD_FLAG_RESIZE        = 0x1,
    LOAD_FLAG_SHADER        = 0x2,
    LOAD_FLAG_RENDER_TARGET = 0x4,
    LOAD_FLAG_ALL           = 0xffffffff
};
using LoadFlagOneBit = LoadFlag;

class Pipeline
{
public:
    virtual ~Pipeline() noexcept              = default;
    virtual bool init(PipelineDesc&) noexcept = 0;
    virtual bool exit() noexcept              = 0;
    virtual bool load(LoadFlag) noexcept      = 0;
    virtual bool unload(LoadFlag) noexcept    = 0;
    virtual void update() noexcept            = 0;
    virtual void draw() noexcept              = 0;
};

}  // namespace axe::pipeline