#include <http_server.hpp>
#include <iostream>
#include <sstream>

namespace hamza_http
{
    http_server::http_server(const hamza::socket_address &addr) : hamza::tcp_server(addr) {}

    void http_server::on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message)
    {
        if (message.empty())
        {
            close_connection(sock_ptr);
            return;
        }

        if (!request_callback)
        {
            throw std::runtime_error("Request callback is not set.");
            exit(1);
            return;
        }

        std::istringstream request_stream(message.to_string());
        std::ostringstream body_stream;

        std::string method, uri, version, body;
        std::multimap<std::string, std::string> headers;
        std::string line;

        // Parse request line
        if (std::getline(request_stream, line) && !line.empty())
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            std::istringstream request_line(line);
            request_line >> method >> uri >> version;
        }

        while (std::getline(request_stream, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            // Empty line indicates end of headers
            if (line.empty())
            {
                break;
            }

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string header_name = line.substr(0, colon_pos);
                std::string header_value = line.substr(colon_pos + 1);

                // Trim whitespace
                size_t start = header_value.find_first_not_of(" \t");
                if (start != std::string::npos)
                {
                    header_value = header_value.substr(start);
                }
                size_t end = header_value.find_last_not_of(" \t");
                if (end != std::string::npos)
                {
                    header_value = header_value.substr(0, end + 1);
                }

                headers.emplace(header_name, header_value);
            }
        }

        // Handle Content-Length for POST requests
        int content_length = 0;
        auto content_length_it = headers.find("Content-Length");
        if (content_length_it != headers.end())
        {
            content_length = std::stoi(content_length_it->second);
        }

        // Read the request body
        body_stream << request_stream.rdbuf();
        body = body_stream.str();

        if (content_length < body.size())
        {
            throw std::runtime_error("Content-Length mismatch: expected " + std::to_string(content_length) +
                                     " but received " + std::to_string(body.size()));
        }

        auto close_connection_for_objects = [this](std::shared_ptr<hamza::socket> client_socket)
        {
            this->close_connection(client_socket);
        };

        http_request request(method, uri, version, headers, body, sock_ptr);
        http_response response("HTTP/1.1", {}, sock_ptr);

        request.close_connection = close_connection_for_objects;
        response.close_connection = close_connection_for_objects;

        response.add_header("Connection", "close");

        request_callback(request, response);
    }

    void http_server::on_listen_success()
    {
        if (listen_success_callback)
            listen_success_callback();
    }

    void http_server::on_exception(std::shared_ptr<hamza::general_socket_exception> e)
    {

        if (error_callback)
            error_callback(std::move(e));
        else
        {
            std::cerr << "Type: " << e->type() << std::endl;
            std::cerr << "Socket error: " << e->what() << std::endl;
        }
    }

    void http_server::on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // for debugging purposes
        // std::cout << "--------------------------------------------------------\n";
        // std::cout << "Client disconnected: " << sock_ptr->get_remote_address() << std::endl;
        // std::cout << "--------------------------------------------------------\n";
    }

    void http_server::on_new_client_connected(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // for debugging purposes
        // std::cout << "--------------------------------------------------------\n";
        // std::cout << "New client connected: " << sock_ptr->get_remote_address() << std::endl;
        // std::cout << "--------------------------------------------------------\n";
    }

    void http_server::set_request_callback(std::function<void(http_request &, http_response &)> callback)
    {
        request_callback = std::move(callback);
    }

    void http_server::set_listen_success_callback(std::function<void()> callback)
    {
        listen_success_callback = std::move(callback);
    }

    void http_server::set_error_callback(std::function<void(std::shared_ptr<hamza::general_socket_exception>)> callback)
    {
        error_callback = std::move(callback);
    }
}