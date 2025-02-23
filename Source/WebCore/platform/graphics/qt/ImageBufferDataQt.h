/*
 * Copyright (C) 2008 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ImageBufferDataQt_h
#define ImageBufferDataQt_h

#include "Image.h"

#include "PlatformLayer.h"

#include <QImage>
#include <QPaintDevice>
#include <QPainter>

#if ENABLE(ACCELERATED_2D_CANVAS)
#include <QOpenGLContext>
#endif

#include <wtf/RefPtr.h>

namespace WebCore {

class IntSize;

struct ImageBufferDataPrivate {
    virtual ~ImageBufferDataPrivate() { }
    virtual QPaintDevice* paintDevice() = 0;
    virtual QImage toQImage() const = 0;
    virtual RefPtr<Image> image() const = 0;
    virtual RefPtr<Image> copyImage() const = 0;
    virtual RefPtr<Image> takeImage() = 0;
    virtual bool isAccelerated() const = 0;
    virtual PlatformLayer* platformLayer() = 0;
    virtual void draw(GraphicsContext& destContext, const FloatRect& destRect,
        const FloatRect& srcRect, CompositeOperator, BlendMode,
        bool ownContext) = 0;
    virtual void drawPattern(GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, const AffineTransform& patternTransform,
        const FloatPoint& phase, const FloatSize& spacing, CompositeOperator,
        BlendMode, bool ownContext) = 0;
    virtual void clip(GraphicsContext&, const IntRect& floatRect) const = 0;
    virtual void platformTransformColorSpace(const std::array<uint8_t, 256>& lookUpTable) = 0;
};

class ImageBufferData {
public:
    ImageBufferData(const FloatSize&, float resolutionScale);
#if ENABLE(ACCELERATED_2D_CANVAS)
    ImageBufferData(const FloatSize&, QOpenGLContext*);
#endif
    ~ImageBufferData();
    QPainter* m_painter;
    std::unique_ptr<GraphicsContext> m_context;
    ImageBufferDataPrivate* m_impl;
protected:
    void initPainter();
};

} // namespace WebCore

#endif
