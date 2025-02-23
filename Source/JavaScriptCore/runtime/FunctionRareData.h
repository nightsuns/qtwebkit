/*
 * Copyright (C) 2015-2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "InternalFunctionAllocationProfile.h"
#include "JSCast.h"
#include "ObjectAllocationProfile.h"
#include "PackedCellPtr.h"
#include "Watchpoint.h"

namespace JSC {

class JSGlobalObject;
class LLIntOffsetsExtractor;
namespace DFG {
class SpeculativeJIT;
class JITCompiler;
}

class FunctionRareData final : public JSCell {
    friend class JIT;
    friend class DFG::SpeculativeJIT;
    friend class DFG::JITCompiler;
    friend class VM;
    
public:
    typedef JSCell Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static FunctionRareData* create(VM&);

    static const bool needsDestruction = true;
    static void destroy(JSCell*);

    static Structure* createStructure(VM&, JSGlobalObject*, JSValue prototype);

    static void visitChildren(JSCell*, SlotVisitor&);

    DECLARE_INFO;

    static inline ptrdiff_t offsetOfObjectAllocationProfile() { return OBJECT_OFFSETOF(FunctionRareData, m_objectAllocationProfile); }
    static inline ptrdiff_t offsetOfObjectAllocationProfileWatchpoint() { return OBJECT_OFFSETOF(FunctionRareData, m_objectAllocationProfileWatchpoint); }
    static inline ptrdiff_t offsetOfInternalFunctionAllocationProfile() { return OBJECT_OFFSETOF(FunctionRareData, m_internalFunctionAllocationProfile); }
    static inline ptrdiff_t offsetOfBoundFunctionStructure() { return OBJECT_OFFSETOF(FunctionRareData, m_boundFunctionStructure); }
    static inline ptrdiff_t offsetOfAllocationProfileClearingWatchpoint() { return OBJECT_OFFSETOF(FunctionRareData, m_allocationProfileClearingWatchpoint); }
    static inline ptrdiff_t offsetOfHasReifiedLength() { return OBJECT_OFFSETOF(FunctionRareData, m_hasReifiedLength); }
    static inline ptrdiff_t offsetOfHasReifiedName() { return OBJECT_OFFSETOF(FunctionRareData, m_hasReifiedName); }

    ObjectAllocationProfileWithPrototype* objectAllocationProfile()
    {
        return &m_objectAllocationProfile;
    }

    Structure* objectAllocationStructure() { return m_objectAllocationProfile.structure(); }
    JSObject* objectAllocationPrototype() { return m_objectAllocationProfile.prototype(); }

    InlineWatchpointSet& allocationProfileWatchpointSet()
    {
        return m_objectAllocationProfileWatchpoint;
    }

    void clear(const char* reason);

    void initializeObjectAllocationProfile(VM&, JSGlobalObject*, JSObject* prototype, size_t inlineCapacity, JSFunction* constructor);

    bool isObjectAllocationProfileInitialized() { return !m_objectAllocationProfile.isNull(); }

    Structure* internalFunctionAllocationStructure() { return m_internalFunctionAllocationProfile.structure(); }
    Structure* createInternalFunctionAllocationStructureFromBase(VM& vm, JSGlobalObject* globalObject, JSObject* prototype, Structure* baseStructure)
    {
        return m_internalFunctionAllocationProfile.createAllocationStructureFromBase(vm, globalObject, this, prototype, baseStructure);
    }
    void clearInternalFunctionAllocationProfile()
    {
        m_internalFunctionAllocationProfile.clear();
    }

    Structure* getBoundFunctionStructure() { return m_boundFunctionStructure.get(); }
    void setBoundFunctionStructure(VM& vm, Structure* structure) { m_boundFunctionStructure.set(vm, this, structure); }

    bool hasReifiedLength() const { return m_hasReifiedLength; }
    void setHasReifiedLength() { m_hasReifiedLength = true; }
    bool hasReifiedName() const { return m_hasReifiedName; }
    void setHasReifiedName() { m_hasReifiedName = true; }

    bool hasAllocationProfileClearingWatchpoint() const { return !!m_allocationProfileClearingWatchpoint; }
    Watchpoint* createAllocationProfileClearingWatchpoint();
    class AllocationProfileClearingWatchpoint;

protected:
    FunctionRareData(VM&);
    ~FunctionRareData();

private:
    friend class LLIntOffsetsExtractor;

    // Ideally, there would only be one allocation profile for subclassing but due to Reflect.construct we
    // have two. There are some pros and cons in comparison to our current system to using the same profile
    // for both JS constructors and subclasses of builtin constructors:
    //
    // 1) + Uses less memory.
    // 2) + Conceptually simplier as there is only one profile.
    // 3) - We would need a check in all JSFunction object creations (both with classes and without) that the
    //      new.target's profiled structure has a JSFinalObject ClassInfo. This is needed, for example, if we have
    //      `Reflect.construct(Array, args, myConstructor)` since myConstructor will be the new.target of Array
    //      the Array constructor will set the allocation profile of myConstructor to hold an Array structure
    //
    // We don't really care about 1) since this memory is rare and small in total. 2) is unfortunate but is
    // probably outweighed by the cost of 3).
    ObjectAllocationProfileWithPrototype m_objectAllocationProfile;
    InlineWatchpointSet m_objectAllocationProfileWatchpoint;
    InternalFunctionAllocationProfile m_internalFunctionAllocationProfile;
    WriteBarrier<Structure> m_boundFunctionStructure;
    std::unique_ptr<AllocationProfileClearingWatchpoint> m_allocationProfileClearingWatchpoint;
    bool m_hasReifiedLength { false };
    bool m_hasReifiedName { false };
};

class FunctionRareData::AllocationProfileClearingWatchpoint final : public Watchpoint {
public:
    AllocationProfileClearingWatchpoint(FunctionRareData* rareData)
        : Watchpoint(Watchpoint::Type::FunctionRareDataAllocationProfileClearing)
        , m_rareData(rareData)
    { }

    void fireInternal(VM&, const FireDetail&);

private:
    // Own destructor may not be called. Keep members trivially destructible.
    JSC_WATCHPOINT_FIELD(PackedCellPtr<FunctionRareData>, m_rareData);
};

inline Watchpoint* FunctionRareData::createAllocationProfileClearingWatchpoint()
{
    RELEASE_ASSERT(!hasAllocationProfileClearingWatchpoint());
    m_allocationProfileClearingWatchpoint = std::make_unique<AllocationProfileClearingWatchpoint>(this);
    return m_allocationProfileClearingWatchpoint.get();
}

} // namespace JSC
