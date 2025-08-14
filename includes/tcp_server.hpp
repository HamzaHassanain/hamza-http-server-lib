#pragma once

#include <clients_container.hpp>
#include <socket_address.hpp>
#include <exceptions.hpp>
#include <socket.hpp>
#include <thread>
#include <functional>
#include <memory>
#include <cstring>

namespace hamza
{

    class tcp_server
    {
    private:
        hamza::clients_container clients;
        fd_set master_fds;
        fd_set read_fds;
        int max_fds;
        std::shared_ptr<hamza::socket> server_socket;

        struct timeval make_timeout(int seconds);
        void remove_client(std::shared_ptr<hamza::socket> sock_ptr);
        void check_for_activity();

    public:
        tcp_server(const hamza::socket_address &addr);
        void run();
        void close_connection(std::shared_ptr<hamza::socket> sock_ptr);
        virtual void on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message) = 0;
        virtual void on_waiting_for_activity();
        virtual void on_new_client_connected(std::shared_ptr<hamza::socket> sock_ptr) = 0;
        virtual void on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr) = 0;
        virtual void on_listen_success() = 0;
        virtual void on_exception(std::unique_ptr<general_socket_exception> e) = 0;

    private:
        void start_listening();
        void handle_client_activity(std::shared_ptr<hamza::socket> sock_ptr);
        void handle_new_connection();
    };
};
