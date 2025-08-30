#pragma once

#include "../libs/socket-lib/socket-lib.hpp"

#include "http_handled_data.hpp"
#include "http_data_under_handling.hpp"
#include "http_consts.hpp"
#include <memory>
#include <map>
#include <sstream>
#include <mutex>
#include <functional>
namespace hh_http
{

    class http_message_handler
    {
        std::map<std::string, http_data_under_handling> under_handling_data;
        std::mutex mtx;

    public:
        http_handled_data handle(std::shared_ptr<hh_socket::connection> conn, const hh_socket::data_buffer &message)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto socket_key = conn->get_remote_address().to_string();

            if (under_handling_data.find(socket_key) != under_handling_data.end())
            {
                return continue_handling(under_handling_data[socket_key], message);
            }

            return start_handling(socket_key, message, conn->get_fd());
        }

        http_handled_data continue_handling(http_data_under_handling &data, const hh_socket::data_buffer &message)
        {
            data.last_activity = std::chrono::steady_clock::now();

            if (data.type == handling_type::CHUNKED)
            {
                return continue_chunked_handling(data, message);
            }
            else // Content-Length handling
            {
                return continue_content_length_handling(data, message);
            }
        }

        http_handled_data start_handling(const std::string &socket_key, const hh_socket::data_buffer &message, int FD)
        {
            // Convert raw message to string stream for line-by-line parsing
            std::istringstream request_stream(message.to_string());

            // HTTP request components to be parsed
            std::string method, uri, version;

            // Parse request line
            auto [request_line_valid, error_message] = parse_request_line(request_stream, method, uri, version);
            if (!request_line_valid)
            {
                return http_handled_data(true, error_message, uri, version, {}, "");
            }

            // Parse headers
            auto [headers_valid, headers] = parse_headers(request_stream, uri, version);
            if (!headers_valid)
            {
                return http_handled_data(true, "BAD_HEADERS_TOO_LARGE", uri, version, {}, "");
            }

            // Check for Transfer-Encoding and Content-Length headers
            std::size_t content_length = 0;
            auto content_length_it = headers.find(hh_socket::to_upper_case("content-length"));
            auto transfer_encoding = headers.find(hh_socket::to_upper_case("Transfer-Encoding"));

            bool has_transfer_encoding = (transfer_encoding != headers.end()) &&
                                         contains_chunked(headers.equal_range(hh_socket::to_upper_case("Transfer-Encoding")));

            bool has_content_length = (content_length_it != headers.end());

            // Validate headers combination
            if (headers.count(hh_socket::to_upper_case("content-length")) > 1 ||
                (has_content_length && has_transfer_encoding))
            {
                return http_handled_data(true, "BAD_REPEATED_LENGTH_OR_TRANSFER_ENCODING_OR_BOTH", uri, version, headers, "");
            }

            // Handle body based on headers
            if (has_content_length)
            {
                content_length = std::stoull(content_length_it->second);
                return handle_content_length(socket_key, request_stream, method, uri, version, headers, content_length, FD);
            }
            else if (has_transfer_encoding)
            {
                return handle_chunked_encoding(socket_key, request_stream, method, uri, version, headers, FD);
            }

            // No body to process
            return http_handled_data(true, method, uri, version, headers, "");
        }

        void cleanup_idle_connections(std::chrono::seconds max_idle_time, std::function<void(int)> close_connection)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto now = std::chrono::steady_clock::now();
            for (auto it = under_handling_data.begin(); it != under_handling_data.end();)
            {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_activity);
                if (duration > max_idle_time)
                {
                    // std::cout << "❌ idle-timeout-exceeded@ " << it->first << " FD=" << it->second.FD << std::endl;
                    close_connection(it->second.FD);
                    it = under_handling_data.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

    private:
        // Helper method to parse request line
        std::pair<bool, std::string> parse_request_line(std::istringstream &request_stream,
                                                        std::string &method,
                                                        std::string &uri,
                                                        std::string &version)
        {
            std::string line;
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

            if (method.empty() || uri.empty() || version.empty())
            {
                return {false, "BAD_METHOD_OR_URI_OR_VERSION"};
            }

            return {true, ""};
        }

        // Helper method to parse headers
        std::pair<bool, std::multimap<std::string, std::string>> parse_headers(std::istringstream &request_stream,
                                                                               const std::string &uri,
                                                                               const std::string &version)
        {
            std::multimap<std::string, std::string> headers;
            std::size_t headers_size = 0;
            std::string line;

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
                    headers_size += header_name.size() + header_value.size();

                    // Check for header size limits
                    if (headers_size > config::MAX_HEADER_SIZE)
                    {
                        return {false, {}};
                    }
                    // Store header (multimap allows duplicate header names)
                    headers.emplace(hh_socket::to_upper_case(header_name), header_value);
                }
            }

            return {true, headers};
        }

        // Helper method to check if "chunked" is present in Transfer-Encoding header
        bool contains_chunked(const std::pair<std::multimap<std::string, std::string>::iterator,
                                              std::multimap<std::string, std::string>::iterator> &range)
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

        // Handle content-length based body
        http_handled_data handle_content_length(const std::string &socket_key,
                                                std::istringstream &request_stream,
                                                const std::string &method,
                                                const std::string &uri,
                                                const std::string &version,
                                                const std::multimap<std::string, std::string> &headers,
                                                size_t content_length,
                                                int FD)
        {
            // Read the body from the stream
            std::ostringstream body_stream;
            body_stream << request_stream.rdbuf();
            std::string body = body_stream.str();

            // Complete request in one go
            if (body.size() == content_length)
            {
                return http_handled_data(true, method, uri, version, headers, body);
            }
            else if (body.size() > content_length || body.size() > config::MAX_BODY_SIZE)
            {
                return http_handled_data(true, "BAD_CONTENT_TOO_LARGE", uri, version, headers, "");
            }
            else
            {
                // Need to continue handling in subsequent calls
                under_handling_data.insert({socket_key, http_data_under_handling(socket_key, handling_type::CONTENT_LENGTH)});
                auto &data_ref = under_handling_data[socket_key];
                data_ref.content_length = content_length;
                data_ref.body = body;
                data_ref.method = method;
                data_ref.uri = uri;
                data_ref.version = version;
                data_ref.headers = headers;
                data_ref.last_activity = std::chrono::steady_clock::now();
                data_ref.FD = FD;
                return http_handled_data(false, method, uri, version, headers, body);
            }
        }

        // Handle chunked encoding body
        http_handled_data handle_chunked_encoding(const std::string &socket_key,
                                                  std::istringstream &request_stream,
                                                  const std::string &method,
                                                  const std::string &uri,
                                                  const std::string &version,
                                                  const std::multimap<std::string, std::string> &headers,
                                                  int FD)
        {
            std::ostringstream chunked_body_stream;
            std::string chunk_size_line;
            bool complete = false;

            // Process chunks from the initial request
            while (std::getline(request_stream, chunk_size_line))
            {
                // Remove carriage return from chunk size line
                if (!chunk_size_line.empty() && chunk_size_line.back() == '\r')
                {
                    chunk_size_line.pop_back();
                }

                // Validate chunk size line is not empty
                if (chunk_size_line.empty())
                {
                    return http_handled_data(true, "BAD_CHUNK_ENCODING", uri, version, headers, "");
                }

                // Extract the actual size (ignore chunk extensions after semicolon)
                std::string chunk_size_hex = chunk_size_line;
                size_t semicolon_pos = chunk_size_hex.find(';');
                if (semicolon_pos != std::string::npos)
                {
                    chunk_size_hex = chunk_size_hex.substr(0, semicolon_pos);
                }

                // Validate chunk size hex format
                for (char c : chunk_size_hex)
                {
                    if (!isxdigit(c))
                    {
                        return http_handled_data(true, "BAD_CHUNK_ENCODING", uri, version, headers, "");
                    }
                }

                // Convert hex string to integer
                std::istringstream hex_stream(chunk_size_hex);
                unsigned int chunk_size_int = 0;
                if (!(hex_stream >> std::hex >> chunk_size_int))
                {
                    return http_handled_data(true, "BAD_CHUNK_ENCODING", uri, version, headers, "");
                }

                // Check for end of chunks
                if (chunk_size_int == 0)
                {
                    complete = true;
                    break;
                }

                // Prevent unreasonable chunk sizes that could cause memory issues
                if (chunk_size_int > config::MAX_BODY_SIZE)
                {
                    return http_handled_data(true, "BAD_CONTENT_TOO_LARGE", uri, version, headers, "");
                }

                // Read the exact number of bytes specified by chunk size
                char *chunk_buffer = new char[chunk_size_int + 2]; // +2 for CRLF

                // Check if we have enough data
                if (!request_stream.read(chunk_buffer, chunk_size_int + 2))
                {
                    delete[] chunk_buffer;
                    break; // Not enough data, will need to continue later
                }

                // Validate CRLF after chunk data
                if (chunk_buffer[chunk_size_int] != '\r' || chunk_buffer[chunk_size_int + 1] != '\n')
                {
                    delete[] chunk_buffer;
                    return http_handled_data(true, "BAD_CHUNK_ENCODING", uri, version, headers, "");
                }

                // Only add the actual data (without the trailing CRLF)
                chunked_body_stream.write(chunk_buffer, chunk_size_int);

                delete[] chunk_buffer;

                // Check if we've exceeded max body size
                if (chunked_body_stream.str().size() > config::MAX_BODY_SIZE)
                {
                    return http_handled_data(true, "BAD_CONTENT_TOO_LARGE", uri, version, headers, "");
                }
            }

            // Request is complete
            if (complete)
            {
                // After the final "0" chunk, there should be an empty line or trailer headers
                std::string trailer_line;
                std::multimap<std::string, std::string> trailer_headers;

                // Process trailer headers if any
                while (std::getline(request_stream, trailer_line))
                {
                    // Remove carriage return
                    if (!trailer_line.empty() && trailer_line.back() == '\r')
                    {
                        trailer_line.pop_back();
                    }

                    // Empty line indicates end of trailer headers
                    if (trailer_line.empty())
                    {
                        break;
                    }

                    // Parse trailer header
                    size_t colon_pos = trailer_line.find(':');
                    if (colon_pos != std::string::npos)
                    {
                        std::string header_name = trailer_line.substr(0, colon_pos);
                        std::string header_value = trailer_line.substr(colon_pos + 1);

                        // Trim leading whitespace
                        size_t start = header_value.find_first_not_of(" \t");
                        if (start != std::string::npos)
                        {
                            header_value = header_value.substr(start);
                        }

                        // Trim trailing whitespace
                        size_t end = header_value.find_last_not_of(" \t");
                        if (end != std::string::npos)
                        {
                            header_value = header_value.substr(0, end + 1);
                        }

                        // Add to headers
                        trailer_headers.emplace(hh_socket::to_upper_case(header_name), header_value);
                    }
                    else
                    {
                        // Invalid trailer header format
                        return http_handled_data(true, "BAD_TRAILER_HEADERS", uri, version, headers, "");
                    }
                }

                // Merge trailer headers with original headers
                // std::multimap<std::string, std::string> combined_headers = headers;
                // for (const auto &trailer : trailer_headers)
                // {
                //     combined_headers.insert(trailer);
                // }

                return http_handled_data(true, method, uri, version, headers, chunked_body_stream.str());
            }
            else
            {
                // Need to continue handling in subsequent calls
                under_handling_data.insert({socket_key, http_data_under_handling(socket_key, handling_type::CHUNKED)});
                auto &data_ref = under_handling_data[socket_key];
                data_ref.content_length = 0; // Not relevant for chunked
                data_ref.body = chunked_body_stream.str();
                data_ref.method = method;
                data_ref.uri = uri;
                data_ref.version = version;
                data_ref.headers = headers;
                data_ref.FD = FD;

                data_ref.last_activity = std::chrono::steady_clock::now();
                return http_handled_data(false, method, uri, version, headers, chunked_body_stream.str());
            }
        }

        // Continue processing chunked encoding for partial requests
        http_handled_data continue_chunked_handling(http_data_under_handling &data,
                                                    const hh_socket::data_buffer &message)
        {
            std::istringstream chunked_stream(message.to_string());
            std::string chunk_size_line;
            bool complete = false;

            // Process additional chunks
            while (std::getline(chunked_stream, chunk_size_line))
            {
                // Remove carriage return from chunk size line
                if (!chunk_size_line.empty() && chunk_size_line.back() == '\r')
                {
                    chunk_size_line.pop_back();
                }

                // Validate chunk size line is not empty
                if (chunk_size_line.empty())
                {
                    return http_handled_data(true, "BAD_CHUNK_ENCODING", data.uri, data.version, data.headers, "");
                }

                // Extract the actual size (ignore chunk extensions after semicolon)
                std::string chunk_size_hex = chunk_size_line;
                size_t semicolon_pos = chunk_size_hex.find(';');
                if (semicolon_pos != std::string::npos)
                {
                    chunk_size_hex = chunk_size_hex.substr(0, semicolon_pos);
                }

                // Validate chunk size hex format
                for (char c : chunk_size_hex)
                {
                    if (!isxdigit(c))
                    {
                        return http_handled_data(true, "BAD_CHUNK_ENCODING", data.uri, data.version, data.headers, "");
                    }
                }

                // Convert hex string to integer
                std::istringstream hex_stream(chunk_size_hex);
                unsigned int chunk_size_int = 0;
                if (!(hex_stream >> std::hex >> chunk_size_int))
                {
                    return http_handled_data(true, "BAD_CHUNK_ENCODING", data.uri, data.version, data.headers, "");
                }

                // Check for end of chunks
                if (chunk_size_int == 0)
                {
                    complete = true;
                    break;
                }

                // Prevent unreasonable chunk sizes that could cause memory issues
                if (chunk_size_int > config::MAX_BODY_SIZE)
                {
                    return http_handled_data(true, "CONTENT_TOO_LARGE", data.uri, data.version, data.headers, "");
                }

                // Read the exact number of bytes specified by chunk size
                char *chunk_buffer = new char[chunk_size_int + 2]; // +2 for CRLF

                // Check if we have enough data
                if (!chunked_stream.read(chunk_buffer, chunk_size_int + 2))
                {
                    delete[] chunk_buffer;
                    break; // Not enough data, will need to continue later
                }

                // Validate CRLF after chunk data
                if (chunk_buffer[chunk_size_int] != '\r' || chunk_buffer[chunk_size_int + 1] != '\n')
                {
                    delete[] chunk_buffer;
                    return http_handled_data(true, "BAD_CHUNK_ENCODING", data.uri, data.version, data.headers, "");
                }

                // Only add the actual data (without the trailing CRLF)
                data.body.append(chunk_buffer, chunk_size_int);

                delete[] chunk_buffer;

                if (data.body.size() > config::MAX_BODY_SIZE)
                {
                    return http_handled_data(true, "CONTENT_TOO_LARGE", data.uri, data.version, data.headers, "");
                }
            }

            // Request is complete
            if (complete)
            {
                // After the final "0" chunk, there should be an empty line or trailer headers
                std::string trailer_line;
                std::multimap<std::string, std::string> trailer_headers;

                // Process trailer headers if any
                while (std::getline(chunked_stream, trailer_line))
                {
                    // Remove carriage return
                    if (!trailer_line.empty() && trailer_line.back() == '\r')
                    {
                        trailer_line.pop_back();
                    }

                    // Empty line indicates end of trailer headers
                    if (trailer_line.empty())
                    {
                        break;
                    }

                    // Parse trailer header
                    size_t colon_pos = trailer_line.find(':');
                    if (colon_pos != std::string::npos)
                    {
                        std::string header_name = trailer_line.substr(0, colon_pos);
                        std::string header_value = trailer_line.substr(colon_pos + 1);

                        // Trim leading whitespace
                        size_t start = header_value.find_first_not_of(" \t");
                        if (start != std::string::npos)
                        {
                            header_value = header_value.substr(start);
                        }

                        // Trim trailing whitespace
                        size_t end = header_value.find_last_not_of(" \t");
                        if (end != std::string::npos)
                        {
                            header_value = header_value.substr(0, end + 1);
                        }

                        // Add to headers
                        trailer_headers.emplace(hh_socket::to_upper_case(header_name), header_value);
                    }
                    else
                    {
                        // Invalid trailer header format
                        return http_handled_data(true, "BAD_TRAILER_HEADERS", data.uri, data.version, data.headers, "");
                    }
                }
                // just Ignore Trailer Headers for now
                // Clean up completed data
                auto return_value = http_handled_data(true, data.method, data.uri, data.version, data.headers, data.body);
                under_handling_data.erase(data.socket_key);
                return return_value;
            }

            return http_handled_data(false, data.method, data.uri, data.version, data.headers, data.body);
        }

        // Continue processing content-length for partial requests
        http_handled_data continue_content_length_handling(http_data_under_handling &data,
                                                           const hh_socket::data_buffer &message)
        {
            // Add new data to existing body
            std::string body = message.to_string();
            data.body += body;

            if (data.body.size() > config::MAX_BODY_SIZE)
            {
                return http_handled_data(true, "BAD_CONTENT_TOO_LARGE", data.uri, data.version, data.headers, "");
            }
            // Check if we've received all expected data
            if (data.body.size() == data.content_length)
            {
                auto return_value = http_handled_data(true, data.method, data.uri, data.version, data.headers, data.body);
                under_handling_data.erase(data.socket_key);
                return return_value;
            }

            // Check for errors: too much data or exceeding size limits
            if (data.body.size() > data.content_length || data.body.size() > config::MAX_BODY_SIZE)
            {
                return http_handled_data(true, "BAD_CONTENT_TOO_LARGE", data.uri, data.version, data.headers, "");
            }

            // Still waiting for more data
            return http_handled_data(false, data.method, data.uri, data.version, {}, "");
        }
    };
}
