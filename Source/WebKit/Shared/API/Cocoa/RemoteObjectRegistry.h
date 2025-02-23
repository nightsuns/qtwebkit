/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "MessageReceiver.h"
#include "ProcessThrottler.h"
#include <WebCore/PageIdentifier.h>
#include <wtf/Function.h>
#include <wtf/WeakObjCPtr.h>
#include <wtf/WeakPtr.h>

OBJC_CLASS _WKRemoteObjectRegistry;

namespace IPC {
class MessageSender;
}

namespace WebKit {

class RemoteObjectInvocation;
class UserData;
class WebPage;
class WebPageProxy;

class RemoteObjectRegistry final : public CanMakeWeakPtr<RemoteObjectRegistry>, public IPC::MessageReceiver {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RemoteObjectRegistry(_WKRemoteObjectRegistry *, WebPage&);
    RemoteObjectRegistry(_WKRemoteObjectRegistry *, WebPageProxy&);

    ~RemoteObjectRegistry();

    void sendInvocation(const RemoteObjectInvocation&);
    void sendReplyBlock(uint64_t replyID, const UserData& blockInvocation);
    void sendUnusedReply(uint64_t replyID);

    void close();

private:
    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;

    // Message handlers
    void invokeMethod(const RemoteObjectInvocation&);
    void callReplyBlock(uint64_t replyID, const UserData& blockInvocation);
    void releaseUnusedReplyBlock(uint64_t replyID);

    WeakObjCPtr<_WKRemoteObjectRegistry> m_remoteObjectRegistry;
    IPC::MessageSender& m_messageSender;
    Function<ProcessThrottler::BackgroundActivityToken()> m_takeBackgroundActivityToken;
    Function<void()> m_launchInitialProcessIfNecessary;
    HashMap<uint64_t, ProcessThrottler::BackgroundActivityToken> m_pendingReplies;
    bool m_isRegisteredAsMessageReceiver { false };
    WebCore::PageIdentifier m_messageReceiverID;
};

} // namespace WebKit
