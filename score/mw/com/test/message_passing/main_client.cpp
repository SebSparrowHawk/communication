/*******************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

// #include "score/mw/com/test/loading_add_on_configuration/common_resources.h"
// #include "score/mw/com/test/loading_add_on_configuration/consumer.h"
// #include "score/mw/com/test/loading_add_on_configuration/test_constants.h"
// #include "score/mw/com/test/loading_add_on_configuration/types/addon_interface.h"
// #include "score/mw/com/test/loading_add_on_configuration/types/example_interface.h"

#include <iostream>

#include "score/mw/com/test/common_test_resources/command_line_parser.h"
#include "score/mw/com/test/common_test_resources/fail_test.h"
#include "score/mw/com/test/common_test_resources/process_synchronizer.h"
// #include "score/mw/com/test/common_test_resources/proxy_container.h"
#include "score/mw/com/test/common_test_resources/stop_token_sig_term_handler.h"

#include "score/mw/com/runtime.h"


#include "score/message_passing/client_factory.h"

#include <cstdlib>
#include <iostream>

#include "score/message_passing/qnx_dispatch/qnx_dispatch_engine.h"
#include "score/message_passing/qnx_dispatch/qnx_dispatch_client_factory.h"

#include <thread>

int main(int argc, const char** argv)
{

    const auto resource = score::cpp::pmr::get_default_resource();
    const auto engine = score::cpp::pmr::make_shared<score::message_passing::QnxDispatchEngine>(resource, resource);

    auto client_factory = score::message_passing::QnxDispatchClientFactory(engine);

    const auto client_connection = client_factory.Create(score::message_passing::ServiceProtocolConfig{
        "test_service",
        10,
        10,
        10
    }, score::message_passing::IClientFactory::ClientConfig{
        10, 5, true, true, true
    });

    auto state_callback = [](score::message_passing::IClientConnection::State state) -> void {
        std::cout << "State callback called: " << int(state) << std::endl;
    };

    auto notify_callback = [](score::cpp::span<const std::uint8_t> message) -> void {
        std::cout << "Notify callback called: " << message.size() << " bytes" << std::endl;
    };

    client_connection->Start(state_callback, notify_callback);

    std::vector<std::uint8_t> underlying_data{1,2,3,4};
    score::cpp::span<std::uint8_t> message{underlying_data.data(), underlying_data.size()};


    std::vector<std::uint8_t> reply_buffer(10);
    score::cpp::span<std::uint8_t> reply_message{reply_buffer.data(), reply_buffer.size()};

    const auto send_result = client_connection->SendWaitReply(message, reply_message);

    bool send_success = false;
    if (send_result.has_value())
    {
        std::cout << "Apparently send&reply was successful and reply was: " << reply_message[0] << std::endl;
        send_success = true;

    } else
    {
        std::cout << "Send&reply failed: " << send_result.error() << std::endl;
    }

    std::cout << "Arg count " << argc << " and elements: " << argv << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(10));

    if (send_success)
    {
        return EXIT_SUCCESS;
    } else
    {
        return EXIT_FAILURE;
    }

}
