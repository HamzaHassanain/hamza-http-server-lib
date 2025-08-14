#include <tcp_server.hpp>
#include <clients_container.hpp>
#include <socket_address.hpp>
#include <socket.hpp>
#include <thread>
#include <functional>
#include <memory>
#include <cstring>
#include <algorithm>
namespace hamza
{
    tcp_server::tcp_server(const hamza::socket_address &addr)
    {
        server_socket = std::make_shared<hamza::socket>(addr, hamza::Protocol::TCP, true);
        server_socket->listen();

        fd_select_server.init(server_socket->get_file_descriptor());
        fd_select_server.set_timeout(1);
    }
    struct timeval tcp_server::make_timeout(int seconds)
    {
        struct timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;

        return timeout;
    }

    void tcp_server::remove_client(std::shared_ptr<hamza::socket> sock_ptr)
    {
        try
        {
            if (sock_ptr == nullptr)
            {
                throw std::runtime_error("Invalid socket pointer.");
            }

            if (server_socket == sock_ptr)
            {
                throw std::runtime_error("Cannot remove server socket.");
            }

            if (server_socket == nullptr)
            {
                throw std::runtime_error("Server socket is not initialized.");
            }

            // FD_CLR(sock_ptr->get_file_descriptor().get(), &master_fds);
            fd_select_server.remove_fd(sock_ptr->get_file_descriptor());
            clients.erase(sock_ptr);
            sock_ptr->disconnect();

            int server_fd = server_socket->get_file_descriptor().get();

            fd_select_server.set_max_fd(std::max(server_socket->get_file_descriptor().get(), clients.max()));
            this->on_client_disconnect(sock_ptr);
        }
        catch (const std::exception &e)
        {
            std::string error_message = "Error removing client: " + std::string(e.what());
            this->on_exception(std::make_unique<client_remove_exception>(error_message));
        }
    }

    void tcp_server::check_for_activity()
    {
        try
        {

            if (server_socket == nullptr)
            {
                throw std::runtime_error("Server socket is not initialized.");
            }

            if (fd_select_server.is_fd_set(server_socket->get_file_descriptor()))
            {
                handle_new_connection();
            }

            for (auto &sock_ptr : clients)
            {
                if (sock_ptr == nullptr || !sock_ptr->is_connected())
                {
                    throw std::runtime_error("Invalid socket pointer in clients container.");
                }

                if (fd_select_server.is_fd_set(sock_ptr->get_file_descriptor()))
                {
                    handle_client_activity(sock_ptr);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::string error_message = "Error checking for activity: " + std::string(e.what());
            this->on_exception(std::make_unique<server_listener_exception>(error_message));
        }
    }

    void tcp_server::start_listening()
    {
        this->on_listen_success();
        while (true)
        {
            try
            {
                // read_fds = master_fds;

                // auto timeout = make_timeout(1);
                // int activity = select(max_fds + 1, &read_fds, nullptr, nullptr, &timeout);
                int activity = fd_select_server.select();
                if (activity < 0)
                {
                    if (errno == EINTR)
                        throw std::runtime_error("Select interrupted");
                    throw std::runtime_error("Select error: " + std::string(strerror(errno)));
                }
                if (activity == 0)
                {
                    this->on_waiting_for_activity();
                }

                check_for_activity();
            }
            catch (const std::exception &e)
            {
                std::string error_message = "Error in tcp_server loop: " + std::string(e.what());
                this->on_exception(std::make_unique<server_listener_exception>(error_message));
            }
        }
    }

    void tcp_server::handle_client_activity(std::shared_ptr<hamza::socket> sock_ptr)
    {
        std::string client_ip;
        std::string client_port;
        try
        {
            if (sock_ptr == nullptr)
            {
                throw std::runtime_error("Invalid socket pointer.");
            }

            client_ip = sock_ptr->get_remote_address().get_ip_address().get();
            client_port = std::to_string(sock_ptr->get_remote_address().get_port().get());

            hamza::data_buffer db = sock_ptr->receive_on_connection();
            this->on_message_received(sock_ptr, db);
        }
        catch (const std::exception &e)
        {
            std::string error_message = "Error handling client activity: " + std::string(e.what());
            error_message += " for client: " + client_ip + ":" + client_port + "\nRemove any misbehaving clients is recommended.";
            this->on_exception(std::make_unique<client_activity_exception>(error_message));
            // remove_client(sock_ptr);
        }
    }

    void tcp_server::handle_new_connection()
    {
        try
        {
            auto client_socket = server_socket->accept();
            auto client_socket_ptr = std::make_shared<hamza::socket>(std::move(client_socket));

            if (client_socket_ptr == nullptr)
            {
                throw std::runtime_error("Failed to create client socket.");
            }

            clients.insert(client_socket_ptr);
            // FD_SET(client_socket_ptr->get_file_descriptor().get(), &master_fds);
            fd_select_server.add_fd(client_socket_ptr->get_file_descriptor());

            int server_fd = server_socket->get_file_descriptor().get();
            int max_client_fd = clients.max();
            int max_fds = std::max(server_fd, max_client_fd);
            fd_select_server.set_max_fd(max_fds);

            this->on_new_client_connected(client_socket_ptr);
        }
        catch (const std::exception &e)
        {
            std::string error_message = "Error handling new connection: " + std::string(e.what());
            this->on_exception(std::make_unique<server_accept_exception>(error_message));
        }
    }

    void tcp_server::run()
    {
        start_listening();
    }
    void tcp_server::close_connection(std::shared_ptr<hamza::socket> sock_ptr)
    {
        std::lock_guard<std::mutex> lock(close_mutex);
        if (sock_ptr)
        {
            remove_client(sock_ptr);
        }
    }
    void tcp_server::on_waiting_for_activity()
    {
        // Default implementation does nothing
        // Can be overridden by derived classes if needed
    }
};
