#pragma once

#include "DX12Context.h"
#include <queue>
#include <core/coroutines.h>
#include <gfx/CommandList.h>

namespace gfx {

    class CommandQueue;

    struct AwaitableFence
    {
        AwaitableFence() = default;

        bool await_ready() const; // Shortcut to avoid delayed execution
        
        void await_suspend(std::coroutine_handle<> h);

        void await_resume()
        {}

    private:
        AwaitableFence(CommandQueue* queue, uint64_t value)
            : m_queue(queue)
            , m_fenceValue(value)
        {}

        uint64_t m_fenceValue{};

        CommandQueue* m_queue{};

        friend class CommandQueue;
    };

    class CommandQueue
    {
    public:
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
        using CmdList = ID3D12GraphicsCommandList;

        CommandQueue(D3D12_COMMAND_LIST_TYPE type, ID3D12Device& device, LPCWSTR name);
        ~CommandQueue();

        // Get a command list ready to record to
        std::shared_ptr<CommandList> getNewCommandList();

        // Submit a command list for execution and return its finish fence.
        // This queue will keep a reference to the command list as long as it is in flight.
        AwaitableFence submitCommandList(std::shared_ptr<CommandList>);

        AwaitableFence signal();

        // Check weather a specific fence value has been reached in this queue
        bool isFenceComplete(const AwaitableFence& fence);

        // Make this queue to reach a fence value before consuming further submissions.
        void syncToFence(const AwaitableFence& fence);

        // Make this queue wait for another queue to signal a fence before consuming further submissions.
        // TODO: Because fence already knows its queue, this can now be unified with the method above
        void syncToOtherQueue(CommandQueue& otherQueue, const AwaitableFence& fence);

        // Make this thread spin until the queue reaches a specific fence value
        void spinOnFence(const AwaitableFence& fence);

        // Spin on the cpu side for the queue to be completely emptied
        void flush();

        ComPtr<ID3D12CommandQueue> getNativeQueue() const { return m_dx12CommandQueue; }

        void ResourceBarrier(
            ComPtr<ID3D12GraphicsCommandList> commandList,
            ComPtr<ID3D12Resource> resource,
            D3D12_RESOURCE_STATES beforeState,
            D3D12_RESOURCE_STATES afterState)
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource.Get(),
                beforeState, afterState);

            commandList->ResourceBarrier(1, &barrier);
        }

        void consumeCoroutines();

    private:

        struct CmdListSubmission
        {
            AwaitableFence finishFence;
            std::shared_ptr<CommandList> commandList;
        };

        D3D12_COMMAND_LIST_TYPE m_commandType;

        ComPtr<ID3D12CommandQueue> m_dx12CommandQueue;
        ComPtr<ID3D12Fence> m_fence;
        uint64_t m_nextFenceValue = 1;
        uint64_t m_lastCompletedFenceValue = 0;

        std::queue<CmdListSubmission> m_InFlightCommands;

        // Coroutine support
        friend class AwaitableFence;

        void suspend(uint64_t fenceValue, std::coroutine_handle<> h);

        std::vector<std::pair<uint64_t, std::coroutine_handle<>>> m_suspendedCoroutines;
    };

    inline bool AwaitableFence::await_ready() const // Shortcut to avoid delayed execution
    {
        return !m_queue || m_queue->isFenceComplete(*this);
    }

    inline void AwaitableFence::await_suspend(std::coroutine_handle<> h)
    {
        assert(m_queue);
        m_queue->suspend(m_fenceValue, h);
    }
} // namespace gfx