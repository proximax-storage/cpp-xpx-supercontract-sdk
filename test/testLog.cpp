/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include "gtest/gtest.h"
#include <string>

namespace sirius::contract::test {

#define TEST_NAME Log

TEST(Internet, TEST_NAME) {
//    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
//    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("mylog.txt", 1024*1024*10, 3);
//    std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
    spdlog::logger logger("multi_sink", {stdout_sink});;
//    auto logger = std::make_shared<spdlog::logger>("loggername", {stdout_sink});
//    spdlog::register_logger(logger);

//    auto stdout_sink2 = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
//    auto rotating_sink2 = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("mylog.txt", 1024*1024*10, 3);
//    std::vector<spdlog::sink_ptr> sinks2 {stdout_sink2, rotating_sink2};
//    auto logger2 = std::make_shared<spdlog::async_logger>("loggername2", sinks2.begin(), sinks2.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
//    spdlog::register_logger(logger2);

    auto h1 = std::thread([&] {
        for (int i = 0; i < 1000; i++)
        {
            logger.critical("this should appear in both console and file");
        }
    });

//    auto h2 = std::thread([=] {
//        for (int i = 0; i < 1000; i++)
//        {
//            logger2->warn("B {}", i);
//        }
//    });

    auto h3 = std::thread([=] {
        for (int i = 0; i < 10000; i++)
        {
            std::cout << "C " << i << std::endl;
        }
    });

    h1.join();
//    h2.join();
    h3.join();
}

}