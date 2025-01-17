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
/// Declaration of Diligent::Texture3D_D3D11 class

#include "TextureBaseD3D11.hpp"

namespace Diligent
{

/// Implementation of a 3D texture in Direct3D11 backend.
class Texture3D_D3D11 final : public TextureBaseD3D11
{
public:
    Texture3D_D3D11(IReferenceCounters*          pRefCounters,
                    FixedBlockMemoryAllocator&   TexViewObjAllocator,
                    class RenderDeviceD3D11Impl* pDeviceD3D11,
                    const TextureDesc&           TexDesc,
                    const TextureData*           pInitData = nullptr);

    Texture3D_D3D11(IReferenceCounters*          pRefCounters,
                    FixedBlockMemoryAllocator&   TexViewObjAllocator,
                    class RenderDeviceD3D11Impl* pDeviceD3D11,
                    RESOURCE_STATE               InitialState,
                    ID3D11Texture3D*             pd3d11Texture);
    ~Texture3D_D3D11();

protected:
    // clang-format off
    virtual void CreateSRV(const TextureViewDesc& SRVDesc, ID3D11ShaderResourceView**  ppD3D11SRV) override final;
    virtual void CreateRTV(const TextureViewDesc& RTVDesc, ID3D11RenderTargetView**    ppD3D11RTV) override final;
    virtual void CreateDSV(const TextureViewDesc& DSVDesc, ID3D11DepthStencilView**    ppD3D11DSV) override final;
    virtual void CreateUAV(const TextureViewDesc& UAVDesc, ID3D11UnorderedAccessView** ppD3D11UAV) override final;
    // clang-format on
};

} // namespace Diligent
