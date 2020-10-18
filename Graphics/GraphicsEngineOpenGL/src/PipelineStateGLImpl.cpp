/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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

#include "pch.h"
#include "PipelineStateGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "ShaderGLImpl.hpp"
#include "ShaderResourceBindingGLImpl.hpp"
#include "EngineMemory.h"
#include "DeviceContextGLImpl.hpp"

namespace Diligent
{


template <typename PSOCreateInfoType>
void PipelineStateGLImpl::Initialize(const PSOCreateInfoType& CreateInfo, const std::vector<GLPipelineShaderStageInfo>& ShaderStages)
{
    // Memory must be released if an exception is thrown.
    LinearAllocator MemPool{GetRawAllocator()};

    MemPool.AddSpace<GLProgramObj>(GetNumShaderStages());
    MemPool.AddSpace<GLProgramResources>(GetNumShaderStages());
    MemPool.AddSpace<SamplerPtr>(m_Desc.ResourceLayout.NumStaticSamplers);

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);

    MemPool.Reserve();

    InitResourceLayouts(ShaderStages, MemPool);
    InitializePipelineDesc(CreateInfo, MemPool);

    void* Ptr = MemPool.Release();
    VERIFY_EXPR(Ptr == m_GLPrograms);
}

PipelineStateGLImpl::PipelineStateGLImpl(IReferenceCounters*                    pRefCounters,
                                         RenderDeviceGLImpl*                    pDeviceGL,
                                         const GraphicsPipelineStateCreateInfo& CreateInfo,
                                         bool                                   bIsDeviceInternal) :
    // clang-format off
    TPipelineStateBase
    {
        pRefCounters,
        pDeviceGL,
        CreateInfo.PSODesc,
        bIsDeviceInternal
    },
    m_ResourceLayout      {*this},
    m_StaticResourceLayout{*this}
// clang-format on
{
    std::vector<GLPipelineShaderStageInfo> ShaderStages;
    ExtractShaders<ShaderGLImpl>(CreateInfo, ShaderStages);

    RefCntAutoPtr<ShaderGLImpl> pTempPS;
    if (CreateInfo.pPS == nullptr)
    {
        // Some OpenGL implementations fail if fragment shader is not present, so
        // create a dummy one.
        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_GLSL;
        ShaderCI.Source          = "void main(){}";
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "Dummy fragment shader";
        pDeviceGL->CreateShader(ShaderCI, reinterpret_cast<IShader**>(static_cast<ShaderGLImpl**>(&pTempPS)));

        ShaderStages.emplace_back(SHADER_TYPE_PIXEL, pTempPS);
        m_ShaderStageTypes[m_NumShaderStages++] = SHADER_TYPE_PIXEL;
    }

    Initialize(CreateInfo, ShaderStages);
}

PipelineStateGLImpl::PipelineStateGLImpl(IReferenceCounters*                   pRefCounters,
                                         RenderDeviceGLImpl*                   pDeviceGL,
                                         const ComputePipelineStateCreateInfo& CreateInfo,
                                         bool                                  bIsDeviceInternal) :
    // clang-format off
    TPipelineStateBase
    {
        pRefCounters,
        pDeviceGL,
        CreateInfo.PSODesc,
        bIsDeviceInternal
    },
    m_ResourceLayout      {*this},
    m_StaticResourceLayout{*this}
// clang-format on
{
    std::vector<GLPipelineShaderStageInfo> ShaderStages;
    ExtractShaders<ShaderGLImpl>(CreateInfo, ShaderStages);

    Initialize(CreateInfo, ShaderStages);
}

PipelineStateGLImpl::~PipelineStateGLImpl()
{
    auto& RawAllocator = GetRawAllocator();
    m_StaticResourceCache.Destroy(RawAllocator);
    GetDevice()->OnDestroyPSO(this);

    for (Uint32 i = 0; i < GetNumShaderStages(); ++i)
    {
        m_GLPrograms[i].~GLProgramObj();
        m_ProgramResources[i].~GLProgramResources();
    }
    for (Uint32 i = 0; i < m_Desc.ResourceLayout.NumStaticSamplers; ++i)
    {
        m_StaticSamplers[i].~SamplerPtr();
    }

    void* pRawMem = m_GLPrograms;
    RawAllocator.Free(pRawMem);
}

IMPLEMENT_QUERY_INTERFACE(PipelineStateGLImpl, IID_PipelineStateGL, TPipelineStateBase)


void PipelineStateGLImpl::InitResourceLayouts(const std::vector<GLPipelineShaderStageInfo>& ShaderStages,
                                              LinearAllocator&                              MemPool)
{
    auto* const pDeviceGL  = GetDevice();
    const auto& DeviceCaps = pDeviceGL->GetDeviceCaps();
    VERIFY(DeviceCaps.DevType != RENDER_DEVICE_TYPE_UNDEFINED, "Device caps are not initialized");

    auto pImmediateCtx = m_pDevice->GetImmediateContext();
    VERIFY_EXPR(pImmediateCtx);
    auto& GLState = pImmediateCtx.RawPtr<DeviceContextGLImpl>()->GetContextState();

    {
        m_TotalUniformBufferBindings = 0;
        m_TotalSamplerBindings       = 0;
        m_TotalImageBindings         = 0;
        m_TotalStorageBufferBindings = 0;
        if (DeviceCaps.Features.SeparablePrograms)
        {
            // Program pipelines are not shared between GL contexts, so we cannot create
            // it now
            m_ShaderResourceLayoutHash = 0;
            m_GLPrograms               = MemPool.Allocate<GLProgramObj>(ShaderStages.size());
            m_ProgramResources         = MemPool.ConstructArray<GLProgramResources>(ShaderStages.size());
            for (size_t i = 0; i < ShaderStages.size(); ++i)
            {
                auto*       pShaderGL  = ShaderStages[i].pShader;
                const auto& ShaderDesc = pShaderGL->GetDesc();
                new (m_GLPrograms + i) GLProgramObj{ShaderGLImpl::LinkProgram(&pShaderGL, 1, true)};
                // Load uniforms and assign bindings
                m_ProgramResources[i].LoadUniforms(ShaderDesc.ShaderType, m_GLPrograms[i], GLState,
                                                   m_TotalUniformBufferBindings,
                                                   m_TotalSamplerBindings,
                                                   m_TotalImageBindings,
                                                   m_TotalStorageBufferBindings);

                HashCombine(m_ShaderResourceLayoutHash, m_ProgramResources[i].GetHash());
            }
        }
        else
        {
            std::vector<ShaderGLImpl*> Shaders;

            SHADER_TYPE ActiveStages = SHADER_TYPE_UNKNOWN;
            for (const auto& Stage : ShaderStages)
            {
                Shaders.push_back(Stage.pShader);
                VERIFY((ActiveStages & Stage.Type) == 0, "Shader stage ", GetShaderTypeLiteralName(Stage.Type), " is already active");
                ActiveStages |= Stage.Type;
            }

            m_GLPrograms       = MemPool.Construct<GLProgramObj>(ShaderGLImpl::LinkProgram(Shaders.data(), static_cast<Uint32>(ShaderStages.size()), false));
            m_ProgramResources = MemPool.Construct<GLProgramResources>();

            m_ProgramResources[0].LoadUniforms(ActiveStages, m_GLPrograms[0], GLState,
                                               m_TotalUniformBufferBindings,
                                               m_TotalSamplerBindings,
                                               m_TotalImageBindings,
                                               m_TotalStorageBufferBindings);

            m_ShaderResourceLayoutHash = m_ProgramResources[0].GetHash();
        }

        // Initialize master resource layout that keeps all variable types and does not reference a resource cache
        m_ResourceLayout.Initialize(m_ProgramResources, static_cast<Uint32>(ShaderStages.size()), m_Desc.PipelineType, m_Desc.ResourceLayout, nullptr, 0, nullptr);
    }

    m_StaticSamplers = MemPool.ConstructArray<SamplerPtr>(m_Desc.ResourceLayout.NumStaticSamplers);
    for (Uint32 s = 0; s < m_Desc.ResourceLayout.NumStaticSamplers; ++s)
    {
        pDeviceGL->CreateSampler(m_Desc.ResourceLayout.StaticSamplers[s].Desc, &m_StaticSamplers[s]);
    }

    {
        // Clone only static variables into static resource layout, assign and initialize static resource cache
        const SHADER_RESOURCE_VARIABLE_TYPE StaticVars[] = {SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
        m_StaticResourceLayout.Initialize(m_ProgramResources, static_cast<Uint32>(ShaderStages.size()), m_Desc.PipelineType, m_Desc.ResourceLayout, StaticVars, _countof(StaticVars), &m_StaticResourceCache);
        InitStaticSamplersInResourceCache(m_StaticResourceLayout, m_StaticResourceCache);
    }
}

void PipelineStateGLImpl::CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
{
    auto* pRenderDeviceGL = GetDevice();
    auto& SRBAllocator    = pRenderDeviceGL->GetSRBAllocator();
    auto  pResBinding     = NEW_RC_OBJ(SRBAllocator, "ShaderResourceBindingGLImpl instance", ShaderResourceBindingGLImpl)(this, m_ProgramResources, GetNumShaderStages());
    if (InitStaticResources)
        pResBinding->InitializeStaticResources(this);
    pResBinding->QueryInterface(IID_ShaderResourceBinding, reinterpret_cast<IObject**>(ppShaderResourceBinding));
}

bool PipelineStateGLImpl::IsCompatibleWith(const IPipelineState* pPSO) const
{
    VERIFY_EXPR(pPSO != nullptr);

    if (pPSO == this)
        return true;

    const PipelineStateGLImpl* pPSOGL = ValidatedCast<const PipelineStateGLImpl>(pPSO);
    if (m_ShaderResourceLayoutHash != pPSOGL->m_ShaderResourceLayoutHash)
        return false;

    if (GetNumShaderStages() != pPSOGL->GetNumShaderStages())
        return false;

    for (size_t i = 0; i < GetNumShaderStages(); ++i)
    {
        if (!m_ProgramResources[i].IsCompatibleWith(pPSOGL->m_ProgramResources[i]))
            return false;
    }

    return true;
}

void PipelineStateGLImpl::CommitProgram(GLContextState& State)
{
    auto ProgramPipelineSupported = m_pDevice->GetDeviceCaps().Features.SeparablePrograms;

    if (ProgramPipelineSupported)
    {
        // WARNING: glUseProgram() overrides glBindProgramPipeline(). That is, if you have a program in use and
        // a program pipeline bound, all rendering will use the program that is in use, not the pipeline programs!
        // So make sure that glUseProgram(0) has been called if pipeline is in use
        State.SetProgram(GLObjectWrappers::GLProgramObj::Null());
        auto& Pipeline = GetGLProgramPipeline(State.GetCurrentGLContext());
        VERIFY(Pipeline != 0, "Program pipeline must not be null");
        State.SetPipeline(Pipeline);
    }
    else
    {
        VERIFY_EXPR(m_GLPrograms != nullptr);
        State.SetProgram(m_GLPrograms[0]);
    }
}

GLObjectWrappers::GLPipelineObj& PipelineStateGLImpl::GetGLProgramPipeline(GLContext::NativeGLContextType Context)
{
    ThreadingTools::LockHelper Lock(m_ProgPipelineLockFlag);
    for (auto& ctx_pipeline : m_GLProgPipelines)
    {
        if (ctx_pipeline.first == Context)
            return ctx_pipeline.second;
    }

    // Create new progam pipeline
    m_GLProgPipelines.emplace_back(Context, true);
    auto&  ctx_pipeline = m_GLProgPipelines.back();
    GLuint Pipeline     = ctx_pipeline.second;
    for (Uint32 i = 0; i < GetNumShaderStages(); ++i)
    {
        auto GLShaderBit = ShaderTypeToGLShaderBit(GetShaderStageType(i));
        // If the program has an active code for each stage mentioned in set flags,
        // then that code will be used by the pipeline. If program is 0, then the given
        // stages are cleared from the pipeline.
        glUseProgramStages(Pipeline, GLShaderBit, m_GLPrograms[i]);
        CHECK_GL_ERROR("glUseProgramStages() failed");
    }
    return ctx_pipeline.second;
}

void PipelineStateGLImpl::InitializeSRBResourceCache(GLProgramResourceCache& ResourceCache) const
{
    ResourceCache.Initialize(m_TotalUniformBufferBindings, m_TotalSamplerBindings, m_TotalImageBindings, m_TotalStorageBufferBindings, GetRawAllocator());
    InitStaticSamplersInResourceCache(m_ResourceLayout, ResourceCache);
}

void PipelineStateGLImpl::InitStaticSamplersInResourceCache(const GLPipelineResourceLayout& ResourceLayout, GLProgramResourceCache& Cache) const
{
    for (Uint32 s = 0; s < ResourceLayout.GetNumResources<GLPipelineResourceLayout::SamplerBindInfo>(); ++s)
    {
        const auto& Sam = ResourceLayout.GetConstResource<GLPipelineResourceLayout::SamplerBindInfo>(s);
        if (Sam.m_StaticSamplerIdx >= 0)
        {
            ISampler* pSampler = m_StaticSamplers[Sam.m_StaticSamplerIdx].RawPtr<ISampler>();
            for (Uint32 binding = Sam.m_Attribs.Binding; binding < Sam.m_Attribs.Binding + Sam.m_Attribs.ArraySize; ++binding)
                Cache.SetStaticSampler(binding, pSampler);
        }
    }
}

void PipelineStateGLImpl::BindStaticResources(Uint32 ShaderFlags, IResourceMapping* pResourceMapping, Uint32 Flags)
{
    m_StaticResourceLayout.BindResources(static_cast<SHADER_TYPE>(ShaderFlags), pResourceMapping, Flags, m_StaticResourceCache);
}

Uint32 PipelineStateGLImpl::GetStaticVariableCount(SHADER_TYPE ShaderType) const
{
    if (!IsConsistentShaderType(ShaderType, m_Desc.PipelineType))
    {
        LOG_WARNING_MESSAGE("Unable to get the number of static variables in shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " as the stage is invalid for ", GetPipelineTypeString(m_Desc.PipelineType), " pipeline '", m_Desc.Name, "'");
        return 0;
    }

    return m_StaticResourceLayout.GetNumVariables(ShaderType);
}

IShaderResourceVariable* PipelineStateGLImpl::GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name)
{
    if (!IsConsistentShaderType(ShaderType, m_Desc.PipelineType))
    {
        LOG_WARNING_MESSAGE("Unable to find static variable '", Name, "' in shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " as the stage is invalid for ", GetPipelineTypeString(m_Desc.PipelineType), " pipeline '", m_Desc.Name, "'");
        return nullptr;
    }

    return m_StaticResourceLayout.GetShaderVariable(ShaderType, Name);
}

IShaderResourceVariable* PipelineStateGLImpl::GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    if (!IsConsistentShaderType(ShaderType, m_Desc.PipelineType))
    {
        LOG_WARNING_MESSAGE("Unable to get static variable at index ", Index, " in shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " as the stage is invalid for ", GetPipelineTypeString(m_Desc.PipelineType), " pipeline '", m_Desc.Name, "'");
        return nullptr;
    }

    return m_StaticResourceLayout.GetShaderVariable(ShaderType, Index);
}

} // namespace Diligent
