/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEBGPU)

#include "WHLSLCodeLocation.h"
#include "WHLSLNamedType.h"
#include "WHLSLStructureElement.h"
#include <wtf/FastMalloc.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

namespace WHLSL {

namespace AST {

class StructureDefinition : public NamedType {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StructureDefinition(CodeLocation location, String&& name, StructureElements&& structureElements)
        : NamedType(location, WTFMove(name))
        , m_structureElements(WTFMove(structureElements))
    {
    }

    virtual ~StructureDefinition() = default;

    StructureDefinition(const StructureDefinition&) = delete;
    StructureDefinition(StructureDefinition&&) = default;

    bool isStructureDefinition() const override { return true; }

    StructureElements& structureElements() { return m_structureElements; }
    StructureElement* find(const String& name)
    {
        auto iterator = std::find_if(m_structureElements.begin(), m_structureElements.end(), [&](StructureElement& structureElement) -> bool {
            return structureElement.name() == name;
        });
        if (iterator == m_structureElements.end())
            return nullptr;
        return &*iterator;
    }

private:
    StructureElements m_structureElements;
};

} // namespace AST

}

}

SPECIALIZE_TYPE_TRAITS_WHLSL_NAMED_TYPE(StructureDefinition, isStructureDefinition())

#endif
