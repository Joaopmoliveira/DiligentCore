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

#include "TestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "ShaderMacroHelper.hpp"
#include "MapHelper.hpp"
#include "Align.hpp"
#include "BasicMath.hpp"

#if METAL_SUPPORTED
#    include "RenderDeviceMtl.h"
#endif

#include "InlineShaders/SparseMemoryTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
#endif

#if D3D12_SUPPORTED
#endif

#if VULKAN_SUPPORTED
#endif

#if METAL_SUPPORTED
#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

class SparseMemoryTest : public testing::TestWithParam<int> //::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        if (!pDevice->GetDeviceInfo().Features.SparseMemory)
            return;

        // Find context.
        constexpr auto QueueTypeMask = COMMAND_QUEUE_TYPE_SPARSE_BINDING;
        for (Uint32 CtxInd = 0; CtxInd < pEnv->GetNumImmediateContexts(); ++CtxInd)
        {
            auto*       Ctx  = pEnv->GetDeviceContext(CtxInd);
            const auto& Desc = Ctx->GetDesc();

            if ((Desc.QueueType & QueueTypeMask) == QueueTypeMask)
            {
                sm_pSparseBindingCtx = Ctx;
                break;
            }
        }

        if (!sm_pSparseBindingCtx)
            return;

        // Fill buffer PSO
        {
            BufferDesc BuffDesc;
            BuffDesc.Name           = "Fill buffer parameters";
            BuffDesc.Size           = sizeof(Uint32) * 4;
            BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
            BuffDesc.Usage          = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

            pDevice->CreateBuffer(BuffDesc, nullptr, &sm_pFillBufferParams);
            ASSERT_NE(sm_pFillBufferParams, nullptr);

            ShaderCreateInfo ShaderCI;
            ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.UseCombinedTextureSamplers = true;
            ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
            ShaderCI.EntryPoint                 = "main";
            ShaderCI.Desc.Name                  = "Fill buffer CS";
            ShaderCI.Source                     = HLSL::FillBuffer_CS.c_str();
            RefCntAutoPtr<IShader> pCS;
            pDevice->CreateShader(ShaderCI, &pCS);
            ASSERT_NE(pCS, nullptr);

            ComputePipelineStateCreateInfo PSOCreateInfo;
            PSOCreateInfo.PSODesc.Name         = "Fill buffer PSO";
            PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
            PSOCreateInfo.pCS                  = pCS;

            const ShaderResourceVariableDesc Variables[] =
                {
                    {SHADER_TYPE_COMPUTE, "CB", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
                    {SHADER_TYPE_COMPUTE, "g_DstBuffer", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS} //
                };
            PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Variables;
            PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Variables);

            pDevice->CreateComputePipelineState(PSOCreateInfo, &sm_pFillBufferPSO);
            ASSERT_NE(sm_pFillBufferPSO, nullptr);

            sm_pFillBufferPSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "CB")->Set(sm_pFillBufferParams);

            sm_pFillBufferPSO->CreateShaderResourceBinding(&sm_pFillBufferSRB, true);
            ASSERT_NE(sm_pFillBufferSRB, nullptr);
        }

        // Fill texture PSO
        {
            BufferDesc BuffDesc;
            BuffDesc.Name           = "Fill texture parameters";
            BuffDesc.Size           = sizeof(Uint32) * 8;
            BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
            BuffDesc.Usage          = USAGE_DYNAMIC;
            BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

            pDevice->CreateBuffer(BuffDesc, nullptr, &sm_pFillTextureParams);
            ASSERT_NE(sm_pFillTextureParams, nullptr);

            ShaderCreateInfo ShaderCI;
            ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
            ShaderCI.UseCombinedTextureSamplers = true;
            ShaderCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
            ShaderCI.EntryPoint                 = "main";
            ShaderCI.Desc.Name                  = "Fill texture CS";
            ShaderCI.Source                     = HLSL::FillTexture_CS.c_str();
            RefCntAutoPtr<IShader> pCS;
            pDevice->CreateShader(ShaderCI, &pCS);
            ASSERT_NE(pCS, nullptr);

            ComputePipelineStateCreateInfo PSOCreateInfo;
            PSOCreateInfo.PSODesc.Name         = "Fill texture PSO";
            PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
            PSOCreateInfo.pCS                  = pCS;

            const ShaderResourceVariableDesc Variables[] =
                {
                    {SHADER_TYPE_COMPUTE, "CB", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
                    {SHADER_TYPE_COMPUTE, "g_DstTexture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC} //
                };
            PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Variables;
            PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Variables);

            pDevice->CreateComputePipelineState(PSOCreateInfo, &sm_pFillTexturePSO);
            ASSERT_NE(sm_pFillTexturePSO, nullptr);

            sm_pFillTexturePSO->GetStaticVariableByName(SHADER_TYPE_COMPUTE, "CB")->Set(sm_pFillTextureParams);

            sm_pFillTexturePSO->CreateShaderResourceBinding(&sm_pFillTextureSRB, true);
            ASSERT_NE(sm_pFillTextureSRB, nullptr);
        }
    }

    static void TearDownTestSuite()
    {
        sm_pSparseBindingCtx.Release();

        sm_pFillBufferPSO.Release();
        sm_pFillBufferSRB.Release();
        sm_pFillBufferParams.Release();

        sm_pFillTexturePSO.Release();
        sm_pFillTextureSRB.Release();
        sm_pFillTextureParams.Release();
    }

    static RefCntAutoPtr<IBuffer> CreateSparseBuffer(Uint64 Size, BIND_FLAGS BindFlags, SPARSE_RESOURCE_FLAGS SparseFlags = SPARSE_RESOURCE_FLAG_NONE)
    {
        auto*          pDevice = TestingEnvironment::GetInstance()->GetDevice();
        constexpr auto Stride  = 4u;

        BufferDesc Desc;
        Desc.Name              = "Sparse buffer";
        Desc.Size              = AlignDown(Size, Stride);
        Desc.BindFlags         = BindFlags | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS; // UAV for fill buffer, SRV to read in PS
        Desc.Usage             = USAGE_SPARSE;
        Desc.SparseFlags       = SparseFlags;
        Desc.Mode              = BUFFER_MODE_STRUCTURED;
        Desc.ElementByteStride = Stride;

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(Desc, nullptr, &pBuffer);
        return pBuffer;
    }

    static RefCntAutoPtr<IDeviceMemory> CreateMemory(Uint32 PageSize, Uint32 NumPages, IDeviceObject* pCompatibleResource)
    {
        auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();

        DeviceMemoryCreateInfo MemCI;
        MemCI.Desc.Name             = "Memory for sparse resources";
        MemCI.Desc.Type             = DEVICE_MEMORY_TYPE_SPARSE;
        MemCI.Desc.PageSize         = PageSize;
        MemCI.InitialSize           = NumPages * PageSize;
        MemCI.ppCompatibleResources = &pCompatibleResource;
        MemCI.NumResources          = pCompatibleResource == nullptr ? 0 : 1;

        RefCntAutoPtr<IDeviceMemory> pMemory;
        pDevice->CreateDeviceMemory(MemCI, &pMemory);
        if (pMemory == nullptr)
            return {};

        // Even if resize is not supported function must return 'true'
        if (!pMemory->Resize(MemCI.InitialSize))
            return {};

        VERIFY_EXPR(pMemory->GetCapacity() == NumPages * PageSize);

        return pMemory;
    }

    struct TextureAndMemory
    {
        RefCntAutoPtr<ITexture>      pTexture;
        RefCntAutoPtr<IDeviceMemory> pMemory;
    };
    static TextureAndMemory CreateSparseTextureAndMemory(int4 Dim, BIND_FLAGS BindFlags, SPARSE_RESOURCE_FLAGS SparseFlags, Uint32 MemoryPageSize, Uint32 NumMemoryPages)
    {
        auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();

        TextureDesc Desc;
        if (Dim.z > 1)
        {
            VERIFY_EXPR(Dim.w <= 1);
            Desc.Type  = RESOURCE_DIM_TEX_3D;
            Desc.Depth = static_cast<Uint32>(Dim.z);
        }
        else
        {
            VERIFY_EXPR(Dim.z <= 1);
            Desc.Type      = Dim.w > 1 ? RESOURCE_DIM_TEX_2D_ARRAY : RESOURCE_DIM_TEX_2D;
            Desc.ArraySize = static_cast<Uint32>(Dim.w);
        }

        Desc.Width       = static_cast<Uint32>(Dim.x);
        Desc.Height      = static_cast<Uint32>(Dim.y);
        Desc.Format      = TEX_FORMAT_RGBA8_UNORM;
        Desc.MipLevels   = 0; // full mip chain
        Desc.SampleCount = 1;
        Desc.BindFlags   = BindFlags | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS; // UAV for fill texture, SRV to read in PS
        Desc.Usage       = USAGE_SPARSE;
        Desc.SparseFlags = SparseFlags;

        TextureAndMemory Result;
        if (pDevice->GetDeviceInfo().IsMetalDevice())
        {
#if METAL_SUPPORTED
            Result.pMemory = CreateMemory(MemoryPageSize, NumMemoryPages, nullptr);
            if (Result.pMemory == nullptr)
                return {};

            RefCntAutoPtr<IRenderDeviceMtl> pDeviceMtl{pDevice, IID_RenderDeviceMtl};
            pDeviceMtl->CreateSparseTexture(Desc, Result.pMemory, &Result.pTexture);
#endif
        }
        else
        {
            pDevice->CreateTexture(Desc, nullptr, &Result.pTexture);
            if (Result.pTexture == nullptr)
                return {};

            Result.pMemory = CreateMemory(MemoryPageSize, NumMemoryPages, Result.pTexture);
        }
        return Result;
    }

    static RefCntAutoPtr<IFence> CreateFence()
    {
        auto* pDevice = TestingEnvironment::GetInstance()->GetDevice();

        FenceDesc Desc;
        Desc.Name = "Fence";
        Desc.Type = FENCE_TYPE_GENERAL;

        RefCntAutoPtr<IFence> pFence;
        pDevice->CreateFence(Desc, &pFence);

        return pFence;
    }

    static void FillBuffer(IDeviceContext* pContext, IBuffer* pBuffer, Uint64 Offset, Uint32 Size, Uint32 Pattern)
    {
        sm_pFillBufferSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstBuffer")->Set(pBuffer->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));

        const auto Stride = pBuffer->GetDesc().ElementByteStride;

        struct CB
        {
            Uint32 Offset;
            Uint32 Size;
            Uint32 Pattern;
            Uint32 padding;
        };
        {
            MapHelper<CB> CBConstants(pContext, sm_pFillBufferParams, MAP_WRITE, MAP_FLAG_DISCARD);
            CBConstants->Offset  = StaticCast<Uint32>(Offset / Stride);
            CBConstants->Size    = Size / Stride;
            CBConstants->Pattern = Pattern;
        }

        pContext->SetPipelineState(sm_pFillBufferPSO);
        pContext->CommitShaderResources(sm_pFillBufferSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DispatchComputeAttribs CompAttrs;
        CompAttrs.ThreadGroupCountX = (Size / Stride + 63) / 64;
        CompAttrs.ThreadGroupCountY = 1;
        CompAttrs.ThreadGroupCountZ = 1;
        pContext->DispatchCompute(CompAttrs);
    }

    static void FillTexture(IDeviceContext* pContext, ITexture* pTexture, const Rect& Region, Uint32 MipLevel, const float4& Color)
    {
        TextureViewDesc Desc;
        Desc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
        Desc.TextureDim      = RESOURCE_DIM_TEX_2D;
        Desc.MostDetailedMip = MipLevel;
        Desc.NumMipLevels    = 1;
        Desc.FirstArraySlice = 0;
        Desc.NumArraySlices  = 1;

        RefCntAutoPtr<ITextureView> pView;
        pTexture->CreateView(Desc, &pView);

        sm_pFillTextureSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_DstTexture")->Set(pView);

        struct CB
        {
            uint2  Offset;
            uint2  Size;
            float4 Color;
        };
        {
            MapHelper<CB> CBConstants(pContext, sm_pFillTextureParams, MAP_WRITE, MAP_FLAG_DISCARD);
            CBConstants->Offset = {static_cast<Uint32>(Region.left), static_cast<Uint32>(Region.top)};
            CBConstants->Size   = {static_cast<Uint32>(Region.right - Region.left), static_cast<Uint32>(Region.bottom - Region.top)};
            CBConstants->Color  = Color;
        }

        pContext->SetPipelineState(sm_pFillTexturePSO);
        pContext->CommitShaderResources(sm_pFillTextureSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DispatchComputeAttribs CompAttrs;
        CompAttrs.ThreadGroupCountX = (Region.right - Region.left + 7) / 8;
        CompAttrs.ThreadGroupCountY = (Region.bottom - Region.top + 7) / 8;
        CompAttrs.ThreadGroupCountZ = 1;
        pContext->DispatchCompute(CompAttrs);
    }

    static void DrawFSQuad(IDeviceContext* pContext, ISwapChain* pSwapChain)
    {
        auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();
        pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float ClearColor[4] = {};
        pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_NONE);

        DrawAttribs DrawAttrs{3, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(DrawAttrs);
    }

    static void CreateGraphicsPSO(const char* Name, const String& PSSource, RefCntAutoPtr<IPipelineState>& pPSO, RefCntAutoPtr<IShaderResourceBinding>& pSRB)
    {
        auto*       pEnv       = TestingEnvironment::GetInstance();
        auto*       pDevice    = pEnv->GetDevice();
        auto*       pSwapChain = pEnv->GetSwapChain();
        const auto& SCDesc     = pSwapChain->GetDesc();

        GraphicsPipelineStateCreateInfo PSOCreateInfo;

        auto& PSODesc          = PSOCreateInfo.PSODesc;
        auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = Name;

        PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        GraphicsPipeline.NumRenderTargets                     = 1;
        GraphicsPipeline.RTVFormats[0]                        = SCDesc.ColorBufferFormat;
        GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_BACK;
        GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
        GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = False;
        GraphicsPipeline.DepthStencilDesc.DepthEnable         = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = true;

        if (pDevice->GetDeviceInfo().IsVulkanDevice())
            ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC; // glslang does not support sparse residency status

        ShaderMacroHelper Macros;
        Macros.AddShaderMacro("SCREEN_WIDTH", SCDesc.Width);
        Macros.AddShaderMacro("SCREEN_HEIGHT", SCDesc.Height);
        ShaderCI.Macros = Macros;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Sparse resource test - VS";
            ShaderCI.Source          = HLSL::SparseMemoryTest_VS.c_str();

            pDevice->CreateShader(ShaderCI, &pVS);
            if (pVS == nullptr)
                return;
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Sparse resource test - PS";
            ShaderCI.Source          = PSSource.c_str();

            pDevice->CreateShader(ShaderCI, &pPS);
            if (pPS == nullptr)
                return;
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
        if (pPSO == nullptr)
            return;

        pPSO->CreateShaderResourceBinding(&pSRB);
        if (pSRB == nullptr)
        {
            pPSO.Release();
            return;
        }
    }

    static RefCntAutoPtr<IDeviceContext> sm_pSparseBindingCtx;

    static RefCntAutoPtr<IPipelineState>         sm_pFillBufferPSO;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pFillBufferSRB;
    static RefCntAutoPtr<IBuffer>                sm_pFillBufferParams;

    static RefCntAutoPtr<IPipelineState>         sm_pFillTexturePSO;
    static RefCntAutoPtr<IShaderResourceBinding> sm_pFillTextureSRB;
    static RefCntAutoPtr<IBuffer>                sm_pFillTextureParams;
};
RefCntAutoPtr<IDeviceContext>         SparseMemoryTest::sm_pSparseBindingCtx;
RefCntAutoPtr<IPipelineState>         SparseMemoryTest::sm_pFillBufferPSO;
RefCntAutoPtr<IShaderResourceBinding> SparseMemoryTest::sm_pFillBufferSRB;
RefCntAutoPtr<IBuffer>                SparseMemoryTest::sm_pFillBufferParams;
RefCntAutoPtr<IPipelineState>         SparseMemoryTest::sm_pFillTexturePSO;
RefCntAutoPtr<IShaderResourceBinding> SparseMemoryTest::sm_pFillTextureSRB;
RefCntAutoPtr<IBuffer>                SparseMemoryTest::sm_pFillTextureParams;

const auto TestParamRange = testing::Range(0, 2);

String TestIdToString(const testing::TestParamInfo<int>& info)
{
    String name;
    switch (info.param)
    {
        // clang-format off
        case 0:  name = "POT";    break;
        case 1:  name = "NonPOT"; break;
        default: name = std::to_string(info.param); UNEXPECTED("unsupported TestId");
            // clang-format on
    }
    return name;
}

int4 TestIdToTextureDim(Uint32 TestId)
{
    switch (TestId)
    {
        case 0: return int4{256, 256, 1, 1};
        case 1: return int4{253, 249, 1, 1};
        default: return int4{};
    }
}

void CheckTextureSparseProperties(ITexture* pTexture)
{
    const auto& Desc      = pTexture->GetDesc();
    const auto& Props     = pTexture->GetSparseProperties();
    const auto& SparseMem = TestingEnvironment::GetInstance()->GetDevice()->GetAdapterInfo().SparseMemory;

    ASSERT_GT(Props.MemorySize, 0u);
    ASSERT_TRUE(Props.MemorySize % SparseMem.SparseBlockSize == 0);

    ASSERT_EQ(Props.MemoryAlignment, SparseMem.SparseBlockSize);

    ASSERT_NE(Props.FirstMipInTail, 0u);
    ASSERT_LT(Props.FirstMipInTail, Desc.MipLevels);

    ASSERT_GT(Props.MipTailOffset, 0u);
    ASSERT_LT(Props.MipTailOffset, Props.MemorySize);
    ASSERT_TRUE(Props.MipTailOffset % SparseMem.SparseBlockSize == 0);

    ASSERT_GT(Props.MipTailSize, 0u);
    ASSERT_TRUE(Props.MipTailSize % SparseMem.SparseBlockSize == 0);
    ASSERT_EQ(Props.MemorySize, Props.MipTailOffset + Props.MipTailSize);

    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_STANDARD_2D_BLOCK_SHAPE) != 0)
    {
        ASSERT_EQ(Props.TileSize[0], 128u);
        ASSERT_EQ(Props.TileSize[1], 128u);
        ASSERT_EQ(Props.TileSize[2], 1u);
    }
}


TEST_F(SparseMemoryTest, SparseBuffer)
{
    auto*       pEnv      = TestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseMem = pDevice->GetAdapterInfo().SparseMemory;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse buffer is not supported by this device";
    }

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                break;
#endif
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                break;
#endif
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                break;
#endif
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO("Sparse buffer test", HLSL::SparseBuffer_PS, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto BlockSize = SparseMem.SparseBlockSize;
    const auto BuffSize  = BlockSize * 4;

    auto pBuffer = CreateSparseBuffer(BuffSize, BIND_NONE, SPARSE_RESOURCE_FLAG_NONE);
    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    const auto& BuffSparseProps = pBuffer->GetSparseProperties();
    ASSERT_GT(BuffSparseProps.MemoryAlignment, 0u);
    ASSERT_TRUE(IsPowerOfTwo(BuffSparseProps.MemoryAlignment));

    const auto MemBlockSize = std::max(AlignUp(BlockSize, BuffSparseProps.MemoryAlignment), BuffSparseProps.MemoryAlignment);

    auto pMemory = CreateMemory(MemBlockSize * 2, 4, pBuffer);
    ASSERT_NE(pMemory, nullptr);

    auto pFence = CreateFence();
    ASSERT_NE(pFence, nullptr);

    // bind sparse
    {
        const SparseBufferMemoryBindRange BindRanges[] = {
            {BlockSize * 0, MemBlockSize * 0, BlockSize, pMemory},
            {BlockSize * 1, MemBlockSize * 2, BlockSize, pMemory},
            {BlockSize * 2, MemBlockSize * 3, BlockSize, pMemory},
            {BlockSize * 3, MemBlockSize * 6, BlockSize, pMemory} //
        };

        SparseBufferMemoryBind SparseBuffBind;
        SparseBuffBind.pBuffer   = pBuffer;
        SparseBuffBind.NumRanges = _countof(BindRanges);
        SparseBuffBind.pRanges   = BindRanges;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        BindSparseMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumBufferBinds     = 1;
        BindSparseAttrs.pBufferBinds       = &SparseBuffBind;
        BindSparseAttrs.ppSignalFences     = &SignalFence;
        BindSparseAttrs.pSignalFenceValues = &SignalValue;
        BindSparseAttrs.NumSignalFences    = 1;

        sm_pSparseBindingCtx->BindSparseMemory(BindSparseAttrs);

        pContext->DeviceWaitForFence(SignalFence, SignalValue);
        FillBuffer(pContext, pBuffer, BlockSize * 0, BlockSize, F4Color_To_RGBA8Unorm({1.0f, 0.0f, 0.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 1, BlockSize, F4Color_To_RGBA8Unorm({0.0f, 1.0f, 0.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, F4Color_To_RGBA8Unorm({0.0f, 0.0f, 1.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 3, BlockSize, F4Color_To_RGBA8Unorm({1.0f, 1.0f, 0.0f, 0.0f}));
    }

    // draw full screen quad
    {
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer")->Set(pBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawFSQuad(pContext, pSwapChain);
    }

    pSwapChain->Present();
}


TEST_F(SparseMemoryTest, SparseResidentBuffer)
{
    auto*       pEnv      = TestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseMem = pDevice->GetAdapterInfo().SparseMemory;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse residency buffer is not supported by this device";
    }
    ASSERT_TRUE((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_BUFFER) != 0);

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                break;
#endif
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                break;
#endif
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                break;
#endif
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO("Sparse residency buffer test", HLSL::SparseBuffer_PS, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto BlockSize = SparseMem.SparseBlockSize;
    const auto BuffSize  = BlockSize * 8;

    auto pBuffer = CreateSparseBuffer(BuffSize, BIND_NONE, SPARSE_RESOURCE_FLAG_RESIDENT);
    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    const auto& BuffSparseProps = pBuffer->GetSparseProperties();
    ASSERT_GT(BuffSparseProps.MemoryAlignment, 0u);
    ASSERT_TRUE(IsPowerOfTwo(BuffSparseProps.MemoryAlignment));

    const auto MemBlockSize = std::max(AlignUp(BlockSize, BuffSparseProps.MemoryAlignment), BuffSparseProps.MemoryAlignment);

    auto pMemory = CreateMemory(MemBlockSize * 2, 4, pBuffer);
    ASSERT_NE(pMemory, nullptr);

    auto pFence = CreateFence();
    ASSERT_NE(pFence, nullptr);

    // bind sparse
    {
        const SparseBufferMemoryBindRange BindRanges[] = {
            {BlockSize * 0, MemBlockSize * 0, BlockSize, pMemory},
            {BlockSize * 2, MemBlockSize * 2, BlockSize, pMemory},
            {BlockSize * 3, MemBlockSize * 3, BlockSize, pMemory},
            {BlockSize * 6, MemBlockSize * 6, BlockSize, pMemory} //
        };

        SparseBufferMemoryBind SparseBuffBind;
        SparseBuffBind.pBuffer   = pBuffer;
        SparseBuffBind.NumRanges = _countof(BindRanges);
        SparseBuffBind.pRanges   = BindRanges;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        BindSparseMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumBufferBinds     = 1;
        BindSparseAttrs.pBufferBinds       = &SparseBuffBind;
        BindSparseAttrs.ppSignalFences     = &SignalFence;
        BindSparseAttrs.pSignalFenceValues = &SignalValue;
        BindSparseAttrs.NumSignalFences    = 1;

        sm_pSparseBindingCtx->BindSparseMemory(BindSparseAttrs);

        pContext->DeviceWaitForFence(SignalFence, SignalValue);
        FillBuffer(pContext, pBuffer, BlockSize * 0, BlockSize, F4Color_To_RGBA8Unorm({1.0f, 0.0f, 0.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, F4Color_To_RGBA8Unorm({0.0f, 1.0f, 0.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 3, BlockSize, F4Color_To_RGBA8Unorm({0.0f, 0.0f, 1.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 6, BlockSize, F4Color_To_RGBA8Unorm({1.0f, 1.0f, 0.0f, 0.0f}));
    }

    // draw full screen quad
    {
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer")->Set(pBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawFSQuad(pContext, pSwapChain);
    }

    pSwapChain->Present();
}


TEST_F(SparseMemoryTest, SparseResidentAliasedBuffer)
{
    auto*       pEnv      = TestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseMem = pDevice->GetAdapterInfo().SparseMemory;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_BUFFER) == 0)
    {
        GTEST_SKIP() << "Sparse residency buffer is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_ALIASED) == 0)
    {
        GTEST_SKIP() << "Sparse residency aliased resources is not supported by this device";
    }

    ASSERT_TRUE((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_BUFFER) != 0);
    ASSERT_TRUE((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_ALIASED) != 0);

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                break;
#endif
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                break;
#endif
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                break;
#endif
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO("Sparse residency aliased buffer test", HLSL::SparseBuffer_PS, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto BlockSize = SparseMem.SparseBlockSize;
    const auto BuffSize  = BlockSize * 8;

    auto pBuffer = CreateSparseBuffer(BuffSize, BIND_NONE, SPARSE_RESOURCE_FLAG_RESIDENT | SPARSE_RESOURCE_FLAG_ALIASED);
    ASSERT_NE(pBuffer, nullptr);
    ASSERT_NE(pBuffer->GetNativeHandle(), 0);

    const auto& BuffSparseProps = pBuffer->GetSparseProperties();
    ASSERT_GT(BuffSparseProps.MemoryAlignment, 0u);
    ASSERT_TRUE(IsPowerOfTwo(BuffSparseProps.MemoryAlignment));

    const auto MemBlockSize = std::max(AlignUp(BlockSize, BuffSparseProps.MemoryAlignment), BuffSparseProps.MemoryAlignment);

    auto pMemory = CreateMemory(MemBlockSize * 2, 4, pBuffer);
    ASSERT_NE(pMemory, nullptr);

    auto pFence = CreateFence();
    ASSERT_NE(pFence, nullptr);

    // bind sparse
    {
        const SparseBufferMemoryBindRange BindRanges[] = {
            {BlockSize * 0, MemBlockSize * 0, BlockSize, pMemory},
            {BlockSize * 1, MemBlockSize * 2, BlockSize, pMemory},
            {BlockSize * 2, MemBlockSize * 0, BlockSize, pMemory}, // reuse 1st memory block
            {BlockSize * 3, MemBlockSize * 1, BlockSize, pMemory},
            {BlockSize * 5, MemBlockSize * 6, BlockSize, pMemory} //
        };

        SparseBufferMemoryBind SparseBuffBind;
        SparseBuffBind.pBuffer   = pBuffer;
        SparseBuffBind.NumRanges = _countof(BindRanges);
        SparseBuffBind.pRanges   = BindRanges;

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        BindSparseMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumBufferBinds     = 1;
        BindSparseAttrs.pBufferBinds       = &SparseBuffBind;
        BindSparseAttrs.ppSignalFences     = &SignalFence;
        BindSparseAttrs.pSignalFenceValues = &SignalValue;
        BindSparseAttrs.NumSignalFences    = 1;

        sm_pSparseBindingCtx->BindSparseMemory(BindSparseAttrs);

        pContext->DeviceWaitForFence(SignalFence, SignalValue);
        FillBuffer(pContext, pBuffer, BlockSize * 2, BlockSize, F4Color_To_RGBA8Unorm({1.0f, 0.0f, 0.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 1, BlockSize, F4Color_To_RGBA8Unorm({0.0f, 1.0f, 0.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 3, BlockSize, F4Color_To_RGBA8Unorm({0.0f, 0.0f, 1.0f, 0.0f}));
        FillBuffer(pContext, pBuffer, BlockSize * 5, BlockSize, F4Color_To_RGBA8Unorm({1.0f, 1.0f, 0.0f, 0.0f}));
    }

    // draw full screen quad
    {
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Buffer")->Set(pBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawFSQuad(pContext, pSwapChain);
    }

    pSwapChain->Present();
}


TEST_P(SparseMemoryTest, SparseTexture2D)
{
    Uint32      TestId    = GetParam();
    auto*       pEnv      = TestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseMem = pDevice->GetAdapterInfo().SparseMemory;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse texture 2D is not supported by this device";
    }

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                break;
#endif
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                break;
#endif
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                break;
#endif
#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                break;
#endif
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO("Sparse texture 2D test", HLSL::SparseTexture_PS, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto BlockSize    = SparseMem.SparseBlockSize;
    const auto MemBlockSize = BlockSize * 2;
    const auto TexSize      = TestIdToTextureDim(TestId);

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, BIND_NONE, SPARSE_RESOURCE_FLAG_NONE, MemBlockSize, 8);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckTextureSparseProperties(pTexture);
    ASSERT_LE(TexSparseProps.MemorySize, BlockSize * 8);
    VERIFY_EXPR(TexSparseProps.MemorySize <= pMemory->GetCapacity());

    auto pFence = CreateFence();
    ASSERT_NE(pFence, nullptr);

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;

        Uint32 MemOffset = 0;
        for (Uint32 Mip = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
        {
            const auto Width  = std::max(1u, TexDesc.Width >> Mip);
            const auto Height = std::max(1u, TexDesc.Height >> Mip);
            for (Uint32 y = 0; y < Height; y += TexSparseProps.TileSize[1])
            {
                for (Uint32 x = 0; x < Width; x += TexSparseProps.TileSize[0])
                {
                    BindRanges.emplace_back();
                    auto& Range        = BindRanges.back();
                    Range.MipLevel     = Mip;
                    Range.ArraySlice   = 0;
                    Range.Region.MinX  = x;
                    Range.Region.MaxX  = std::min(Width, x + TexSparseProps.TileSize[0]);
                    Range.Region.MinY  = y;
                    Range.Region.MaxY  = std::min(Height, y + TexSparseProps.TileSize[1]);
                    Range.Region.MinZ  = 0;
                    Range.Region.MaxZ  = 1;
                    Range.MemoryOffset = MemOffset;
                    Range.MemorySize   = SparseMem.SparseBlockSize;
                    Range.pMemory      = pMemory;
                    MemOffset += SparseMem.SparseBlockSize;
                }
            }
        }

        // Mip tail
        {
            if (MemOffset % MemBlockSize + TexSparseProps.MipTailSize > MemBlockSize)
                MemOffset = AlignUp(MemOffset, MemBlockSize);

            BindRanges.emplace_back();
            auto& Range        = BindRanges.back();
            Range.MipLevel     = TexSparseProps.FirstMipInTail;
            Range.ArraySlice   = 0;
            Range.Region.MinX  = 0;
            Range.Region.MaxX  = std::max(1u, TexDesc.Width >> TexSparseProps.FirstMipInTail);
            Range.Region.MinY  = 0;
            Range.Region.MaxY  = std::max(1u, TexDesc.Height >> TexSparseProps.FirstMipInTail);
            Range.Region.MinZ  = 0;
            Range.Region.MaxZ  = 1;
            Range.MemoryOffset = MemOffset;
            Range.MemorySize   = TexSparseProps.MipTailSize;
            Range.pMemory      = pMemory;
            MemOffset += TexSparseProps.MipTailSize;
        }

        VERIFY_EXPR(MemOffset < pMemory->GetCapacity());

        SparseTextureMemoryBind SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        BindSparseMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds    = 1;
        BindSparseAttrs.pTextureBinds      = &SparseTexBind;
        BindSparseAttrs.ppSignalFences     = &SignalFence;
        BindSparseAttrs.pSignalFenceValues = &SignalValue;
        BindSparseAttrs.NumSignalFences    = 1;

        sm_pSparseBindingCtx->BindSparseMemory(BindSparseAttrs);

        pContext->DeviceWaitForFence(SignalFence, SignalValue);
        // clang-format off
        FillTexture(pContext, pTexture, Rect{  0,   0,       128,            128     }, 0, float4{1.0f, 0.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,            128     }, 0, float4{1.0f, 0.7f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0, 128,       128,      TexSize.y     }, 0, float4{0.9f, 1.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x,      TexSize.y     }, 0, float4{1.0f, 1.0f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 1, TexSize.y >> 1}, 1, float4{0.0f, 1.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 2, TexSize.y >> 2}, 2, float4{0.0f, 0.7f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 3, TexSize.y >> 3}, 3, float4{0.0f, 0.5f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 4, TexSize.y >> 4}, 4, float4{0.0f, 0.0f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 5, TexSize.y >> 5}, 5, float4{0.6f, 0.0f, 0.8f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 6, TexSize.y >> 6}, 6, float4{1.0f, 0.0f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 7, TexSize.y >> 7}, 7, float4{0.5f, 0.5f, 0.5f, 1.0f});
        // clang-format on
    }

    // draw full screen quad
    {
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawFSQuad(pContext, pSwapChain);
    }

    pSwapChain->Present();
}


TEST_P(SparseMemoryTest, SparseResidencyTexture2D)
{
    Uint32      TestId    = GetParam();
    auto*       pEnv      = TestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseMem = pDevice->GetAdapterInfo().SparseMemory;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse residency texture 2D is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_SHADER_RESOURCE_RESIDENCY) == 0)
    {
        GTEST_SKIP() << "Shader resource residency is not supported by this device";
    }
    ASSERT_TRUE((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_TEXTURE_2D) != 0);

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                break;
#endif
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                break;
#endif
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                break;
#endif
#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                break;
#endif
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO("Sparse resident texture 2D test", HLSL::SparseTextureResidency_PS, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto BlockSize    = SparseMem.SparseBlockSize;
    const auto MemBlockSize = BlockSize * 2;
    const auto TexSize      = TestIdToTextureDim(TestId);

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, BIND_NONE, SPARSE_RESOURCE_FLAG_RESIDENT, MemBlockSize, 8);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckTextureSparseProperties(pTexture);
    ASSERT_LE(TexSparseProps.MemorySize, BlockSize * 8);
    VERIFY_EXPR(TexSparseProps.MemorySize <= pMemory->GetCapacity());

    auto pFence = CreateFence();
    ASSERT_NE(pFence, nullptr);

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;
        Uint32                                    MemOffset = 0;
        for (Uint32 Mip = 0, Idx = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
        {
            const Uint32 Width  = std::max(1u, TexDesc.Width >> Mip);
            const Uint32 Height = std::max(1u, TexDesc.Height >> Mip);
            for (Uint32 TileY = 0; TileY < Height; TileY += TexSparseProps.TileSize[1])
            {
                for (Uint32 TileX = 0; TileX < Width; TileX += TexSparseProps.TileSize[0])
                {
                    if (++Idx & 2)
                        continue;

                    BindRanges.emplace_back();
                    auto& Range        = BindRanges.back();
                    Range.Region.MinX  = TileX;
                    Range.Region.MaxX  = std::min(TileX + TexSparseProps.TileSize[0], Width);
                    Range.Region.MinY  = TileY;
                    Range.Region.MaxY  = std::min(TileY + TexSparseProps.TileSize[1], Height);
                    Range.Region.MinZ  = 0;
                    Range.Region.MaxZ  = 1;
                    Range.MipLevel     = Mip;
                    Range.ArraySlice   = 0;
                    Range.MemoryOffset = MemOffset;
                    Range.MemorySize   = TexSparseProps.MemoryAlignment;
                    Range.pMemory      = pMemory;

                    MemOffset += TexSparseProps.MemoryAlignment;
                }
            }
        }

        // Mip tail
        {
            BindRanges.emplace_back();
            auto& Range        = BindRanges.back();
            Range.Region.MinX  = 0;
            Range.Region.MaxX  = std::max(1u, TexDesc.Width >> TexSparseProps.FirstMipInTail);
            Range.Region.MinY  = 0;
            Range.Region.MaxY  = std::max(1u, TexDesc.Height >> TexSparseProps.FirstMipInTail);
            Range.Region.MinZ  = 0;
            Range.Region.MaxZ  = 1;
            Range.MipLevel     = TexSparseProps.FirstMipInTail;
            Range.ArraySlice   = 0;
            Range.MemoryOffset = MemOffset;
            Range.MemorySize   = TexSparseProps.MipTailSize;
            Range.pMemory      = pMemory;

            MemOffset += TexSparseProps.MipTailSize;
        }

        VERIFY_EXPR(MemOffset < pMemory->GetCapacity());

        SparseTextureMemoryBind SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        BindSparseMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds    = 1;
        BindSparseAttrs.pTextureBinds      = &SparseTexBind;
        BindSparseAttrs.ppSignalFences     = &SignalFence;
        BindSparseAttrs.pSignalFenceValues = &SignalValue;
        BindSparseAttrs.NumSignalFences    = 1;

        sm_pSparseBindingCtx->BindSparseMemory(BindSparseAttrs);

        pContext->DeviceWaitForFence(SignalFence, SignalValue);
        // clang-format off
        FillTexture(pContext, pTexture, Rect{  0,   0,       128,            128     }, 0, float4{1.0f, 0.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,            128     }, 0, float4{1.0f, 0.7f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0, 128,       128,      TexSize.y     }, 0, float4{0.9f, 1.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x,      TexSize.y     }, 0, float4{1.0f, 1.0f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 1, TexSize.y >> 1}, 1, float4{0.0f, 1.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 2, TexSize.y >> 2}, 2, float4{0.0f, 0.7f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 3, TexSize.y >> 3}, 3, float4{0.0f, 0.5f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 4, TexSize.y >> 4}, 4, float4{0.0f, 0.0f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 5, TexSize.y >> 5}, 5, float4{0.6f, 0.0f, 0.8f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 6, TexSize.y >> 6}, 6, float4{1.0f, 0.0f, 1.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 7, TexSize.y >> 7}, 7, float4{0.5f, 0.5f, 0.5f, 1.0f});
        // clang-format on
    }

    // draw full screen quad
    {
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawFSQuad(pContext, pSwapChain);
    }

    pSwapChain->Present();
}


TEST_P(SparseMemoryTest, SparseResidencyAliasedTexture2D)
{
    Uint32      TestId    = GetParam();
    auto*       pEnv      = TestingEnvironment::GetInstance();
    auto*       pDevice   = pEnv->GetDevice();
    const auto& SparseMem = pDevice->GetAdapterInfo().SparseMemory;

    if (!sm_pSparseBindingCtx)
    {
        GTEST_SKIP() << "Sparse binding queue is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_TEXTURE_2D) == 0)
    {
        GTEST_SKIP() << "Sparse residency texture 2D is not supported by this device";
    }
    if ((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_RESIDENCY_ALIASED) == 0)
    {
        GTEST_SKIP() << "Sparse residency aliased resources is not supported by this device";
    }
    ASSERT_TRUE((SparseMem.CapFlags & SPARSE_MEMORY_CAP_FLAG_TEXTURE_2D) != 0);

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        auto DeviceType = pDevice->GetDeviceInfo().Type;
        switch (DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                break;
#endif
#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                break;
#endif
#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                break;
#endif
#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                break;
#endif
            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    RefCntAutoPtr<IPipelineState>         pPSO;
    RefCntAutoPtr<IShaderResourceBinding> pSRB;
    CreateGraphicsPSO("Sparse resident aliased texture 2D test", HLSL::SparseTexture_PS, pPSO, pSRB);
    ASSERT_NE(pPSO, nullptr);
    ASSERT_NE(pSRB, nullptr);

    const auto BlockSize    = SparseMem.SparseBlockSize;
    const auto MemBlockSize = BlockSize * 2;
    const auto TexSize      = TestIdToTextureDim(TestId);

    auto TexAndMem = CreateSparseTextureAndMemory(TexSize, BIND_NONE, SPARSE_RESOURCE_FLAG_RESIDENT | SPARSE_RESOURCE_FLAG_ALIASED, MemBlockSize, 8);
    auto pTexture  = TexAndMem.pTexture;
    ASSERT_NE(pTexture, nullptr);
    ASSERT_NE(pTexture->GetNativeHandle(), 0);
    auto pMemory = TexAndMem.pMemory;
    ASSERT_NE(pMemory, nullptr);

    const auto& TexDesc        = pTexture->GetDesc();
    const auto& TexSparseProps = pTexture->GetSparseProperties();
    CheckTextureSparseProperties(pTexture);
    ASSERT_LE(TexSparseProps.MemorySize, BlockSize * 8);
    VERIFY_EXPR(TexSparseProps.MemorySize <= pMemory->GetCapacity());

    auto pFence = CreateFence();
    ASSERT_NE(pFence, nullptr);

    // bind sparse
    {
        std::vector<SparseTextureMemoryBindRange> BindRanges;

        // Mip tail - must not alias with other tiles
        {
            BindRanges.emplace_back();
            auto& Range        = BindRanges.back();
            Range.Region.MinX  = 0;
            Range.Region.MaxX  = std::max(1u, TexDesc.Width >> TexSparseProps.FirstMipInTail);
            Range.Region.MinY  = 0;
            Range.Region.MaxY  = std::max(1u, TexDesc.Height >> TexSparseProps.FirstMipInTail);
            Range.Region.MinZ  = 0;
            Range.Region.MaxZ  = 1;
            Range.MipLevel     = TexSparseProps.FirstMipInTail;
            Range.ArraySlice   = 0;
            Range.MemoryOffset = 0;
            Range.MemorySize   = TexSparseProps.MipTailSize;
            Range.pMemory      = pMemory;
        }

        // tiles may alias
        Uint32 MemOffset = AlignUp(TexSparseProps.MipTailSize, TexSparseProps.MemoryAlignment);
        for (Uint32 Mip = 0, Idx = 0; Mip < TexSparseProps.FirstMipInTail; ++Mip)
        {
            const Uint32 Width  = std::max(1u, TexDesc.Width >> Mip);
            const Uint32 Height = std::max(1u, TexDesc.Height >> Mip);
            for (Uint32 TileY = 0; TileY < Height; TileY += TexSparseProps.TileSize[1])
            {
                for (Uint32 TileX = 0; TileX < Width; TileX += TexSparseProps.TileSize[0])
                {
                    if (++Idx > 3)
                    {
                        Idx = 0;
                        VERIFY_EXPR(MemOffset < pMemory->GetCapacity());
                        MemOffset = AlignUp(TexSparseProps.MipTailSize, TexSparseProps.MemoryAlignment);
                    }

                    BindRanges.emplace_back();
                    auto& Range        = BindRanges.back();
                    Range.Region.MinX  = TileX;
                    Range.Region.MaxX  = std::min(TileX + TexSparseProps.TileSize[0], Width);
                    Range.Region.MinY  = TileY;
                    Range.Region.MaxY  = std::min(TileY + TexSparseProps.TileSize[1], Height);
                    Range.Region.MinZ  = 0;
                    Range.Region.MaxZ  = 1;
                    Range.MipLevel     = Mip;
                    Range.ArraySlice   = 0;
                    Range.MemoryOffset = MemOffset;
                    Range.MemorySize   = TexSparseProps.MemoryAlignment;
                    Range.pMemory      = pMemory;

                    MemOffset += TexSparseProps.MemoryAlignment;
                }
            }
        }

        SparseTextureMemoryBind SparseTexBind;
        SparseTexBind.pTexture  = pTexture;
        SparseTexBind.NumRanges = static_cast<Uint32>(BindRanges.size());
        SparseTexBind.pRanges   = BindRanges.data();

        IFence*      SignalFence = pFence;
        const Uint64 SignalValue = 1;

        BindSparseMemoryAttribs BindSparseAttrs;
        BindSparseAttrs.NumTextureBinds    = 1;
        BindSparseAttrs.pTextureBinds      = &SparseTexBind;
        BindSparseAttrs.ppSignalFences     = &SignalFence;
        BindSparseAttrs.pSignalFenceValues = &SignalValue;
        BindSparseAttrs.NumSignalFences    = 1;

        sm_pSparseBindingCtx->BindSparseMemory(BindSparseAttrs);

        pContext->DeviceWaitForFence(SignalFence, SignalValue);
        // clang-format off
        FillTexture(pContext, pTexture, Rect{  0,   0,       128,            128     }, 0, float4{1.0f, 0.0f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{128,   0, TexSize.x,            128     }, 0, float4{1.0f, 0.7f, 0.0f, 1.0f});
        FillTexture(pContext, pTexture, Rect{  0, 128,       128,      TexSize.y     }, 0, float4{0.9f, 1.0f, 0.0f, 1.0f});
      //FillTexture(pContext, pTexture, Rect{128, 128, TexSize.x,      TexSize.y     }, 0, float4{1.0f, 1.0f, 1.0f, 1.0f}); // --|-- aliased
      //FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 1, TexSize.y >> 1}, 1, float4{0.0f, 1.0f, 0.0f, 1.0f}); // --|
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 2, TexSize.y >> 2}, 2, float4{0.0f, 0.7f, 1.0f, 1.0f}); // -|-- mip tail
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 3, TexSize.y >> 3}, 3, float4{0.0f, 0.5f, 0.0f, 1.0f}); //  |
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 4, TexSize.y >> 4}, 4, float4{0.0f, 0.0f, 1.0f, 1.0f}); //  |
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 5, TexSize.y >> 5}, 5, float4{0.6f, 0.0f, 0.8f, 1.0f}); //  |
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 6, TexSize.y >> 6}, 6, float4{1.0f, 0.0f, 1.0f, 1.0f}); //  |
        FillTexture(pContext, pTexture, Rect{  0,   0, TexSize.x >> 7, TexSize.y >> 7}, 7, float4{0.5f, 0.5f, 0.5f, 1.0f}); // -|
        // clang-format on
    }

    // draw full screen quad
    {
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawFSQuad(pContext, pSwapChain);
    }

    pSwapChain->Present();
}

INSTANTIATE_TEST_SUITE_P(Sparse, SparseMemoryTest, TestParamRange, TestIdToString);

// AZ TODO:
//  - bind and unbind
//  - 3D
//  - 2D array

} // namespace
