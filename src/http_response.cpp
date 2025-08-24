#include <http_response.hpp>
#include <utilities.hpp>
#include <iostream>
#include <sstream>
#include <memory>
#include <map>

#include <ctime>

namespace hamza_http
{
    http_response::http_response(const std::string &version, const std::multimap<std::string, std::string> &headers,
                                 std::function<void()> close_connection,
                                 std::function<void(const std::string &)> send_message)
        : version(version), headers(headers), close_connection(close_connection), send_message(send_message)
    {

        std::multimap<std::string, std::string> lower_case_headers;

        for (const auto &header : headers)
        {
            lower_case_headers.insert({to_upper_case(header.first), header.second});
        }
        this->headers = std::move(lower_case_headers);
    }

    http_response::http_response(http_response &&other)
        : version(std::move(other.version)), status_code(other.status_code),
          status_message(std::move(other.status_message)), headers(std::move(other.headers)),
          trailers(std::move(other.trailers)), body(std::move(other.body)),
          send_message(std::move(other.send_message)), close_connection(std::move(other.close_connection))
    {
        other.status_code = 0;            // Invalidate the moved-from response
        other.send_message = nullptr;     // Reset the moved-from send_message
        other.close_connection = nullptr; // Reset the moved-from close_connection
    }

    bool http_response::validate() const
    {
        // if (!client_socket)
        // {
        //     return false;
        // }

        return true;
    }

    std::string get_current_date()
    {
        // Get current time
        std::time_t now = std::time(nullptr);
        std::tm tm = *std::gmtime(&now);

        // Format date according to RFC 1123
        char buffer[30];
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
        return std::string(buffer);
    }

    std::string http_response::to_string() const
    {
        std::ostringstream response_stream;
        response_stream << version << " " << status_code << " " << status_message << "\r\n";
        response_stream << "Date: " << get_current_date() << "\r\n";
        for (const auto &header : headers)
        {
            response_stream << to_upper_case(header.first) << ": " << header.second << "\r\n";
        }
        if (body.size())
            response_stream << "\r\n"
                            << body;
        else
            response_stream << "\r\n";
        for (const auto &trailer : trailers)
        {
            response_stream << to_upper_case(trailer.first) << ": " << trailer.second << "\r\n";
        }

        return response_stream.str();
    }

    void http_response::set_body(const std::string &body)
    {
        this->body = body;
    }

    void http_response::set_status(int status_code, const std::string &status_message)
    {
        this->status_code = status_code;
        this->status_message = status_message;
    }

    void http_response::set_version(const std::string &version)
    {
        this->version = version;
    }

    void http_response::add_trailer(const std::string &name, const std::string &value)
    {

        trailers.insert({to_upper_case(name), value});
    }

    void http_response::add_header(const std::string &name, const std::string &value)
    {
        headers.insert({to_upper_case(name), value});
    }

    std::string http_response::get_body() const
    {
        return body;
    }

    std::string http_response::get_version() const
    {
        return version;
    }

    std::string http_response::get_status_message() const
    {
        return status_message;
    }

    int http_response::get_status_code() const
    {
        return status_code;
    }

    std::vector<std::string> http_response::get_header(const std::string &name) const
    {
        std::vector<std::string> values;
        auto range = headers.equal_range(to_upper_case(name));
        for (auto it = range.first; it != range.second; ++it)
        {
            values.push_back(it->second);
        }
        return values;
    }

    std::vector<std::string> http_response::get_trailer(const std::string &name) const
    {
        std::vector<std::string> values;
        auto range = trailers.equal_range(to_upper_case(name));
        for (auto it = range.first; it != range.second; ++it)
        {
            values.push_back(it->second);
        }
        return values;
    }

    void http_response::end()
    {
        try
        {
            if (validate())
            {
                close_connection();
            }
            else
            {
                throw std::runtime_error("Invalid HTTP response or client connection may be already closed");
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error ending HTTP response: " + std::string(e.what()));
        }
    }

    void http_response::send()
    {

        try
        {
            if (validate())
            {
                send_message(to_string());
            }
            else
            {
                throw std::runtime_error("Invalid HTTP response or client connection may be already closed");
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error sending HTTP response:\n" + std::string(e.what()));
        }
    }
}