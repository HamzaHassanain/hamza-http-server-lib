#pragma once
#include <functional>
#include <http_objects.hpp>
#include <memory>
#include <exceptions.hpp>
#include <web_response.hpp>
#include <web_request.hpp>
namespace hamza::web
{
    using request_callback_t = std::function<void(hamza::http::http_request &, hamza::http::http_response &)>;
    using listen_success_callback_t = std::function<void()>;
    using error_callback_t = std::function<void(std::shared_ptr<hamza::general_socket_exception>)>;
    using request_handler_t = std::function<void(std::shared_ptr<web_request>, std::shared_ptr<web_response>)>;
};