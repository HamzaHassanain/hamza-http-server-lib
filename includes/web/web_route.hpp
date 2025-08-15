#pragma once
#include <string>
#include <vector>
#include <web/web_types.hpp>
#include <web/web_router.hpp>
namespace hamza::web
{

    class web_route
    {
        std::string path;
        std::string method;
        std::vector<web_request_handler_t> handlers;
        bool match_path(const std::string &rhs) const
        {
            // if path is path/:id/something, then rhs = path/123/something should match,

            if (path == rhs)
            {
                return true;
            }

            // Check for dynamic segments
            size_t pos = 0;
            size_t rhs_pos = 0;
            while (pos < path.length() && rhs_pos < rhs.length())
            {
                if (path[pos] == ':')
                {
                    // Skip dynamic segment in both paths
                    pos = path.find('/', pos);
                    rhs_pos = rhs.find('/', rhs_pos);
                    if (pos == std::string::npos || rhs_pos == std::string::npos)
                        break;
                }
                else if (path[pos] != rhs[rhs_pos])
                {
                    return false;
                }
                ++pos;
                ++rhs_pos;
            }
            return pos == path.length() && rhs_pos == rhs.length();
        }

    public:
        friend class web_router;
        web_route(const std::string &path, const std::string &method, const std::vector<web_request_handler_t> &handlers)
            : path(path), method(method), handlers(std::move(handlers))
        {
            if (this->handlers.size() == 0)
            {
                throw std::invalid_argument("At least one handler must be provided");
            }
        }

        std::string get_path() const
        {
            return path;
        }

        std::string get_method() const
        {
            return method;
        }

        bool match(const std::string &path, const std::string &method) const
        {
            return this->method == method && match_path(path);
        }
    };
};