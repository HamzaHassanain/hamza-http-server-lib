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
                std::cout << "path: " << path << " rhs: " << rhs << std::endl;
                while (pos < path.length() && rhs_pos < rhs.length())
                {
                    if (path[pos] == ':')
                    {
                        pos = path.find('/', pos);
                        rhs_pos = rhs.find('/', rhs_pos);
                        std::cout << pos << " " << rhs_pos << std::endl;
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
                std::cout << "Path matched successfully!" << std::endl;
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

        web_route(const web_route &) = delete;
        web_route &operator=(const web_route &) = delete;

        web_route(web_route &&) = default;
        web_route &operator=(web_route &&) = default;

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