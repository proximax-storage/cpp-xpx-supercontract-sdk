///*
//*** Copyright 2021 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//*/
//
//#include <virtualMachine/RPCInternetService.h>
//#include "OpenConnectionRPCInternetResponce.h"
//#include
//
//namespace sirius::contract::vm {
//
//InternetService::InternetService( const SessionId& sessionId,
//                                  const std::shared_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>>& handlersExtractor,
//                                  GlobalEnvironment& environment )
//        : RPCServiceImpl( sessionId, handlersExtractor, environment ) {}
//
//void InternetService::registerCalls() {
//
//    ASSERT(isSingleThread(), m_environment.logger() )
//
//    new OpenConnectionRPCInternetRequest( m_sessionId, &m_service, m_completionQueue.get(), m_handlersExtractor,
//                                          m_terminated );
//    new ReadConnectionRPCInternetRequest( m_sessionId, &m_service, m_completionQueue.get(), m_handlersExtractor,
//                                          m_terminated );
//    new CloseConnectionRPCInternetRequest( m_sessionId, &m_service, m_completionQueue.get(), m_handlersExtractor,
//                                           m_terminated );
//}
//
//}
//
