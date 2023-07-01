#pragma once

#include "03Resource/Config.hpp"
#include "03Resource/Image/Image.hpp"

#include "02Rhi/Rhi.hpp"

#include "00Core/Thread/Thread.hpp"

#include <thread>
#include <queue>

namespace axe::resource
{

struct ResourceLoaderDesc
{
    rhi::Device* pDevice = nullptr;
    u32 stageBufferCount = 0;  // number of staging buffers,
    u32 stageBufferSize  = 0;
};

struct RequestLoadTextureDesc
{
    const std::filesystem::path filepath;

    rhi::Texture** ppOutRegisteredTexture = nullptr;
};

struct RequestLoadBufferDesc
{
    std::string_view name;        // gpu name of texture
    const void* pData = nullptr;  // cpu data
    rhi::BufferDesc bufferDesc;

    rhi::Buffer** ppOutRegisteredBuffer = nullptr;
};

struct RequestUpdateTextureDesc
{
    rhi::Texture* pTexture = nullptr;
    ImageDDS* pImageDDS    = nullptr;  // TODO: ?
};

struct RequestUpdateBufferDesc
{
    rhi::Buffer* pBuffer = nullptr;
    u64 offset           = 0;  // offset to mapped address

    const void* pData    = nullptr;  // cpu data
    u64 size             = 0;        // byte count of pData
};

struct Request
{
    enum Type
    {
        INVALID,
        UPDATE_BUFFER,
        UPDATE_TEXTURE,
    } type = INVALID;
    union
    {
        RequestUpdateBufferDesc bufferDesc{};
        RequestUpdateTextureDesc textureDesc;
    };

    Request() noexcept {};  // to avoid gcc error: implicitly deleted because the default definition would be ill-formed
    Request(Type type) noexcept : type(type) {}
};

// a resource loader with a standalone thread
class ResourceLoader
{
    AXE_NON_COPYABLE(ResourceLoader);

    struct CopyResourceSet
    {
        rhi::Fence* pFence                      = nullptr;
        rhi::Semaphore* pCopyCompletedSemaphore = nullptr;
        rhi::CmdPool* pCmdPool                  = nullptr;
        rhi::Cmd* pCmd                          = nullptr;
        rhi::Buffer* pBuffer                    = nullptr;
        u64 allocatedSpace                      = 0;
        bool isRecording                        = false;
        std::pmr::vector<rhi::Buffer*> pTempBuffers;  // created in case we ran out of space in the original staging buffer
                                                      // will be cleaned up after the fence for this set is complete
    };

public:
    void pushRequest(RequestUpdateBufferDesc& desc) noexcept
    {
        _mQueueMutex.lock();
        _mRequestQueue.push({Request::UPDATE_BUFFER});
        _mRequestQueue.back().bufferDesc = desc;
        _mQueueMutex.unlock();
    }

    void pushRequest(RequestUpdateTextureDesc& desc) noexcept
    {
        _mQueueMutex.lock();
        _mRequestQueue.push(Request{Request::UPDATE_TEXTURE});
        _mRequestQueue.back().textureDesc = desc;
        _mQueueMutex.unlock();
    }

    void pushRequest(RequestLoadTextureDesc&) noexcept;
    void pushRequest(RequestLoadBufferDesc&) noexcept;

    // wait all requests to be done manually (NOTE: please make sure no pushRequest following this call)
    // (It also be called in destructor automatically)
    void waitIdle() noexcept;

    ResourceLoader(ResourceLoaderDesc&) noexcept;
    ~ResourceLoader() noexcept;

private:
    // begin update buffer, and return mapped address
    [[nodiscard]] u32 _updateBuffer(const RequestUpdateBufferDesc&) noexcept;

    [[nodiscard]] u32 _updateTexture(const RequestUpdateTextureDesc&) noexcept;

    void _loop() noexcept;

    [[nodiscard]] bool _popRequest(Request& outRequest) noexcept
    {
        _mQueueMutex.lock();
        const bool isNonEmpty = !_mRequestQueue.empty();
        if (isNonEmpty)
        {
            outRequest = _mRequestQueue.front();
            _mRequestQueue.pop();
        }
        _mQueueMutex.unlock();
        return isNonEmpty;
    }

private:
    rhi::Queue* _mpQueue = nullptr;                    // queue used to submit resources to gpu
    std::pmr::vector<CopyResourceSet> _mResourceSets;  // all resource sets that can be used concurrently
    u32 _mNextAvailableSet = 0;                        // index of next available resource set

    std::queue<Request> _mRequestQueue;
    std::mutex _mQueueMutex;  // TODO: remove mutex, and use lockfree queue

    rhi::Device* _mpDevice = nullptr;                     // which device attach to
    std::pmr::vector<rhi::Semaphore*> _mpWaitSemaphores;  // external semaphores need to be waited
    u64 _mBufferSize = 0;                                 // buffer size for each resource set

    std::jthread _mStreamerThread;  // thread to stream data from cpu to gpu
    bool _mIsRunning = true;
};

}  // namespace axe::resource
