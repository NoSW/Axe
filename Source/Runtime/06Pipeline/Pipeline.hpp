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

enum class LoadFlag : u32
{
    RESIZE        = 1 << 0,
    SHADER        = 1 << 1,
    RENDER_TARGET = 1 << 2,
    ALL           = 0xffffffff
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