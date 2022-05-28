//--------------------------------------------------------------------------------------------------
// Revolution Engine
//--------------------------------------------------------------------------------------------------
// Copyright 2021 Carmelo J Fdez-Aguera
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "rasterPipeline.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "dxHelpers.h"

#include <dxcapi.h>

RasterPipeline::RasterPipeline(
    ComPtr<ID3D12RootSignature> rootSignature,
    std::string_view vtxShaderFilename,
    std::string_view pxlShaderFilename,
    bool depthTest,
    bool blend
)
	: m_vtxShader(vtxShaderFilename)
	, m_pxlShader(pxlShaderFilename)
	, m_rootSignature(rootSignature)
	, m_depthTest(depthTest)
	, m_blend(blend)
{
	m_dx12Pipeline = tryLoad();
}

/*
void RasterPipeline::bind(const vk::CommandBuffer& cmdBuf)
{
	if (m_invalidated)
	{
		if (reload())
			m_invalidated = false;
	}
	cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vkPipeline);
}*/

bool RasterPipeline::reload()
{
    ComPtr<ID3D12PipelineState> newPipeline = tryLoad();
	if (newPipeline)
	{
        // TODO: Wait for the old shader to no longer be in use
		// RenderContext().device().waitIdle();
		m_dx12Pipeline = newPipeline;
		return true;
	}

	return false;
}

ComPtr<ID3D12PipelineState> RasterPipeline::tryLoad()
{
    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(stringToWideString(m_vtxShader).c_str(), &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(stringToWideString(m_pxlShader).c_str(), &pixelShaderBlob));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterState;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_DEPTH_STENCIL_DESC depthStencil { CD3DX12_DEFAULT() };
    depthStencil.DepthEnable = true;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;

    CD3DX12_RASTERIZER_DESC rasterState { CD3DX12_DEFAULT() };
    rasterState.FrontCounterClockwise = true;
    rasterState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;

    pipelineStateStream.pRootSignature = m_rootSignature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;
    pipelineStateStream.DepthStencil = depthStencil;
    pipelineStateStream.RasterState = rasterState;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ComPtr<ID3D12PipelineState> pipeline;
    auto device = gfx::RenderContext()->m_dx12Device;
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline)));

    return pipeline;
}
