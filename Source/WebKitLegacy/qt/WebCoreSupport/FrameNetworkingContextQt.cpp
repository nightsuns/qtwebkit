/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#include "FrameNetworkingContextQt.h"

#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "qwebsettings.h"

#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/NotImplemented.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

FrameNetworkingContextQt::FrameNetworkingContextQt(Frame* frame, QObject* originatingObject, bool mimeSniffingEnabled)
    : FrameNetworkingContext(frame)
    , m_originatingObject(originatingObject)
    , m_mimeSniffingEnabled(mimeSniffingEnabled)
{
}

void FrameNetworkingContextQt::setSession(std::unique_ptr<NetworkStorageSession>&& session)
{
    m_session = WTFMove(session);
}

Ref<FrameNetworkingContextQt> FrameNetworkingContextQt::create(Frame* frame, QObject* originatingObject, bool mimeSniffingEnabled)
{
    Ref<FrameNetworkingContextQt> self = adoptRef(*new FrameNetworkingContextQt(frame, originatingObject, mimeSniffingEnabled));
    self->setSession(std::make_unique<NetworkStorageSession>(PAL::SessionID::defaultSessionID(), self.ptr()));
    return self;
}

NetworkStorageSession* FrameNetworkingContextQt::storageSession() const
{
    return m_session.get();
}

QObject* FrameNetworkingContextQt::originatingObject() const
{
    return m_originatingObject;
}

QNetworkAccessManager* FrameNetworkingContextQt::networkAccessManager() const
{
    return QWebFrameAdapter::kit(frame())->pageAdapter->networkAccessManager();
}

bool FrameNetworkingContextQt::mimeSniffingEnabled() const
{
    return m_mimeSniffingEnabled;
}

bool FrameNetworkingContextQt::thirdPartyCookiePolicyPermission(const QUrl& url) const
{
    switch (QWebSettings::globalSettings()->thirdPartyCookiePolicy()) {
    case QWebSettings::AlwaysAllowThirdPartyCookies:
        return true;
    case QWebSettings::AlwaysBlockThirdPartyCookies:
        return false;
    case QWebSettings::AllowThirdPartyWithExistingCookies: {
        QList<QNetworkCookie> cookies = networkAccessManager()->cookieJar()->cookiesForUrl(url);
        return !cookies.isEmpty();
    }
    default:
        break;
    }

    return false;
}

}
