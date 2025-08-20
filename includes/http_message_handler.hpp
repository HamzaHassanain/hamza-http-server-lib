#pragma once

#include <socket.hpp>
#include <http_handled_data.hpp>
#include <http_data_under_handling.hpp>
#include <data_buffer.hpp>
#include <memory>
#include <map>
#include <mutex>
#include <sstream>
namespace hamza_http
{

    class http_message_handler
    {
        std::map<std::string, http_data_under_handling> under_handling_data;
        mutable std::mutex data_mutex;

    public:
        http_handled_data handle(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message)
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            auto socket_key = sock_ptr->get_remote_address().to_string();

            if (under_handling_data.find(socket_key) != under_handling_data.end())
            {
                return continue_handling(under_handling_data[socket_key], message);
            }

            return start_handling(socket_key, message);
        };

        http_handled_data continue_handling(http_data_under_handling &data, const hamza::data_buffer &message)
        {
            if (data.type == handling_type::CHUNKED)
            {
                // Handle chunked transfer encoding
                std::istringstream chunked_stream(message.to_string());
                std::string chunk_size_line;
                while (std::getline(chunked_stream, chunk_size_line))
                {
                    // Remove carriage return from chunk size line
                    if (!chunk_size_line.empty() && chunk_size_line.back() == '\r')
                    {
                        chunk_size_line.pop_back();
                    }

                    // Check for end of chunks
                    if (chunk_size_line == "0")
                    {
                        // Clean up completed data
                        auto return_value = http_handled_data(true, data.method, data.uri, data.version, data.headers, data.body);
                        under_handling_data.erase(data.socket_key);
                        return return_value;
                    }

                    // Read chunk data
                    std::string chunk_data;
                    std::getline(chunked_stream, chunk_data);
                    data.body += chunk_data;

                    // Read and discard trailing CRLF
                    std::string trailing_crlf;
                    std::getline(chunked_stream, trailing_crlf);
                }
            }

            // handle content length

            std::string body = message.to_string();
            data.body += body;

            if (data.body.size() == data.content_length)
            {
                auto return_value = http_handled_data(true, data.method, data.uri, data.version, data.headers, data.body);
                under_handling_data.erase(data.socket_key);
                return return_value;
            }

            return http_handled_data(false);
        }

        http_handled_data start_handling(const std::string &socket_key, const hamza::data_buffer &message)
        {
            // Convert raw message to string stream for line-by-line parsing

            handling_type http_data_under_handling_type;

            std::istringstream request_stream(message.to_string());

            // HTTP request components to be parsed
            std::string method, uri, version;
            std::multimap<std::string, std::string> headers; // Allows duplicate header names
            std::string line;

            // Parse HTTP request line: "METHOD URI VERSION"
            // Example: "GET /index.html HTTP/1.1"
            if (std::getline(request_stream, line) && !line.empty())
            {
                // Remove carriage return from line ending (CRLF -> LF)
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                // Split request line into method, URI, and version
                std::istringstream request_line(line);
                request_line >> method >> uri >> version;
            }

            // Parse HTTP headers until empty line is encountered
            while (std::getline(request_stream, line))
            {
                // Remove carriage return from each header line
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }

                // Empty line indicates end of headers and start of body
                if (line.empty())
                {
                    break;
                }

                // Parse header in format "Name: Value"
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos)
                {
                    std::string header_name = line.substr(0, colon_pos);
                    std::string header_value = line.substr(colon_pos + 1);

                    // Trim leading whitespace from header value
                    size_t start = header_value.find_first_not_of(" \t");
                    if (start != std::string::npos)
                    {
                        header_value = header_value.substr(start);
                    }

                    // Trim trailing whitespace from header value
                    size_t end = header_value.find_last_not_of(" \t");
                    if (end != std::string::npos)
                    {
                        header_value = header_value.substr(0, end + 1);
                    }

                    // Store header (multimap allows duplicate header names)
                    headers.emplace(hamza::to_upper_case(header_name), header_value);
                }
            }

            std::size_t content_length = 0;
            auto content_length_it = headers.find(hamza::to_upper_case("content-length"));
            if (content_length_it != headers.end())
            {
                content_length = std::stoull(content_length_it->second);
                http_data_under_handling_type = handling_type::CONTENT_LENGTH;

                std::ostringstream body_stream;
                body_stream << request_stream.rdbuf();
                std::string body = body_stream.str();

                if (body.size() == content_length)
                {
                    return http_handled_data(true, method, uri, version, headers, body);
                }
                else
                {
                    under_handling_data.insert({socket_key, http_data_under_handling(socket_key, http_data_under_handling_type)});
                    auto &data_ref = under_handling_data[socket_key];
                    data_ref.content_length = content_length;
                    data_ref.body = body;
                    data_ref.method = method;
                    data_ref.uri = uri;
                    data_ref.version = version;
                    data_ref.headers = headers;

                    return http_handled_data(false);
                }
            }

            auto transfer_encoding = headers.find(hamza::to_upper_case("Transfer-Encoding"));
            if (transfer_encoding != headers.end())
            {
                // Handle chunked transfer encoding
                if (contains_chunked(headers.equal_range(transfer_encoding->first)))
                {
                    http_data_under_handling_type = handling_type::CHUNKED;

                    std::ostringstream chunked_body_stream;
                    std::string chunk_size;
                    while (std::getline(request_stream, chunk_size))
                    {
                        // Remove carriage return from chunk size line
                        if (!chunk_size.empty() && chunk_size.back() == '\r')
                        {
                            chunk_size.pop_back();
                        }

                        // Check for end of chunks
                        if (chunk_size == "0")
                        {
                            break;
                        }

                        // Read chunk data
                        std::string chunk_data;
                        std::getline(request_stream, chunk_data);
                        chunked_body_stream << chunk_data;

                        // Read and discard trailing CRLF
                        std::string trailing_crlf;
                        std::getline(request_stream, trailing_crlf);
                    }

                    if (chunk_size == "0")
                    {
                        return http_handled_data(true, method, uri, version, headers, chunked_body_stream.str());
                    }
                    else
                    {
                        under_handling_data.insert({socket_key, http_data_under_handling(socket_key, http_data_under_handling_type)});
                        auto &data_ref = under_handling_data[socket_key];
                        data_ref.content_length = content_length;
                        data_ref.body = chunked_body_stream.str();
                        data_ref.method = method;
                        data_ref.uri = uri;
                        data_ref.version = version;
                        data_ref.headers = headers;

                        return http_handled_data(false);
                    }
                }
            }

            return http_handled_data(true, method, uri, version, headers);
        }

        bool contains_chunked(const std::pair<std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> &range)
        {
            for (auto it = range.first; it != range.second; ++it)
            {
                auto tmp = it->second;
                std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
                if (tmp.find("chunked") != std::string::npos)
                {
                    return true;
                }
            }
            return false;
        }
    };
}