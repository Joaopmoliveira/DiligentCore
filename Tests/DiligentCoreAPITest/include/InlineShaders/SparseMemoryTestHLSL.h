/*
 *  Copyright 2019-2021 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <string>

namespace
{

namespace HLSL
{

const std::string FillBuffer_CS{R"hlsl(
RWStructuredBuffer<uint> g_DstBuffer : register(u0);

cbuffer CB
{
    uint Offset;
    uint Size;
    uint Pattern;
    uint padding;
};

[numthreads(16, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
    if (DTid < Size)
    {
        g_DstBuffer[Offset + DTid] = Pattern;
    }
}
)hlsl"};


const std::string SparseMemoryTest_VS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

void main(in uint vid : SV_VertexID,
          out PSInput PSIn) 
{
    // fullscreen triangle
    float2 uv = float2(vid >> 1, vid & 1) * 2.0;
    PSIn.Pos  = float4(uv * 2.0 - 1.0, 0.0, 1.0);
}
)hlsl"};


const std::string SparseBuffer_PS{R"hlsl(
struct PSInput 
{ 
    float4 Pos : SV_POSITION;
};

StructuredBuffer<uint> g_Buffer;

float4 main(in PSInput PSIn) : SV_Target
{
    uint Idx         = uint(PSIn.Pos.x) + uint(PSIn.Pos.y) * SCREEN_WIDTH;
    uint PackedColor = g_Buffer[Idx];

    float4 Color;
    Color.r = (PackedColor & 0xFF) / 255.0;
    Color.g = ((PackedColor >> 8) & 0xFF) / 255.0;
    Color.b = ((PackedColor >> 16) & 0xFF) / 255.0;
    Color.a = 1.0;

    return Color;
}
)hlsl"};

} // namespace HLSL

} // namespace
