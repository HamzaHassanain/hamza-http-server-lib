#pragma once
#include <stdexcept>

namespace hamza
{

    class general_socket_exception : public std::runtime_error
    {
    public:
        explicit general_socket_exception(const std::string &message)
            : std::runtime_error(message) {}

        virtual std::string type() const noexcept
        {
            return "GeneralSocketException";
        }
    };
    class server_listener_exception : public general_socket_exception
    {
    public:
        explicit server_listener_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ServerListenerException";
        }
    };
    class server_accept_exception : public general_socket_exception
    {
    public:
        explicit server_accept_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ServerAcceptException";
        }
    };
    class client_connection_exception : public general_socket_exception
    {
    public:
        explicit client_connection_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ClientConnectionException";
        }
    };

    class client_remove_exception : public general_socket_exception
    {
    public:
        explicit client_remove_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ClientRemoveException";
        }
    };

    class client_activity_exception : public general_socket_exception
    {
    public:
        explicit client_activity_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ClientActivityException";
        }
    };

    class client_message_exception : public general_socket_exception
    {
    public:
        explicit client_message_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ClientMessageException";
        }
    };

    class client_disconnect_exception : public general_socket_exception
    {
    public:
        explicit client_disconnect_exception(const std::string &message)
            : general_socket_exception(message) {}

        std::string type() const noexcept override
        {
            return "ClientDisconnectException";
        }
    };

};