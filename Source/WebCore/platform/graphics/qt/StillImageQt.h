/*
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * All rights reserved.
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

#ifndef StillImageQt_h
#define StillImageQt_h

#include "Image.h"

namespace WebCore {

    class StillImage final : public Image {
    public:
        static Ref<StillImage> create(const QPixmap& pixmap)
        {
            return adoptRef(*new StillImage(pixmap));
        }

        static Ref<StillImage> createForRendering(const QPixmap* pixmap)
        {
            return adoptRef(*new StillImage(pixmap));
        }

        static Ref<StillImage> create(QPixmap&& pixmap)
        {
            return adoptRef(*new StillImage(WTFMove(pixmap)));
        }

        bool currentFrameKnownToBeOpaque() const override;

        // FIXME: StillImages are underreporting decoded sizes and will be unable
        // to prune because these functions are not implemented yet.
        void destroyDecodedData(bool destroyAll = true) override { Q_UNUSED(destroyAll); }

        FloatSize size() const override;
        NativeImagePtr nativeImageForCurrentFrame(const GraphicsContext*) override;
        ImageDrawResult draw(GraphicsContext&, const FloatRect& dstRect, const FloatRect& srcRect, CompositeOperator, BlendMode, DecodingMode, ImageOrientationDescription) override;

    private:
        StillImage(const QPixmap&);
        StillImage(const QPixmap*);
        StillImage(QPixmap&&);
        ~StillImage() override;
        
        const QPixmap* m_pixmap;
        bool m_ownsPixmap;
    };

}

#endif
