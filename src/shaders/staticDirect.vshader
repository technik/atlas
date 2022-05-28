//--------------------------------------------------------------------------------------------------
// Revolution Engine
//--------------------------------------------------------------------------------------------------
// Copyright 2019 Carmelo J Fdez-Aguera

#include "staticDirect.h"

// Constant buffer bindings
ConstantBuffer<InstanceData> gInstanceData : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

// Vertex shader
VertexOutput main(VertexInput attributes)
{
    VertexOutput vertex;

    // Using the row-vector convention
    float4x4 worldViewProj = mul(gInstanceData.WorldMtx, gFrameData.ViewProj);

    vertex.pos = mul(float4(attributes.pos, 1.0), worldViewProj);
    vertex.normal = mul(gInstanceData.InvWorldMtx, float4(attributes.normal, 0.0));

    return vertex;
}