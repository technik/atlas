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
#include "ComputePipeline.h"

#include <fstream>
#include <iostream>

#include <dxcapi.h>
#include "dxHelpers.h"

#include <d3dcompiler.h>

namespace {
    HRESULT compileShader(LPCWSTR fileName, LPCSTR entryPoint, LPCSTR profile, ID3DBlob*& outBlob)
    {
        if (!fileName || !entryPoint || !profile)
            return E_INVALIDARG;

        outBlob = nullptr;

        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
        flags |= D3DCOMPILE_DEBUG;
#endif

        const D3D_SHADER_MACRO defines[] =
        {
            "EXAMPLE_DEFINE", "1",
            NULL, NULL
        };

        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = D3DCompileFromFile(fileName, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint, profile,
            flags, 0, &shaderBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            if (shaderBlob)
                shaderBlob->Release();

            return hr;
        }

        outBlob = shaderBlob;

        return hr;
    }

    ComPtr<ID3D12PipelineState> compileShader(const std::string_view fileName);
}

ComputePipeline::ComputePipeline(
    ComPtr<ID3D12RootSignature> rootSignature,
    std::string_view shaderFilename
)
	: m_shader(shaderFilename)
	, m_rootSignature(rootSignature)
{
	m_dx12Pipeline = tryLoad();
}

/*void ComputePipeline::bind(const vk::CommandBuffer& cmdBuf)
{
	if (m_invalidated)
	{
		if (reload())
			m_invalidated = false;
	}
	cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vkPipeline);
}*/

bool ComputePipeline::reload()
{
    ComPtr<ID3D12PipelineState> newPipeline = tryLoad();
	if (newPipeline)
	{
		// TODO: Wait until the pipeline is no longer in use 
        // RenderContext()->m_dx12Device->waitIdle();
		m_dx12Pipeline = newPipeline;
		return true;
	}

	return false;
}

ComPtr<ID3D12PipelineState> ComputePipeline::tryLoad()
{
    assert(false && "Not implemented");
    return nullptr;
}