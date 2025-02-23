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

#include "config.h"
#include "WHLSLNativeFunctionWriter.h"

#if ENABLE(WEBGPU)

#include "NotImplemented.h"
#include "WHLSLAddressSpace.h"
#include "WHLSLArrayType.h"
#include "WHLSLEnumerationDefinition.h"
#include "WHLSLInferTypes.h"
#include "WHLSLIntrinsics.h"
#include "WHLSLNamedType.h"
#include "WHLSLNativeFunctionDeclaration.h"
#include "WHLSLNativeTypeDeclaration.h"
#include "WHLSLPointerType.h"
#include "WHLSLStructureDefinition.h"
#include "WHLSLTypeDefinition.h"
#include "WHLSLTypeNamer.h"
#include "WHLSLUnnamedType.h"
#include "WHLSLVariableDeclaration.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

namespace WHLSL {

namespace Metal {

static String mapFunctionName(String& functionName)
{
    if (functionName == "ddx")
        return "dfdx"_str;
    if (functionName == "ddy")
        return "dfdy"_str;
    if (functionName == "asint")
        return "as_type<int32_t>"_str;
    if (functionName == "asuint")
        return "as_type<uint32_t>"_str;
    if (functionName == "asfloat")
        return "as_type<float>"_str;
    return functionName;
}

static String atomicName(String input)
{
    if (input == "Add")
        return "fetch_add"_str;
    if (input == "And")
        return "fetch_and"_str;
    if (input == "Exchange")
        return "exchange"_str;
    if (input == "Max")
        return "fetch_max"_str;
    if (input == "Min")
        return "fetch_min"_str;
    if (input == "Or")
        return "fetch_or"_str;
    ASSERT(input == "Xor");
        return "fetch_xor"_str;
}

static int vectorLength(AST::NativeTypeDeclaration& nativeTypeDeclaration)
{
    int vectorLength = 1;
    if (!nativeTypeDeclaration.typeArguments().isEmpty()) {
        ASSERT(nativeTypeDeclaration.typeArguments().size() == 2);
        ASSERT(WTF::holds_alternative<AST::ConstantExpression>(nativeTypeDeclaration.typeArguments()[1]));
        vectorLength = WTF::get<AST::ConstantExpression>(nativeTypeDeclaration.typeArguments()[1]).integerLiteral().value();
    }
    return vectorLength;
}

static AST::NamedType& vectorInnerType(AST::NativeTypeDeclaration& nativeTypeDeclaration)
{
    if (nativeTypeDeclaration.typeArguments().isEmpty())
        return nativeTypeDeclaration;

    ASSERT(nativeTypeDeclaration.typeArguments().size() == 2);
    ASSERT(WTF::holds_alternative<Ref<AST::TypeReference>>(nativeTypeDeclaration.typeArguments()[0]));
    return WTF::get<Ref<AST::TypeReference>>(nativeTypeDeclaration.typeArguments()[0])->resolvedType();
}

static const char* vectorSuffix(int vectorLength)
{
    switch (vectorLength) {
    case 1:
        return "";
    case 2:
        return "2";
    case 3:
        return "3";
    default:
        ASSERT(vectorLength == 4);
        return "4";
    }
}

String writeNativeFunction(AST::NativeFunctionDeclaration& nativeFunctionDeclaration, String& outputFunctionName, Intrinsics& intrinsics, TypeNamer& typeNamer)
{
    StringBuilder stringBuilder;
    if (nativeFunctionDeclaration.isCast()) {
        auto& returnType = nativeFunctionDeclaration.type();
        auto metalReturnName = typeNamer.mangledNameForType(returnType);
        if (!nativeFunctionDeclaration.parameters().size()) {
            stringBuilder.flexibleAppend(
                metalReturnName, ' ', outputFunctionName, "() {\n"
                "    ", metalReturnName, " x = { };\n"
                "    return x;\n"
                "}\n"
            );
            return stringBuilder.toString();
        }

        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto& parameterType = *nativeFunctionDeclaration.parameters()[0]->type();
        auto metalParameterName = typeNamer.mangledNameForType(parameterType);
        stringBuilder.flexibleAppend(metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " x) {\n");

        {
            auto isEnumerationDefinition = [] (auto& type) {
                return is<AST::NamedType>(type) && is<AST::EnumerationDefinition>(downcast<AST::NamedType>(type));
            };
            auto& unifiedReturnType = returnType.unifyNode();
            if (isEnumerationDefinition(unifiedReturnType) && !isEnumerationDefinition(parameterType.unifyNode())) { 
                auto& enumerationDefinition = downcast<AST::EnumerationDefinition>(downcast<AST::NamedType>(unifiedReturnType));
                stringBuilder.append("    switch (x) {\n");
                bool hasZeroCase = false;
                for (auto& member : enumerationDefinition.enumerationMembers()) {
                    hasZeroCase |= !member.get().value();
                    stringBuilder.flexibleAppend("        case ", member.get().value(), ": break;\n");
                }
                ASSERT_UNUSED(hasZeroCase, hasZeroCase);
                stringBuilder.append("        default: x = 0; break; }\n");
            }
        }

        stringBuilder.flexibleAppend(
            "    return static_cast<", metalReturnName, ">(x);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "operator.value") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " x) {\n"
            "    return static_cast<", metalReturnName, ">(x);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=198077 Authors can make a struct field named "length" too. Autogenerated getters for those shouldn't take this codepath.
    if (nativeFunctionDeclaration.name() == "operator.length") {
        ASSERT_UNUSED(intrinsics, matches(nativeFunctionDeclaration.type(), intrinsics.uintType()));
        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto& parameterType = nativeFunctionDeclaration.parameters()[0]->type()->unifyNode();
        auto& unnamedParameterType = downcast<AST::UnnamedType>(parameterType);
        if (is<AST::ArrayType>(unnamedParameterType)) {
            auto& arrayParameterType = downcast<AST::ArrayType>(unnamedParameterType);
            stringBuilder.flexibleAppend(
                "uint ", outputFunctionName, '(', metalParameterName, ") {\n"
                "    return ", arrayParameterType.numElements(), ";\n"
                "}\n"
            );
            return stringBuilder.toString();
        }

        ASSERT(is<AST::ArrayReferenceType>(unnamedParameterType));
        stringBuilder.flexibleAppend(
            "uint ", outputFunctionName, '(', metalParameterName, " v) {\n"
            "    return v.length;\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name().startsWith("operator."_str)) {
        auto mangledFieldName = [&](const String& fieldName) -> String {
            auto& unifyNode = nativeFunctionDeclaration.parameters()[0]->type()->unifyNode();
            auto& namedType = downcast<AST::NamedType>(unifyNode);
            if (is<AST::StructureDefinition>(namedType)) {
                auto& structureDefinition = downcast<AST::StructureDefinition>(namedType);
                auto* structureElement = structureDefinition.find(fieldName);
                ASSERT(structureElement);
                return typeNamer.mangledNameForStructureElement(*structureElement);
            }
            ASSERT(is<AST::NativeTypeDeclaration>(namedType));
            return fieldName;
        };

        if (nativeFunctionDeclaration.name().endsWith("=")) {
            ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
            auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
            auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
            auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
            auto fieldName = nativeFunctionDeclaration.name().substring("operator."_str.length());
            fieldName = fieldName.substring(0, fieldName.length() - 1);
            auto metalFieldName = mangledFieldName(fieldName);
            stringBuilder.flexibleAppend(
                metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " v, ", metalParameter2Name, " n) {\n"
                "    v.", metalFieldName, " = n;\n"
                "    return v;\n"
                "}\n"
            );
            return stringBuilder.toString();
        }

        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto fieldName = nativeFunctionDeclaration.name().substring("operator."_str.length());
        auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        auto metalFieldName = mangledFieldName(fieldName);
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " v) {\n"
            "    return v.", metalFieldName, ";\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name().startsWith("operator&."_str)) {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        auto fieldName = nativeFunctionDeclaration.name().substring("operator&."_str.length());

        String metalFieldName;
        auto& unnamedType = *nativeFunctionDeclaration.parameters()[0]->type();
        auto& unifyNode = downcast<AST::PointerType>(unnamedType).elementType().unifyNode();
        auto& namedType = downcast<AST::NamedType>(unifyNode);
        if (is<AST::StructureDefinition>(namedType)) {
            auto& structureDefinition = downcast<AST::StructureDefinition>(namedType);
            auto* structureElement = structureDefinition.find(fieldName);
            ASSERT(structureElement);
            metalFieldName = typeNamer.mangledNameForStructureElement(*structureElement);
        } else
            metalFieldName = fieldName;

        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " v) {\n"
            "    return &(v->", metalFieldName, ");\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "operator&[]") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
        auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        ASSERT(is<AST::ArrayReferenceType>(*nativeFunctionDeclaration.parameters()[0]->type()));
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " v, ", metalParameter2Name, " n) {\n"
            "    if (n < v.length) return &(v.pointer[n]);\n"
            "    return nullptr;\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    auto matrixDimension = [&] (unsigned typeArgumentIndex) -> unsigned {
        auto& typeReference = downcast<AST::TypeReference>(*nativeFunctionDeclaration.parameters()[0]->type());
        auto& matrixType = downcast<AST::NativeTypeDeclaration>(downcast<AST::TypeReference>(downcast<AST::TypeDefinition>(typeReference.resolvedType()).type()).resolvedType());
        ASSERT(matrixType.name() == "matrix");
        ASSERT(matrixType.typeArguments().size() == 3);
        return WTF::get<AST::ConstantExpression>(matrixType.typeArguments()[typeArgumentIndex]).integerLiteral().value();
    };
    auto numberOfMatrixRows = [&] {
        return matrixDimension(1);
    };
    auto numberOfMatrixColumns = [&] {
        return matrixDimension(2);
    };

    if (nativeFunctionDeclaration.name() == "operator[]") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
        auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());

        unsigned numberOfRows = numberOfMatrixRows();
        unsigned numberOfColumns = numberOfMatrixColumns();

        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " m, ", metalParameter2Name, " i) {\n"
            "    if (i >= ", numberOfRows, ") return ", metalReturnName, "(0);\n"
            "    ", metalReturnName, " result;\n"
            "    result[0] = m[i];\n"
            "    result[1] = m[i + ", numberOfRows, "];\n"
        );
        if (numberOfColumns >= 3)
            stringBuilder.flexibleAppend("    result[2] = m[i + ", numberOfRows * 2, "];\n");
        if (numberOfColumns >= 4)
            stringBuilder.flexibleAppend("    result[3] = m[i + ", numberOfRows * 3, "];\n");
        stringBuilder.append(
            "    return result;\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "operator[]=") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 3);
        auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto metalParameter3Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[2]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());

        unsigned numberOfRows = numberOfMatrixRows();
        unsigned numberOfColumns = numberOfMatrixColumns();

        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " m, ", metalParameter2Name, " i, ", metalParameter3Name, " v) {\n"
            "    if (i >= ", numberOfRows, ") return m;\n"
            "    m[i] = v[0];\n"
            "    m[i + ", numberOfRows, "] = v[1];\n"
        );
        if (numberOfColumns >= 3)
            stringBuilder.flexibleAppend("    m[i + ", numberOfRows * 2, "] = v[2];\n");
        if (numberOfColumns >= 4)
            stringBuilder.flexibleAppend("    m[i + ", numberOfRows * 3, "] = v[3];\n");
        stringBuilder.append(
            "    return m;"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.isOperator()) {
        if (nativeFunctionDeclaration.parameters().size() == 1) {
            auto operatorName = nativeFunctionDeclaration.name().substring("operator"_str.length());
            auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
            auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
            stringBuilder.flexibleAppend(
                metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " x) {\n"
                "    return ", operatorName, "x;\n"
                "}\n"
            );
            return stringBuilder.toString();
        }

        ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
        auto operatorName = nativeFunctionDeclaration.name().substring("operator"_str.length());
        auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " x, ", metalParameter2Name, " y) {\n"
            "    return x ", operatorName, " y;\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "cos"
        || nativeFunctionDeclaration.name() == "sin"
        || nativeFunctionDeclaration.name() == "tan"
        || nativeFunctionDeclaration.name() == "acos"
        || nativeFunctionDeclaration.name() == "asin"
        || nativeFunctionDeclaration.name() == "atan"
        || nativeFunctionDeclaration.name() == "cosh"
        || nativeFunctionDeclaration.name() == "sinh"
        || nativeFunctionDeclaration.name() == "tanh"
        || nativeFunctionDeclaration.name() == "ceil"
        || nativeFunctionDeclaration.name() == "exp"
        || nativeFunctionDeclaration.name() == "floor"
        || nativeFunctionDeclaration.name() == "log"
        || nativeFunctionDeclaration.name() == "round"
        || nativeFunctionDeclaration.name() == "trunc"
        || nativeFunctionDeclaration.name() == "ddx"
        || nativeFunctionDeclaration.name() == "ddy"
        || nativeFunctionDeclaration.name() == "isnormal"
        || nativeFunctionDeclaration.name() == "isfinite"
        || nativeFunctionDeclaration.name() == "isinf"
        || nativeFunctionDeclaration.name() == "isnan"
        || nativeFunctionDeclaration.name() == "asint"
        || nativeFunctionDeclaration.name() == "asuint"
        || nativeFunctionDeclaration.name() == "asfloat") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " x) {\n"
            "    return ", mapFunctionName(nativeFunctionDeclaration.name()), "(x);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "pow" || nativeFunctionDeclaration.name() == "atan2") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
        auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " x, ", metalParameter2Name, " y) {\n"
            "    return ", nativeFunctionDeclaration.name(), "(x, y);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "AllMemoryBarrierWithGroupSync") {
        ASSERT(!nativeFunctionDeclaration.parameters().size());
        stringBuilder.flexibleAppend(
            "void ", outputFunctionName, "() {\n"
            "    threadgroup_barrier(mem_flags::mem_device);\n"
            "    threadgroup_barrier(mem_flags::mem_threadgroup);\n"
            "    threadgroup_barrier(mem_flags::mem_texture);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "DeviceMemoryBarrierWithGroupSync") {
        ASSERT(!nativeFunctionDeclaration.parameters().size());
        stringBuilder.flexibleAppend(
            "void ", outputFunctionName, "() {\n"
            "    threadgroup_barrier(mem_flags::mem_device);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "GroupMemoryBarrierWithGroupSync") {
        ASSERT(!nativeFunctionDeclaration.parameters().size());
        stringBuilder.flexibleAppend(
            "void ", outputFunctionName, "() {\n"
            "    threadgroup_barrier(mem_flags::mem_threadgroup);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name().startsWith("Interlocked"_str)) {
        if (nativeFunctionDeclaration.name() == "InterlockedCompareExchange") {
            ASSERT(nativeFunctionDeclaration.parameters().size() == 4);
            auto& firstArgumentPointer = downcast<AST::PointerType>(*nativeFunctionDeclaration.parameters()[0]->type());
            auto firstArgumentAddressSpace = firstArgumentPointer.addressSpace();
            auto firstArgumentPointee = typeNamer.mangledNameForType(firstArgumentPointer.elementType());
            auto secondArgument = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
            auto thirdArgument = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[2]->type());
            auto& fourthArgumentPointer = downcast<AST::PointerType>(*nativeFunctionDeclaration.parameters()[3]->type());
            auto fourthArgumentAddressSpace = fourthArgumentPointer.addressSpace();
            auto fourthArgumentPointee = typeNamer.mangledNameForType(fourthArgumentPointer.elementType());
            stringBuilder.flexibleAppend(
                "void ", outputFunctionName, '(', toString(firstArgumentAddressSpace), ' ', firstArgumentPointee, "* object, ", secondArgument, " compare, ", thirdArgument, " desired, ", toString(fourthArgumentAddressSpace), ' ', fourthArgumentPointee, "* out) {\n"
                "    atomic_compare_exchange_weak_explicit(object, &compare, desired, memory_order_relaxed, memory_order_relaxed);\n"
                "    *out = compare;\n"
                "}\n"
            );
            return stringBuilder.toString();
        }

        ASSERT(nativeFunctionDeclaration.parameters().size() == 3);
        auto& firstArgumentPointer = downcast<AST::PointerType>(*nativeFunctionDeclaration.parameters()[0]->type());
        auto firstArgumentAddressSpace = firstArgumentPointer.addressSpace();
        auto firstArgumentPointee = typeNamer.mangledNameForType(firstArgumentPointer.elementType());
        auto secondArgument = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto& thirdArgumentPointer = downcast<AST::PointerType>(*nativeFunctionDeclaration.parameters()[2]->type());
        auto thirdArgumentAddressSpace = thirdArgumentPointer.addressSpace();
        auto thirdArgumentPointee = typeNamer.mangledNameForType(thirdArgumentPointer.elementType());
        auto name = atomicName(nativeFunctionDeclaration.name().substring("Interlocked"_str.length()));
        stringBuilder.flexibleAppend(
            "void ", outputFunctionName, '(', toString(firstArgumentAddressSpace), ' ', firstArgumentPointee, "* object, ", secondArgument, " operand, ", toString(thirdArgumentAddressSpace), ' ', thirdArgumentPointee, "* out) {\n"
            "    *out = atomic_", name, "_explicit(object, operand, memory_order_relaxed);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "Sample") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 3 || nativeFunctionDeclaration.parameters().size() == 4);
        
        auto& textureType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[0]->type()->unifyNode()));
        auto& locationType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[2]->type()->unifyNode()));
        auto locationVectorLength = vectorLength(locationType);
        auto& returnType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.type().unifyNode()));
        auto returnVectorLength = vectorLength(returnType);

        auto metalParameter1Name = typeNamer.mangledNameForType(textureType);
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        auto metalParameter3Name = typeNamer.mangledNameForType(locationType);
        String metalParameter4Name;
        if (nativeFunctionDeclaration.parameters().size() == 4)
            metalParameter4Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[3]->type());
        auto metalReturnName = typeNamer.mangledNameForType(returnType);
        stringBuilder.flexibleAppend(metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " theTexture, ", metalParameter2Name, " theSampler, ", metalParameter3Name, " location");
        if (!metalParameter4Name.isNull())
            stringBuilder.flexibleAppend(", ", metalParameter4Name, " offset");
        stringBuilder.append(
            ") {\n"
            "    return theTexture.sample(theSampler, "
        );
        if (textureType.isTextureArray()) {
            ASSERT(locationVectorLength > 1);
            stringBuilder.flexibleAppend("location.", "xyzw"_str.substring(0, locationVectorLength - 1), ", location.", "xyzw"_str.substring(locationVectorLength - 1, 1));
        } else
            stringBuilder.append("location");
        if (!metalParameter4Name.isNull())
            stringBuilder.append(", offset");
        stringBuilder.append(")");
        if (!textureType.isDepthTexture())
            stringBuilder.flexibleAppend(".", "xyzw"_str.substring(0, returnVectorLength));
        stringBuilder.append(
            ";\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "Load") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
        
        auto& textureType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[0]->type()->unifyNode()));
        auto& locationType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[1]->type()->unifyNode()));
        auto locationVectorLength = vectorLength(locationType);
        auto& returnType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.type().unifyNode()));
        auto returnVectorLength = vectorLength(returnType);

        auto metalParameter1Name = typeNamer.mangledNameForType(textureType);
        auto metalParameter2Name = typeNamer.mangledNameForType(locationType);
        auto metalReturnName = typeNamer.mangledNameForType(returnType);
        stringBuilder.flexibleAppend(metalReturnName, ' ', outputFunctionName, '(', metalParameter1Name, " theTexture, ", metalParameter2Name, " location) {\n");
        if (textureType.isTextureArray()) {
            ASSERT(locationVectorLength > 1);
            String dimensions[] = { "width"_str, "height"_str, "depth"_str };
            for (int i = 0; i < locationVectorLength - 1; ++i) {
                auto suffix = "xyzw"_str.substring(i, 1);
                stringBuilder.flexibleAppend("    if (location.", suffix, " < 0 || static_cast<uint32_t>(location.", suffix, ") >= theTexture.get_", dimensions[i], "()) return ", metalReturnName, "(0);\n");
            }
            auto suffix = "xyzw"_str.substring(locationVectorLength - 1, 1);
            stringBuilder.flexibleAppend("    if (location.", suffix, " < 0 || static_cast<uint32_t>(location.", suffix, ") >= theTexture.get_array_size()) return ", metalReturnName, "(0);\n");
        } else {
            if (locationVectorLength == 1)
                stringBuilder.flexibleAppend("    if (location < 0 || static_cast<uint32_t>(location) >= theTexture.get_width()) return ", metalReturnName, "(0);\n");
            else {
                stringBuilder.flexibleAppend(
                    "    if (location.x < 0 || static_cast<uint32_t>(location.x) >= theTexture.get_width()) return ", metalReturnName, "(0);\n"
                    "    if (location.y < 0 || static_cast<uint32_t>(location.y) >= theTexture.get_height()) return ", metalReturnName, "(0);\n"
                );
                if (locationVectorLength >= 3)
                    stringBuilder.flexibleAppend("    if (location.z < 0 || static_cast<uint32_t>(location.z) >= theTexture.get_depth()) return ", metalReturnName, "(0);\n");
            }
        }
        stringBuilder.append("    return theTexture.read(");
        if (textureType.isTextureArray()) {
            ASSERT(locationVectorLength > 1);
            stringBuilder.flexibleAppend("uint", vectorSuffix(locationVectorLength - 1), "(location.", "xyzw"_str.substring(0, locationVectorLength - 1), "), uint(location.", "xyzw"_str.substring(locationVectorLength - 1, 1), ')');
        } else
            stringBuilder.flexibleAppend("uint", vectorSuffix(locationVectorLength), "(location)");
        stringBuilder.append(')');
        if (!textureType.isDepthTexture())
            stringBuilder.flexibleAppend('.', "xyzw"_str.substring(0, returnVectorLength));
        stringBuilder.append(
            ";\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "load") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 1);
        auto metalParameterName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalReturnName = typeNamer.mangledNameForType(nativeFunctionDeclaration.type());
        stringBuilder.flexibleAppend(
            metalReturnName, ' ', outputFunctionName, '(', metalParameterName, " x) {\n"
            "    return atomic_load_explicit(x, memory_order_relaxed);\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "store") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 2);
        auto metalParameter1Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[0]->type());
        auto metalParameter2Name = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[1]->type());
        stringBuilder.flexibleAppend("void ", outputFunctionName, '(', metalParameter1Name, " x, ", metalParameter2Name, " y) {\n"
            "    atomic_store_explicit(x, y, memory_order_relaxed);\n"
            "}\n");
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "GetDimensions") {
        auto& textureType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[0]->type()->unifyNode()));

        size_t index = 1;
        if (!textureType.isWritableTexture() && textureType.textureDimension() != 1)
            ++index;
        auto widthTypeName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[index]->type());
        ++index;
        String heightTypeName;
        if (textureType.textureDimension() >= 2) {
            heightTypeName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[index]->type());
            ++index;
        }
        String depthTypeName;
        if (textureType.textureDimension() >= 3) {
            depthTypeName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[index]->type());
            ++index;
        }
        String elementsTypeName;
        if (textureType.isTextureArray()) {
            elementsTypeName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[index]->type());
            ++index;
        }
        String numberOfLevelsTypeName;
        if (!textureType.isWritableTexture() && textureType.textureDimension() != 1) {
            numberOfLevelsTypeName = typeNamer.mangledNameForType(*nativeFunctionDeclaration.parameters()[index]->type());
            ++index;
        }
        ASSERT(index == nativeFunctionDeclaration.parameters().size());

        auto metalParameter1Name = typeNamer.mangledNameForType(textureType);
        stringBuilder.flexibleAppend("void ", outputFunctionName, '(', metalParameter1Name, " theTexture");
        if (!textureType.isWritableTexture() && textureType.textureDimension() != 1)
            stringBuilder.append(", uint mipLevel");
        stringBuilder.flexibleAppend(", ", widthTypeName, " width");
        if (!heightTypeName.isNull())
            stringBuilder.flexibleAppend(", ", heightTypeName, " height");
        if (!depthTypeName.isNull())
            stringBuilder.flexibleAppend(", ", depthTypeName, " depth");
        if (!elementsTypeName.isNull())
            stringBuilder.flexibleAppend(", ", elementsTypeName, " elements");
        if (!numberOfLevelsTypeName.isNull())
            stringBuilder.flexibleAppend(", ", numberOfLevelsTypeName, " numberOfLevels");
        stringBuilder.append(
            ") {\n"
            "    if (width)\n"
            "        *width = theTexture.get_width("
        );
        if (!textureType.isWritableTexture() && textureType.textureDimension() != 1)
            stringBuilder.append("mipLevel");
        stringBuilder.append(");\n");
        if (!heightTypeName.isNull()) {
            stringBuilder.append(
                "    if (height)\n"
                "        *height = theTexture.get_height("
            );
            if (!textureType.isWritableTexture() && textureType.textureDimension() != 1)
                stringBuilder.append("mipLevel");
            stringBuilder.append(");\n");
        }
        if (!depthTypeName.isNull()) {
            stringBuilder.append(
                "    if (depth)\n"
                "        *depth = theTexture.get_depth("
            );
            if (!textureType.isWritableTexture() && textureType.textureDimension() != 1)
                stringBuilder.append("mipLevel");
            stringBuilder.append(");\n");
        }
        if (!elementsTypeName.isNull()) {
            stringBuilder.append(
                "    if (elements)\n"
                "        *elements = theTexture.get_array_size();\n"
            );
        }
        if (!numberOfLevelsTypeName.isNull()) {
            stringBuilder.append(
                "    if (numberOfLevels)\n"
                "        *numberOfLevels = theTexture.get_num_mip_levels();\n"
            );
        }
        stringBuilder.append("}\n");
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "SampleBias") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "SampleGrad") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "SampleLevel") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "Gather") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "GatherRed") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "SampleCmp") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "SampleCmpLevelZero") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "Store") {
        ASSERT(nativeFunctionDeclaration.parameters().size() == 3);
        
        auto& textureType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[0]->type()->unifyNode()));
        auto& itemType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[1]->type()->unifyNode()));
        auto& itemVectorInnerType = vectorInnerType(itemType);
        auto itemVectorLength = vectorLength(itemType);
        auto& locationType = downcast<AST::NativeTypeDeclaration>(downcast<AST::NamedType>(nativeFunctionDeclaration.parameters()[2]->type()->unifyNode()));
        auto locationVectorLength = vectorLength(locationType);

        auto metalParameter1Name = typeNamer.mangledNameForType(textureType);
        auto metalParameter2Name = typeNamer.mangledNameForType(itemType);
        auto metalParameter3Name = typeNamer.mangledNameForType(locationType);
        auto metalInnerTypeName = typeNamer.mangledNameForType(itemVectorInnerType);
        stringBuilder.flexibleAppend("void ", outputFunctionName, '(', metalParameter1Name, " theTexture, ", metalParameter2Name, " item, ", metalParameter3Name, " location) {\n");
        if (textureType.isTextureArray()) {
            ASSERT(locationVectorLength > 1);
            String dimensions[] = { "width"_str, "height"_str, "depth"_str };
            for (int i = 0; i < locationVectorLength - 1; ++i) {
                auto suffix = "xyzw"_str.substring(i, 1);
                stringBuilder.flexibleAppend("    if (location.", suffix, " >= theTexture.get_", dimensions[i], "()) return;\n");
            }
            auto suffix = "xyzw"_str.substring(locationVectorLength - 1, 1);
            stringBuilder.flexibleAppend("    if (location.", suffix, " >= theTexture.get_array_size()) return;\n");
        } else {
            if (locationVectorLength == 1)
                stringBuilder.append("    if (location >= theTexture.get_width()) return;\n");
            else {
                stringBuilder.append(
                    "    if (location.x >= theTexture.get_width()) return;\n"
                    "    if (location.y >= theTexture.get_height()) return;\n"
                );
                if (locationVectorLength >= 3)
                    stringBuilder.append("    if (location.z >= theTexture.get_depth()) return;\n");
            }
        }
        stringBuilder.flexibleAppend("    theTexture.write(vec<", metalInnerTypeName, ", 4>(item");
        for (int i = 0; i < 4 - itemVectorLength; ++i)
            stringBuilder.append(", 0");
        stringBuilder.append("), ");
        if (textureType.isTextureArray()) {
            ASSERT(locationVectorLength > 1);
            stringBuilder.flexibleAppend("uint", vectorSuffix(locationVectorLength - 1), "(location.", "xyzw"_str.substring(0, locationVectorLength - 1), "), uint(location.", "xyzw"_str.substring(locationVectorLength - 1, 1), ')');
        } else
            stringBuilder.flexibleAppend("uint", vectorSuffix(locationVectorLength), "(location)");
        stringBuilder.append(
            ");\n"
            "}\n"
        );
        return stringBuilder.toString();
    }

    if (nativeFunctionDeclaration.name() == "GatherAlpha") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "GatherBlue") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "GatherCmp") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "GatherCmpRed") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    if (nativeFunctionDeclaration.name() == "GatherGreen") {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=195813 Implement this
        notImplemented();
    }

    ASSERT_NOT_REACHED();
    return String();
}

} // namespace Metal

} // namespace WHLSL

} // namespace WebCore

#endif
