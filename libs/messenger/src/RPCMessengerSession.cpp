/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCMessengerSession.h"

#include "ReadInputMessageTag.h"
#include "WriteOutputMessageTag.h"
#include "WriteSubscribeTag.h"
#include "StartSessionTag.h"
#include "FinishSessionTag.h"

namespace sirius::contract::messenger {

RPCMessengerSession::RPCMessengerSession(GlobalEnvironment& environment,
                                         messengerServer::MessengerServer::Stub& stub)
        : m_environment(environment)
        , m_stub(stub)
        , m_stream(m_stub.PrepareAsyncCommunicate(&m_context, &m_completionQueue))
        , m_completionQueueThread([this] {
            waitForRPCResponse();
        })
        {}


void RPCMessengerSession::read(std::shared_ptr<AsyncQueryCallback<InputMessage>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto* tag = new ReadInputMessageTag(m_environment, std::move(callback));
    m_stream->Read(&tag->m_response, tag);
}

void RPCMessengerSession::write(const OutputMessage& message, std::shared_ptr<AsyncQueryCallback<void>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())
    m_environment.logger().error("Session write ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    m_environment.logger().error("Session write tag: {} ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", message.m_tag);
    std::stringstream ss;
    ss << std::hex << std::setfill('0'); // Set fill character to '0' for padding

    for (const auto& ch : message.m_content) {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(ch));
    }
    m_environment.logger().error("Session write content: {} ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", ss.str());
    


    auto* outputMessage = new messengerServer::OutputMessage();
    outputMessage->set_receiver(message.m_receiver.toString());
    outputMessage->set_tag(message.m_tag);
    outputMessage->set_content(message.m_content);

    messengerServer::ClientMessage clientMessage;
    clientMessage.set_allocated_output_message(outputMessage);

    auto* tag = new WriteOutputMessageTag(m_environment, std::move(callback));
    m_stream->Write(clientMessage, tag);
}

void RPCMessengerSession::waitForRPCResponse() {

    ASSERT(!isSingleThread(), m_environment.logger())

    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCMessengerSession::initiate(std::shared_ptr<AsyncQueryCallback<void>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto* tag = new StartSessionTag(m_environment, std::move(callback));
    m_stream->StartCall(tag);
}

void RPCMessengerSession::subscribe(const std::string& tag, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto* subscribe = new messengerServer::Subscribe();
    subscribe->set_tag(tag);

    messengerServer::ClientMessage clientMessage;
    clientMessage.set_allocated_subscribe(subscribe);

    auto* rpcTag = new WriteSubscribeTag(m_environment, std::move(callback));
    m_stream->Write(clientMessage, rpcTag);
}

RPCMessengerSession::~RPCMessengerSession() {

    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();

    auto* tag = new FinishSessionTag(m_environment);
    m_stream->Finish(&tag->m_status, tag);

    m_completionQueue.Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

}