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

#include "WHLSLExpression.h"
#include <wtf/FastMalloc.h>

namespace WebCore {

namespace WHLSL {

namespace AST {

class BooleanLiteral : public Expression {
    WTF_MAKE_FAST_ALLOCATED;
public:
    BooleanLiteral(CodeLocation location, bool value)
        : Expression(location)
        , m_value(value)
    {
    }

    virtual ~BooleanLiteral() = default;

    BooleanLiteral(const BooleanLiteral&) = delete;
    BooleanLiteral(BooleanLiteral&&) = default;

    BooleanLiteral& operator=(const BooleanLiteral&) = delete;
    BooleanLiteral& operator=(BooleanLiteral&&) = default;

    bool value() const { return m_value; }

    bool isBooleanLiteral() const override { return true; }

    BooleanLiteral clone() const
    {
        BooleanLiteral result(codeLocation(), m_value);
        copyTypeTo(result);
        return result;
    }

private:
    bool m_value;
};

} // namespace AST

}

}

SPECIALIZE_TYPE_TRAITS_WHLSL_EXPRESSION(BooleanLiteral, isBooleanLiteral())

#endif
