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
#include "WHLSLFunctionDeclaration.h"
#include <wtf/FastMalloc.h>
#include <wtf/UniqueRef.h>

namespace WebCore {

namespace WHLSL {

namespace AST {

class PropertyAccessExpression : public Expression {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PropertyAccessExpression(CodeLocation location, UniqueRef<Expression>&& base)
        : Expression(location)
        , m_base(WTFMove(base))
    {
    }

    virtual ~PropertyAccessExpression() = default;

    PropertyAccessExpression(const PropertyAccessExpression&) = delete;
    PropertyAccessExpression(PropertyAccessExpression&&) = default;

    bool isPropertyAccessExpression() const override { return true; }

    virtual String getterFunctionName() const = 0;
    virtual String setterFunctionName() const = 0;
    virtual String anderFunctionName() const = 0;

    FunctionDeclaration* getterFunction() { return m_getterFunction; }
    FunctionDeclaration* anderFunction() { return m_anderFunction; }
    FunctionDeclaration* threadAnderFunction() { return m_threadAnderFunction; }
    FunctionDeclaration* setterFunction() { return m_setterFunction; }

    void setGetterFunction(FunctionDeclaration* getterFunction)
    {
        m_getterFunction = getterFunction;
    }

    void setAnderFunction(FunctionDeclaration* anderFunction)
    {
        m_anderFunction = anderFunction;
    }

    void setThreadAnderFunction(FunctionDeclaration* threadAnderFunction)
    {
        m_threadAnderFunction = threadAnderFunction;
    }

    void setSetterFunction(FunctionDeclaration* setterFunction)
    {
        m_setterFunction = setterFunction;
    }

    Expression& base() { return m_base; }
    UniqueRef<Expression>& baseReference() { return m_base; }
    UniqueRef<Expression> takeBase() { return WTFMove(m_base); }

private:
    UniqueRef<Expression> m_base;
    FunctionDeclaration* m_getterFunction { nullptr };
    FunctionDeclaration* m_anderFunction { nullptr };
    FunctionDeclaration* m_threadAnderFunction { nullptr };
    FunctionDeclaration* m_setterFunction { nullptr };
};

} // namespace AST

}

}

SPECIALIZE_TYPE_TRAITS_WHLSL_EXPRESSION(PropertyAccessExpression, isPropertyAccessExpression())

#endif
