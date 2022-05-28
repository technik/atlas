//--------------------------------------------------------------------------------------------------
// Revolution Engine
//--------------------------------------------------------------------------------------------------
// Copyright 2019 Carmelo J Fdez-Aguera
#pragma once

#ifdef __cplusplus
#include <math/matrix.h>
#include <math/vector.h>

namespace hlsl {

using float3 = math::Vec3f;
using float4 = math::Vec3f;
using float4x4 = math::Mat44f;

#else

struct VertexInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
};
#endif

struct InstanceData
{
    float4x4 WorldMtx;
    float4x4 InvWorldMtx;
};

struct FrameData
{
    float4x4 ViewProj;
};

#ifdef __cplusplus

} // namespace hlsl

#endif