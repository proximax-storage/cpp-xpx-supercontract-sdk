/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <string>
#include "RPCExecutorClient.h"

int main(int argc, char * argv[]) {

	// TODO This is a workaround to not to interrupt the process
	//  when the parent process is interrupted
	signal(SIGTERM, SIG_IGN);

    const int expectedArgc = 1 + 1;

    if (argc < expectedArgc) {
        return 1;
    }

    std::string rpcAddress = argv[1];

    sirius::contract::rpcExecutorClient::RPCExecutorClient executorClient(rpcAddress);
    auto barrier = executorClient.run();
    barrier.wait();

    return 0;
}

