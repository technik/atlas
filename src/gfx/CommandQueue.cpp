#include "CommandQueue.h"
#include "dxHelpers.h"

namespace gfx {

    CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type, ID3D12Device& device, LPCWSTR name)
        : m_commandType(type)
    {
        auto& rc = *RenderContext();

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        // Create native queue
        ThrowIfFailed(device.CreateCommandQueue(&desc, IID_PPV_ARGS(&m_dx12CommandQueue)));
        m_dx12CommandQueue->SetName(name);

        // Create a fence to keep track of work executed in this queue
        ThrowIfFailed(device.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    }

    CommandQueue::~CommandQueue()
    {
        flush();
    }

    auto CommandQueue::getNewCommandList() -> std::shared_ptr<CommandList>
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        if (!m_InFlightCommands.empty() && isFenceComplete(m_InFlightCommands.front().finishFence))
        {
            auto cmdList = m_InFlightCommands.front().commandList;
            m_InFlightCommands.pop();

            cmdList->Reset();

            return cmdList;
        }
        else
        {
            return std::make_shared<CommandList>(m_commandType);
        }
    }

    AwaitableFence CommandQueue::submitCommandList(std::shared_ptr<CommandList> cmdList)
    {
        ID3D12CommandList* const cmdListArray[] = { cmdList->m_dx12CommandList.Get() };
        m_dx12CommandQueue->ExecuteCommandLists(1, cmdListArray);

        auto& submission = m_InFlightCommands.emplace();
        submission.finishFence = signal();
        submission.commandList = cmdList;

        return submission.finishFence;
    }

    AwaitableFence CommandQueue::signal()
    {
        uint64_t fenceValueForSignal = m_nextFenceValue++;
        ThrowIfFailed(m_dx12CommandQueue->Signal(m_fence.Get(), fenceValueForSignal));

        return AwaitableFence(this, fenceValueForSignal);
    }

    bool CommandQueue::isFenceComplete(const AwaitableFence& fence)
    {
        // Early out
        if (fence.m_fenceValue <= m_lastCompletedFenceValue)
        {
            return true;
        }
        // Query the driver
        m_lastCompletedFenceValue = m_fence->GetCompletedValue();
        return m_lastCompletedFenceValue >= fence.m_fenceValue;
    }

    void CommandQueue::syncToFence(const AwaitableFence& fence)
    {
        if (isFenceComplete(fence)) // Early out
        {
            return;
        }
        m_dx12CommandQueue->Wait(m_fence.Get(), fence.m_fenceValue);
    }

    void CommandQueue::syncToOtherQueue(CommandQueue& otherQueue, const AwaitableFence& fence)
    {
        // Make this queue wait for the other queue to signal the fence.
        m_dx12CommandQueue->Wait(otherQueue.m_fence.Get(), fence.m_fenceValue);
    }

    void CommandQueue::spinOnFence(const AwaitableFence& t)
    {
        while (!isFenceComplete(t))
        {
            // TODO: Instead of spinning, maybe give up the thread and enqueue ourselves to resume execution
        }
        m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, t.m_fenceValue);
    }

    void CommandQueue::flush()
    {
        spinOnFence(signal());
    }

    void CommandQueue::suspend(uint64_t fenceValue, std::coroutine_handle<> h)
    {
        m_suspendedCoroutines.emplace_back(fenceValue, h);
    }

    void CommandQueue::consumeCoroutines()
    {
        // Refresh fence signals
        m_lastCompletedFenceValue = m_fence->GetCompletedValue();

        for (auto& [fenceValue, h] : m_suspendedCoroutines)
        {
            if (m_lastCompletedFenceValue >= fenceValue)
            {
                h.resume();
            }
        }
    }
} // namespace gfx