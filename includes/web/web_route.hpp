#pragma once
#include <string>
#include <vector>
#include <web/web_types.hpp>
#include <web/web_router.hpp>
#include <web/web_exceptions.hpp>
namespace hamza::web
{

    class web_route
    {
        std::string path;
        std::string method;
        std::vector<web_request_handler_t> handlers;
        bool match_path(const std::string &rhs) const
        {
            try
            {

                if (path == rhs)
                {
                    return true;
                }

                int pos = 0;
                int rhs_pos = 0;
                while (pos < path.length() && rhs_pos < rhs.length())
                {
                    if (path[pos] == ':')
                    {
                        pos = path.find('/', pos);
                        rhs_pos = rhs.find('/', rhs_pos);
                        if (pos == std::string::npos || rhs_pos == std::string::npos)
                            return true;
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
            catch (const std::exception &e)
            {
                throw web_general_exception("Error matching path: " + std::string(e.what()));
            }
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