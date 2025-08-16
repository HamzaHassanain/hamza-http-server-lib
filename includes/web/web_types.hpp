#pragma once
#include <functional>
#include <http_objects.hpp>
#include <memory>
#include <exceptions.hpp>
#include <web/web_response.hpp>
#include <web/web_request.hpp>
#include <web/web_exceptions.hpp>
namespace hamza::web
{
    using http_request_callback_t = std::function<void(hamza::http::http_request &, hamza::http::http_response &)>;
    using web_listen_success_callback_t = std::function<void()>;
    using web_error_callback_t = std::function<void(std::shared_ptr<web_general_exception>)>;
    using web_request_handler_t = std::function<int(std::shared_ptr<web_request>, std::shared_ptr<web_response>)>;

    const int EXIT = 1;
    const int CONTINUE = 0;
    const int ERROR = -1;
};