/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TextureMapperImageBuffer.h"

#include "BitmapTexturePool.h"
#include "GraphicsLayer.h"
#include "NotImplemented.h"

#if USE(TEXTURE_MAPPER)
namespace WebCore {

static const int s_maximumAllowedImageBufferDimension = 4096;

TextureMapperImageBuffer::TextureMapperImageBuffer()
    : TextureMapper(SoftwareMode)
{
    m_texturePool = std::make_unique<BitmapTexturePool>();
}

IntSize TextureMapperImageBuffer::maxTextureSize() const
{
    return IntSize(s_maximumAllowedImageBufferDimension, s_maximumAllowedImageBufferDimension);
}

void TextureMapperImageBuffer::beginClip(const TransformationMatrix& matrix, const FloatRect& rect)
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;
#if ENABLE(3D_TRANSFORMS)
    TransformationMatrix previousTransform = context->get3DTransform();
#else
    AffineTransform previousTransform = context->getCTM();
#endif
    context->save();

#if ENABLE(3D_TRANSFORMS)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif

    context->clip(rect);

#if ENABLE(3D_TRANSFORMS)
    context->set3DTransform(previousTransform);
#else
    context->setCTM(previousTransform);
#endif
}

void TextureMapperImageBuffer::endClip()
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    context->restore();
}

void TextureMapperImageBuffer::drawTexture(const BitmapTexture& texture, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity, unsigned /* exposedEdges */)
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    const BitmapTextureImageBuffer& textureImageBuffer = static_cast<const BitmapTextureImageBuffer&>(texture);
    ImageBuffer* image = textureImageBuffer.image();
    context->save();
    context->setCompositeOperation(isInMaskMode() ? CompositeDestinationIn : CompositeSourceOver);
    context->setAlpha(opacity);
#if ENABLE(3D_TRANSFORMS)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif
    context->drawImageBuffer(*image, targetRect);
    context->restore();
}

void TextureMapperImageBuffer::drawSolidColor(const FloatRect& rect, const TransformationMatrix& matrix, const Color& color)
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    context->save();
    context->setCompositeOperation(isInMaskMode() ? CompositeDestinationIn : CompositeSourceOver);
#if ENABLE(3D_TRANSFORMS)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif

    context->fillRect(rect, color);
    context->restore();
}

void TextureMapperImageBuffer::drawBorder(const Color& color, float borderWidth , const FloatRect& rect, const TransformationMatrix& matrix)
{
#if PLATFORM(QT)
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    context->save();
    context->setCompositeOperation(isInMaskMode() ? CompositeDestinationIn : CompositeSourceOver);
#if ENABLE(3D_TRANSFORMS)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif

    QPainter& painter = *context->platformContext();
    painter.setBrush(Qt::NoBrush);
    QPen newPen(color);
    newPen.setWidthF(borderWidth);
    painter.setPen(newPen);
    painter.drawRect(rect);

    context->restore();
#endif
}

void TextureMapperImageBuffer::drawNumber(int number, const Color& color, const FloatPoint& targetPoint, const TransformationMatrix& matrix)
{
#if PLATFORM(QT)
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    context->save();
    context->setCompositeOperation(isInMaskMode() ? CompositeDestinationIn : CompositeSourceOver);
#if ENABLE(3D_TRANSFORMS)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif

    // Partially duplicates TextureMapperGL::drawNumber
    int pointSize = 8;
    QString counterString = QString::number(number);

    QFont font(QString::fromLatin1("Monospace"), pointSize, QFont::Bold);
    font.setStyleHint(QFont::TypeWriter);

    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(counterString) + 4;
    int height = fontMetrics.height();

    IntSize size(width, height);
    IntRect sourceRect(IntPoint::zero(), size);

    QPainter& painter = *context->platformContext();
    painter.translate(targetPoint);
    painter.fillRect(sourceRect, color);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(2, height * 0.85, counterString);

    context->restore();
#endif
}

RefPtr<BitmapTexture> BitmapTextureImageBuffer::applyFilters(TextureMapper&, const FilterOperations& filters)
{
    ASSERT_UNUSED(filters, filters.isEmpty());

    return this;
}

}
#endif
