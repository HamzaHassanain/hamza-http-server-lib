#include <http_objects.hpp>

namespace hamza
{
    namespace http
    {
        // ========== http_request Implementation ==========

        http_request::http_request(const std::string &method, const std::string &uri, const std::string &version,
                                   const std::map<std::string, std::string> &headers, const std::string &body,
                                   std::shared_ptr<hamza::socket> client_socket)
            : method(method), uri(uri), version(version), headers(headers), body(body), client_socket(client_socket) {}

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
            body.clear();
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

        std::string http_request::get_header(const std::string &name) const
        {
            auto it = headers.find(name);
            if (it != headers.end())
            {
                return it->second;
            }
            return "";
        }

        std::vector<std::pair<std::string, std::string>> http_request::get_headers() const
        {
            std::vector<std::pair<std::string, std::string>> headers_vector;
            for (const auto &header : headers)
            {
                headers_vector.emplace_back(header.first, header.second);
            }
            return headers_vector;
        }

        std::string http_request::get_body() const
        {
            return body;
        }

        // ========== http_response Implementation ==========

        http_response::http_response(const std::string &version, const std::map<std::string, std::string> &headers,
                                     std::shared_ptr<hamza::socket> client_socket)
            : version(version), headers(headers), client_socket(client_socket) {}

        bool http_response::validate() const
        {
            if (!client_socket)
            {
                return false;
            }
            if (status_code < 100 || status_code > 599)
            {
                return false;
            }
            if (version != hamza::http::versions::HTTP_1_1 && version != hamza::http::versions::HTTP_2_0)
            {
                return false;
            }
            if (headers.find(hamza::http::headers::CONTENT_TYPE) == headers.end())
            {
                return false;
            }
            return true;
        }

        std::string http_response::to_string() const
        {
            std::ostringstream response_stream;
            response_stream << version << " " << status_code << " " << status_message << "\r\n";
            for (const auto &header : headers)
            {
                response_stream << header.first << ": " << header.second << "\r\n";
            }
            response_stream << "\r\n"
                            << body;

            for (const auto &trailer : trailers)
            {
                response_stream << trailer.first << ": " << trailer.second << "\r\n";
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
            trailers[name] = value;
        }

        void http_response::add_header(const std::string &name, const std::string &value)
        {
            headers[name] = value;
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

        std::string http_response::get_header(const std::string &name) const
        {
            auto it = headers.find(name);
            if (it != headers.end())
            {
                return it->second;
            }
            return "";
        }

        std::string http_response::get_trailer(const std::string &name) const
        {
            auto it = trailers.find(name);
            if (it != trailers.end())
            {
                return it->second;
            }
            return "";
        }

        void http_response::end()
        {
            if (validate())
            {
                client_socket->send_on_connection(hamza::data_buffer(to_string()));
                close_connection(client_socket);
            }
            else
            {
                std::string message = "\nMake sure that the following headers are set and correct: ";
                message += hamza::http::headers::CONTENT_LENGTH + ", " + hamza::http::headers::CONTENT_TYPE + ", and the status code is between 100 and 599.\n";
                throw std::runtime_error(message);
            }
        }
    }
}