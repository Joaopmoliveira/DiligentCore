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
/// Declaration of Diligent::CommandListVkImpl class

#include "EngineVkImplTraits.hpp"
#include "VulkanUtilities/VulkanHeaders.h"
#include "CommandListBase.hpp"

namespace Diligent
{

/// Command list implementation in Vulkan backend.
class CommandListVkImpl final : public CommandListBase<EngineVkImplTraits>
{
public:
    using TCommandListBase = CommandListBase<EngineVkImplTraits>;

    CommandListVkImpl(IReferenceCounters*  pRefCounters,
                      RenderDeviceVkImpl*  pDevice,
                      DeviceContextVkImpl* pDeferredCtx,
                      VkCommandBuffer      vkCmdBuff) :
        // clang-format off
        TCommandListBase {pRefCounters, pDevice, pDeferredCtx},
        m_pDeferredCtx   {pDeferredCtx},
        m_vkCmdBuff      {vkCmdBuff   }
    // clang-format on
    {
    }

    ~CommandListVkImpl()
    {
        VERIFY(m_vkCmdBuff == VK_NULL_HANDLE && !m_pDeferredCtx, "Destroying command list that was never executed");
    }

    void Close(RefCntAutoPtr<IDeviceContext>& outDeferredCtx, VkCommandBuffer& outVkCmdBuff)
    {
        outVkCmdBuff   = m_vkCmdBuff;
        outDeferredCtx = std::move(m_pDeferredCtx);
        m_vkCmdBuff    = VK_NULL_HANDLE;
    }

private:
    RefCntAutoPtr<IDeviceContext> m_pDeferredCtx;
    VkCommandBuffer               m_vkCmdBuff;
};

} // namespace Diligent
