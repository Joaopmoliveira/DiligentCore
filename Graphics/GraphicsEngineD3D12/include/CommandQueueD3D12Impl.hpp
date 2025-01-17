/*
 *  Copyright 2019-2021 Diligent Graphics LLC
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

#pragma once

/// \file
/// Declaration of Diligent::CommandQueueD3D12Impl class

#include <mutex>
#include <atomic>

#include "CommandQueueD3D12.h"
#include "ObjectBase.hpp"

namespace Diligent
{

/// Implementation of the Diligent::ICommandQueueD3D12 interface
class CommandQueueD3D12Impl final : public ObjectBase<ICommandQueueD3D12>
{
public:
    using TBase = ObjectBase<ICommandQueueD3D12>;

    CommandQueueD3D12Impl(IReferenceCounters* pRefCounters, ID3D12CommandQueue* pd3d12NativeCmdQueue, ID3D12Fence* pd3d12Fence);
    ~CommandQueueD3D12Impl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_CommandQueueD3D12, TBase)

    // Implementation of ICommandQueue::GetNextFenceValue().
    virtual Uint64 DILIGENT_CALL_TYPE GetNextFenceValue() const override final { return m_NextFenceValue.load(); }

    // Implementation of ICommandQueue::WaitForIdle().
    virtual Uint64 DILIGENT_CALL_TYPE WaitForIdle() override final;

    // Implementation of ICommandQueue::GetCompletedFenceValue().
    virtual Uint64 DILIGENT_CALL_TYPE GetCompletedFenceValue() override final;

    // Implementation of ICommandQueueD3D12::Submit().
    virtual Uint64 DILIGENT_CALL_TYPE Submit(Uint32                    NumCommandLists,
                                             ID3D12CommandList* const* ppCommandLists) override final;

    // Implementation of ICommandQueueD3D12::GetD3D12CommandQueue().
    virtual ID3D12CommandQueue* DILIGENT_CALL_TYPE GetD3D12CommandQueue() override final { return m_pd3d12CmdQueue; }

    // Implementation of ICommandQueueD3D12::EnqueueSignal().
    virtual void DILIGENT_CALL_TYPE EnqueueSignal(ID3D12Fence* pFence, Uint64 Value) override final;

    // Implementation of ICommandQueueD3D12::WaitFence().
    virtual void DILIGENT_CALL_TYPE WaitFence(ID3D12Fence* pFence, Uint64 Value) override final;

    // Implementation of ICommandQueueD3D12::GetD3D12CommandQueueDesc().
    virtual const D3D12_COMMAND_QUEUE_DESC& DILIGENT_CALL_TYPE GetD3D12CommandQueueDesc(THIS) const override final
    {
        return m_d3d12CmdQueueDesc;
    }

private:
    // A value that will be signaled by the command queue next
    std::atomic<Uint64> m_NextFenceValue{1};

    // Last fence value completed by the GPU
    std::atomic<Uint64> m_LastCompletedFenceValue{0};

    std::mutex                  m_QueueMtx;
    CComPtr<ID3D12CommandQueue> m_pd3d12CmdQueue;

    const D3D12_COMMAND_QUEUE_DESC m_d3d12CmdQueueDesc;

    // The fence is signaled right after the command list has been
    // submitted to the command queue for execution.
    // All command lists with fence value less or equal to the signaled value
    // are guaranteed to be finished by the GPU
    CComPtr<ID3D12Fence> m_d3d12Fence;

    HANDLE m_WaitForGPUEventHandle = {};
};

} // namespace Diligent
