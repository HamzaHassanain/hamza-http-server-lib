#include <http_request.hpp>
#include <utilities.hpp>

namespace hamza_http
{
    http_request::http_request(const std::string &method, const std::string &uri, const std::string &version,
                               const std::multimap<std::string, std::string> &headers,
                               const std::string &body,
                               std::shared_ptr<hamza::socket> client_socket)
        : method(method), uri(uri), version(version), headers(headers), body(body), client_socket(client_socket)
    {

        std::multimap<std::string, std::string> lower_case_headers;
        for (const auto &header : headers)
        {
            lower_case_headers.insert({hamza::to_upper_case(header.first), header.second});
        }
        this->headers = std::move(lower_case_headers);
    }

    http_request::http_request(http_request &&other)
        : method(std::move(other.method)), uri(std::move(other.uri)), version(std::move(other.version)),
          headers(std::move(other.headers)), body(std::move(other.body)),
          client_socket(std::move(other.client_socket)), close_connection(std::move(other.close_connection))
    {
        other.client_socket.reset(); // Reset the moved-from socket
    }

    void http_request::destroy(bool Isure)
    {
        if (!Isure)
        {
            throw std::runtime_error("Insure is false, cannot destroy request.");
        }
        close_connection(client_socket);
        uri.clear();
        headers.clear();
        body.clear();
        client_socket.reset();
        body.clear(); // Note: This appears to be a duplicate clear() call
    }

    std::string http_request::get_method() const
    {
        return method;
    }

    std::string http_request::get_uri() const
    {
        return uri;
    }

    std::string http_request::get_version() const
    {
        return version;
    }

    std::vector<std::string> http_request::get_header(const std::string &name) const
    {
        std::vector<std::string> values;
        auto range = headers.equal_range(hamza::to_upper_case(name));
        for (auto it = range.first; it != range.second; ++it)
        {
            values.push_back(it->second);
        }
        return values;
    }

    std::vector<std::pair<std::string, std::string>> http_request::get_headers() const
    {
        std::vector<std::pair<std::string, std::string>> headers_vector;
        for (const auto &header : headers)
        {
            headers_vector.emplace_back(hamza::to_upper_case(header.first), header.second);
        }
        return headers_vector;
    }

    std::string http_request::get_body() const
    {
        return body;
    }
}