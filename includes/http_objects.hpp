#pragma once

#include <bits/stdc++.h>
#include <socket.hpp>
#include <http_headers.hpp>

namespace hamza::http
{
    class http_request
    {
    private:
        std::string method;
        std::string uri;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
        std::shared_ptr<hamza::socket> client_socket;
        std::function<void(std::shared_ptr<hamza::socket>)> close_connection;

        http_request(const std::string &method, const std::string &uri, const std::string &version,
                     const std::map<std::string, std::string> &headers, const std::string &body,
                     std::shared_ptr<hamza::socket> client_socket);

    public:
        friend class http_server;

        void destroy(bool Isure);
        std::string get_method() const;
        std::string get_uri() const;
        std::string get_version() const;
        std::string get_header(const std::string &name) const;
        std::vector<std::pair<std::string, std::string>> get_headers() const;
        std::string get_body() const;
    };

    class http_response
    {
    private:
        std::string version = "HTTP/1.1";
        int status_code = 200;
        std::string status_message = "OK";
        std::map<std::string, std::string> headers, trailers;
        std::string body;
        std::shared_ptr<hamza::socket> client_socket;
        std::function<void(std::shared_ptr<hamza::socket>)> close_connection;

        bool validate() const;
        http_response(const std::string &version, const std::map<std::string, std::string> &headers,
                      std::shared_ptr<hamza::socket> client_socket);

    public:
        friend class http_server;

        std::string to_string() const;
        void set_body(const std::string &body);
        void set_status(int status_code, const std::string &status_message);
        void set_version(const std::string &version);
        void add_trailer(const std::string &name, const std::string &value);
        void add_header(const std::string &name, const std::string &value);
        std::string get_body() const;
        std::string get_version() const;
        std::string get_status_message() const;
        int get_status_code() const;
        std::string get_header(const std::string &name) const;
        std::string get_trailer(const std::string &name) const;
        void end();
    };
} // namespace hamza