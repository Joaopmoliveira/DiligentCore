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

#pragma once

/// \file
/// Definition of the Diligent::IDeviceMemory interface

#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {815F7AE1-84A8-4ADD-A93B-3E28C1711D5E}
static const INTERFACE_ID IID_DeviceMemory =
    {0x815f7ae1, 0x84a8, 0x4add, {0xa9, 0x3b, 0x3e, 0x28, 0xc1, 0x71, 0x1d, 0x5e}};

// clang-format off

DILIGENT_TYPED_ENUM(DEVICE_MEMORY_TYPE, Uint8)
{
    /// AZ TODO
    DEVICE_MEMORY_TYPE_NONE   = 0,

    /// AZ TODO
    DEVICE_MEMORY_TYPE_SPARSE = 1,
};

/// Device memory description
struct DeviceMemoryDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// AZ TODO
    DEVICE_MEMORY_TYPE  Type                  DEFAULT_INITIALIZER(DEVICE_MEMORY_TYPE_NONE);

    /// AZ TODO
    Uint32              PageSize              DEFAULT_INITIALIZER(0);
    
    /// Defines which immediate contexts are allowed to execute commands that use this device memory.

    /// When ImmediateContextMask contains a bit at position n, the device memory may be
    /// used in the immediate context with index n directly (see DeviceContextDesc::ContextId).
    /// It may also be used in a command list recorded by a deferred context that will be executed
    /// through that immediate context.
    ///
    /// \remarks    Only specify these bits that will indicate those immediate contexts where the device memory
    ///             will actually be used. Do not set unnecessary bits as this will result in extra overhead.
    Uint64 ImmediateContextMask     DEFAULT_INITIALIZER(1);

#if DILIGENT_CPP_INTERFACE
    constexpr DeviceMemoryDesc() noexcept {}

    constexpr DeviceMemoryDesc(DEVICE_MEMORY_TYPE _Type,
                               Uint32             _PageSize,
                               Uint64             _ImmediateContextMask  = DeviceMemoryDesc{}.ImmediateContextMask) noexcept :
        Type                {_Type                 },
        PageSize            {_PageSize             },
        ImmediateContextMask{_ImmediateContextMask }
    {}
#endif
};
typedef struct DeviceMemoryDesc DeviceMemoryDesc;

/// AZ TODO
struct DeviceMemoryCreateInfo
{
    /// AZ TODO
    DeviceMemoryDesc  Desc;
    
    /// AZ TODO
    IDeviceObject**   ppCompatibleResources DEFAULT_INITIALIZER(nullptr);
    
    /// AZ TODO
    Uint32            NumResources          DEFAULT_INITIALIZER(0);
};
typedef struct DeviceMemoryCreateInfo DeviceMemoryCreateInfo;

// clang-format on

#define DILIGENT_INTERFACE_NAME IDeviceMemory
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IDeviceMemoryInclusiveMethods \
    IDeviceObjectInclusiveMethods;    \
    IDeviceMemoryMethods DeviceMemory

// clang-format off

/// Device memory interface

/// Defines the methods to manipulate a device memory object
DILIGENT_BEGIN_INTERFACE(IDeviceMemory, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the device memory description used to create the object
    virtual const DeviceMemoryDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// AZ TODO
    VIRTUAL Bool METHOD(Resize)(THIS_
                                Uint64 NewSize) PURE;

    /// AZ TODO
    VIRTUAL Uint64 METHOD(GetCapacity)(THIS) PURE;

    /// AZ TODO
    VIRTUAL Bool METHOD(IsCompatible)(THIS_
                                      IDeviceObject* pResource) CONST PURE;

    // AZ TODO:
    //VIRTUAL void METHOD(DbgOnResourceDestroyed)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IDeviceMemory_GetDesc(This) (const struct DeviceMemoryDesc*)IDeviceObject_GetDesc(This)

#    define IDeviceMemory_Resize(This, ...)        CALL_IFACE_METHOD(DeviceMemory, Resize,       This, __VA_ARGS__)
#    define IDeviceMemory_GetCapacity(This)        CALL_IFACE_METHOD(DeviceMemory, GetCapacity,  This)
#    define IDeviceMemory_IsCompatible(This, ...)  CALL_IFACE_METHOD(DeviceMemory, IsCompatible, This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
