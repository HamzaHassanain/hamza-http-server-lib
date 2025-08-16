#pragma once

#include <bits/stdc++.h>
#include <socket_address.hpp>
#include <socket.hpp>
#include <tcp_server.hpp>
#include <string>
#include <http_objects.hpp>

namespace hamza::http
{
    class http_server : public hamza::tcp_server
    {
    private:
        std::function<void(http_request &, http_response &)> request_callback;
        std::function<void()> listen_success_callback;
        std::function<void(std::shared_ptr<hamza::general_socket_exception>)> error_callback;

    protected:
        void on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message) override;
        void on_listen_success() override;
        void on_exception(std::shared_ptr<hamza::general_socket_exception> e) override;
        void on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr) override;
        void on_new_client_connected(std::shared_ptr<hamza::socket> sock_ptr) override;

    public:
        http_server(const hamza::socket_address &addr);
        http_server(const std::string &ip, int port)
            : http_server(hamza::socket_address(hamza::ip_address(ip), hamza::port(port), hamza::family(hamza::IPV4))) {}

        // disable copy and move
        http_server(const http_server &) = delete;
        http_server &operator=(const http_server &) = delete;
        http_server(http_server &&) = delete;
        http_server &operator=(http_server &&) = delete;

        void set_request_callback(std::function<void(http_request &, http_response &)> callback);
        void set_listen_success_callback(std::function<void()> callback);
        void set_error_callback(std::function<void(std::shared_ptr<hamza::general_socket_exception>)> callback);
    };
}