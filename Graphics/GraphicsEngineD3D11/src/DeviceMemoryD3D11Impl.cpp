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

#include "pch.h"

#include "DeviceMemoryD3D11Impl.hpp"

#include "RenderDeviceD3D11Impl.hpp"
#include "DeviceContextD3D11Impl.hpp"

#include "D3D11TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "EngineMemory.h"

namespace Diligent
{

DeviceMemoryD3D11Impl::DeviceMemoryD3D11Impl(IReferenceCounters*           pRefCounters,
                                             RenderDeviceD3D11Impl*        pRenderDeviceD3D11,
                                             const DeviceMemoryCreateInfo& MemCI) :
    TDeviceMemoryBase{pRefCounters, pRenderDeviceD3D11, MemCI.Desc}
{
    D3D11_BUFFER_DESC D3D11BuffDesc{};
    D3D11BuffDesc.ByteWidth = 0;
    D3D11BuffDesc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;
    D3D11BuffDesc.Usage     = D3D11_USAGE_DEFAULT;

    auto* pDeviceD3D11 = pRenderDeviceD3D11->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateBuffer(&D3D11BuffDesc, nullptr, &m_pd3d11Buffer),
                           "Failed to create the Direct3D11 tile pool");
}

DeviceMemoryD3D11Impl::~DeviceMemoryD3D11Impl()
{
}

IMPLEMENT_QUERY_INTERFACE(DeviceMemoryD3D11Impl, IID_DeviceMemoryD3D11, TDeviceMemoryBase)

Bool DeviceMemoryD3D11Impl::Resize(Uint64 NewSize)
{
    DvpVerifyResize(NewSize);

    // AZ TODO: use ID3D11DeviceContext2::ResizeTilePool
    return false;
}

Uint64 DeviceMemoryD3D11Impl::GetCapacity()
{
    return 0;
}

Bool DeviceMemoryD3D11Impl::IsCompatible(IDeviceObject* pResource) const
{
    return true;
}

} // namespace Diligent
