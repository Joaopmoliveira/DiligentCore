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


#include <Unknwn.h>
#include <guiddef.h>
#include <atlbase.h>
#include <atlcom.h>

#include "dxc/dxcapi.h"

#include "DXCompiler.hpp"

namespace Diligent
{

namespace
{

class DXCompilerBase : public IDXCompiler
{
public:
    ~DXCompilerBase() override
    {
        if (Module)
            FreeLibrary(Module);
    }

protected:
    DxcCreateInstanceProc Load(DXCompilerTarget, const String& LibName)
    {
        if (LibName.size())
        {
            std::wstring wname{LibName.begin(), LibName.end()};
            wname += L".dll";

            Module = LoadPackagedLibrary(wname.c_str(), 0);
        }

        if (Module == nullptr)
            Module = LoadPackagedLibrary(L"dxcompiler.dll", 0);

        return Module ? reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(Module, "DxcCreateInstance")) : nullptr;
    }

private:
    HMODULE Module = nullptr;
};

} // namespace

} // namespace Diligent
