/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Charles Samuels <charles@kde.org>
 * Copyright (C) 2008, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 University of Szeged
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

#include "config.h"
#include "Cursor.h"

#include "Image.h"
#include "IntPoint.h"

#include "NotImplemented.h"

#include <stdio.h>
#include <stdlib.h>

#undef CopyCursor

namespace WebCore {

Cursor::Cursor(const Cursor& other)
    : m_type(other.m_type)
    , m_image(other.m_image)
    , m_hotSpot(other.m_hotSpot)
#ifndef QT_NO_CURSOR
    , m_platformCursor(other.m_platformCursor ? std::make_unique<QCursor>(*other.m_platformCursor)  : nullptr)
#endif
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_type = other.m_type;
    m_image = other.m_image;
    m_hotSpot = other.m_hotSpot;
#ifndef QT_NO_CURSOR
    m_platformCursor = other.m_platformCursor ? std::make_unique<QCursor>(*other.m_platformCursor) : nullptr;
#endif
    return *this;
}

#ifndef QT_NO_CURSOR
static std::unique_ptr<QCursor> createCustomCursor(Image* image, const IntPoint& hotSpot)
{
    if (!image->nativeImageForCurrentFrame())
        return nullptr;
    IntPoint effectiveHotSpot = determineHotSpot(image, hotSpot);
    return std::make_unique<QCursor>(*(image->nativeImageForCurrentFrame()), effectiveHotSpot.x(), effectiveHotSpot.y());
}
#endif

void Cursor::ensurePlatformCursor() const
{
#ifndef QT_NO_CURSOR
    if (m_platformCursor)
        return;

    switch (m_type) {
    case Pointer:
        m_platformCursor = std::make_unique<QCursor>(Qt::ArrowCursor);
        break;
    case Cross:
        m_platformCursor = std::make_unique<QCursor>(Qt::CrossCursor);
        break;
    case Hand:
        m_platformCursor = std::make_unique<QCursor>(Qt::PointingHandCursor);
        break;
    case IBeam:
        m_platformCursor = std::make_unique<QCursor>(Qt::IBeamCursor);
        break;
    case Wait:
        m_platformCursor = std::make_unique<QCursor>(Qt::WaitCursor);
        break;
    case Help:
        m_platformCursor = std::make_unique<QCursor>(Qt::WhatsThisCursor);
        break;
    case EastResize:
    case EastPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeHorCursor);
        break;
    case NorthResize:
    case NorthPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeVerCursor);
        break;
    case NorthEastResize:
    case NorthEastPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeBDiagCursor);
        break;
    case NorthWestResize:
    case NorthWestPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeFDiagCursor);
        break;
    case SouthResize:
    case SouthPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeVerCursor);
        break;
    case SouthEastResize:
    case SouthEastPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeFDiagCursor);
        break;
    case SouthWestResize:
    case SouthWestPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeBDiagCursor);
        break;
    case WestResize:
    case WestPanning:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeHorCursor);
        break;
    case NorthSouthResize:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeVerCursor);
        break;
    case EastWestResize:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeHorCursor);
        break;
    case NorthEastSouthWestResize:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeBDiagCursor);
        break;
    case NorthWestSouthEastResize:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeFDiagCursor);
        break;
    case ColumnResize:
        m_platformCursor = std::make_unique<QCursor>(Qt::SplitHCursor);
        break;
    case RowResize:
        m_platformCursor = std::make_unique<QCursor>(Qt::SplitVCursor);
        break;
    case MiddlePanning:
    case Move:
        m_platformCursor = std::make_unique<QCursor>(Qt::SizeAllCursor);
        break;
    case None:
        m_platformCursor = std::make_unique<QCursor>(Qt::BlankCursor);
        break;
    case NoDrop:
    case NotAllowed:
        m_platformCursor = std::make_unique<QCursor>(Qt::ForbiddenCursor);
        break;
    case Grab:
    case Grabbing:
        notImplemented();
        m_platformCursor = std::make_unique<QCursor>(Qt::ArrowCursor);
        break;
    case VerticalText:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/verticalTextCursor.png")), 7, 7);
        break;
    case Cell:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/cellCursor.png")), 7, 7);
        break;
    case ContextMenu:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/contextMenuCursor.png")), 3, 2);
        break;
    case Alias:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/aliasCursor.png")), 11, 3);
        break;
    case Progress:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/progressCursor.png")), 3, 2);
        break;
    case Copy:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/copyCursor.png")), 3, 2);
        break;
    case ZoomIn:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/zoomInCursor.png")), 7, 7);
        break;
    case ZoomOut:
        m_platformCursor = std::make_unique<QCursor>(QPixmap(QStringLiteral(":/webkit/resources/zoomOutCursor.png")), 7, 7);
        break;
    case Custom:
        m_platformCursor = createCustomCursor(m_image.get(), m_hotSpot);
        if (!m_platformCursor)
            m_platformCursor = std::make_unique<QCursor>(Qt::ArrowCursor);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
#endif
}

}

// vim: ts=4 sw=4 et
