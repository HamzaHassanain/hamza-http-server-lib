#pragma once
#include <exceptions.hpp>

namespace hamza::web
{
    class web_general_exception : public general_socket_exception
    {
        int status_code = 500;
        std::string status_message = "Internal Server Error";

    public:
        explicit web_general_exception(const std::string &message) : general_socket_exception(message) {}
        explicit web_general_exception(const std::string &message, int status_code, const std::string &status_message) : general_socket_exception(message), status_code(status_code), status_message(status_message) {}
        std::string type() const noexcept override
        {
            return "WebGeneralException";
        }

        int get_status_code() const noexcept
        {
            return status_code;
        }

        std::string get_status_message() const noexcept
        {
            return status_message;
        }
    };

    class web_not_found_exception : public web_general_exception
    {
    public:
        explicit web_not_found_exception(const std::string &message) : web_general_exception(message, 404, "Not Found") {}
        std::string type() const noexcept override
        {
            return "WebNotFoundException";
        }
    };

    class web_method_not_allowed_exception : public web_general_exception
    {
    public:
        explicit web_method_not_allowed_exception(const std::string &message) : web_general_exception(message, 405, "Method Not Allowed") {}
        std::string type() const noexcept override
        {
            return "WebMethodNotAllowedException";
        }
    };

    class web_bad_request_exception : public web_general_exception
    {
    public:
        explicit web_bad_request_exception(const std::string &message) : web_general_exception(message, 400, "Bad Request") {}
        std::string type() const noexcept override
        {
            return "WebBadRequestException";
        }
    };
    class web_unauthorized_exception : public web_general_exception
    {
    public:
        explicit web_unauthorized_exception(const std::string &message) : web_general_exception(message, 401, "Unauthorized") {}
        std::string type() const noexcept override
        {
            return "WebUnauthorizedException";
        }
    };
    class web_internal_server_error_exception : public web_general_exception
    {
    public:
        explicit web_internal_server_error_exception(const std::string &message) : web_general_exception(message, 500, "Internal Server Error") {}
        std::string type() const noexcept override
        {
            return "WebInternalServerErrorException";
        }
    };

}