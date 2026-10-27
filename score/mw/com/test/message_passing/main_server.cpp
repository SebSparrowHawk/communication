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
// #include "score/mw/com/test/loading_add_on_configuration/provider.h"
// #include "score/mw/com/test/loading_add_on_configuration/test_constants.h"
// #include "score/mw/com/test/loading_add_on_configuration/types/addon_interface.h"
//
#include "score/mw/com/runtime.h"
#include "score/mw/com/test/common_test_resources/command_line_parser.h"
#include "score/mw/com/test/common_test_resources/fail_test.h"
#include "score/mw/com/test/common_test_resources/process_synchronizer.h"
// #include "score/mw/com/test/common_test_resources/skeleton_container.h"
#include "score/mw/com/test/common_test_resources/stop_token_sig_term_handler.h"

#include "score/message_passing/qnx_dispatch/qnx_dispatch_engine.h"
#include "score/message_passing/qnx_dispatch/qnx_dispatch_server_factory.h"
#include "score/message_passing/qnx_dispatch/qnx_dispatch_server.h"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>

#include "score/message_passing/i_server_connection.h"

class MyConnectionHandler : public score::message_passing::IConnectionHandler
{
    score::cpp::expected_blank<score::os::Error> OnMessageSent(
        score::message_passing::IServerConnection& connection,
        score::cpp::span<const std::uint8_t> message) noexcept override
    {
        std::cout << "MyConnectionHandler: message callback called " << connection.GetClientIdentity().pid << std::endl;
        std::cout << "MyConnectionHandler: Message: " << message.size() << " bytes" << std::endl;
        return {};
    }
    score::cpp::expected_blank<score::os::Error> OnMessageSentWithReply(
        score::message_passing::IServerConnection& connection,
        score::cpp::span<const std::uint8_t> message) noexcept override
    {
        std::cout << "MyConnectionHandler: message callback called " << connection.GetClientIdentity().pid << std::endl;
        std::cout << "MyConnectionHandler: Message: " << message.size() << " bytes" << std::endl;

        std::vector<std::uint8_t> response_data{113};
        const auto reply_result = connection.Reply(score::cpp::span<const std::uint8_t>{response_data.data(), response_data.size()});

        if (reply_result.has_value())
        {
            std::cout << "Seems like reply could be sent" << std::endl;
        } else
        {
            std::cout << "Could not sent reply" << std::endl;
        }

        return {};
    }
    void OnDisconnect(score::message_passing::IServerConnection& connection) noexcept override
    {
        std::cout << "MyConnectionHandler: disconnect callback called " << connection.GetClientIdentity().pid << std::endl;
    }
};


int main(/*int argc, const char** argv */)
{
    const auto resource = score::cpp::pmr::get_default_resource();
    const auto engine = score::cpp::pmr::make_shared<score::message_passing::QnxDispatchEngine>(resource, resource);


    score::message_passing::QnxDispatchServerFactory server_factory(engine);

    const auto server = server_factory.Create(score::message_passing::ServiceProtocolConfig{
        "test_service",
        10,
        10,
        10
    }, score::message_passing::IServerFactory::ServerConfig{
        10, 5, 10
    });

    int amount_messages_received = 0;
    uint8_t response_counter = 123;

    auto connect_callback = [resource](score::message_passing::IServerConnection& connection) -> score::cpp::expected<score::message_passing::UserData, score::os::Error>
    {
        std::cout << "connect callback called " << connection.GetClientIdentity().pid << std::endl;

        auto my_connection_handler = score::cpp::pmr::make_unique<MyConnectionHandler>(resource);
        return score::message_passing::UserData(std::move(my_connection_handler));
    };

    auto disconnect_callback = [](score::message_passing::IServerConnection& connection)
    {
        std::cout << "disconnect callback called " << connection.GetClientIdentity().pid << std::endl;
    };

    auto sent_callback = [&amount_messages_received](score::message_passing::IServerConnection& connection, score::cpp::span<const std::uint8_t> message) -> score::cpp::expected_blank<score::os::Error>
    {
        std::cout << "message callback called " << connection.GetClientIdentity().pid << std::endl;
        std::cout << "Message: " << message.size() << " bytes" << std::endl;
        amount_messages_received++;
        return {};
    };

    auto sent_reply_callback = [&response_counter, &amount_messages_received](score::message_passing::IServerConnection& connection, score::cpp::span<const std::uint8_t> message) -> score::cpp::expected_blank<score::os::Error>
    {
        std::cout << "message callback called " << connection.GetClientIdentity().pid << std::endl;
        std::cout << "Message: " << message.size() << " bytes" << std::endl;
        amount_messages_received++;

        std::vector<std::uint8_t> response_data{response_counter};
        const auto reply_result = connection.Reply(score::cpp::span<const std::uint8_t>{response_data.data(), response_data.size()});

        if (reply_result.has_value())
        {
            std::cout << "Seems like reply could be sent" << std::endl;
        } else
        {
            std::cout << "Could not sent reply" << std::endl;
        }

        return {};
    };

    const auto listening_result = server->StartListening(connect_callback, disconnect_callback, sent_callback, sent_reply_callback);

    if (!listening_result.has_value())
    {
        std::cerr << "Failed to start listening: " << std::endl;
        return EXIT_FAILURE;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    if (amount_messages_received == 0)
    {
        std::cerr << "No messages received, terminating" << std::endl;
        return EXIT_FAILURE;
    } else
    {
        std::cout << "Received " << amount_messages_received << " messages" << std::endl;
        return EXIT_SUCCESS;
    }

}
