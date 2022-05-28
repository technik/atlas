#pragma once

#include "DX12Context.h"
#include "dxHelpers.h"

namespace gfx
{
    class GPUBuffer
    {
    public:
        size_t size;
        ComPtr<ID3D12Resource> dx12Resource;
    };
}