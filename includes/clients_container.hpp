#pragma once

#include <socket_address.hpp>
#include <socket.hpp>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>

namespace hamza
{
    struct clients_container
    {
    private:
        std::vector<std::shared_ptr<hamza::socket>> sockets;
        using callback = std::function<void(std::shared_ptr<hamza::socket>)>;
        mutable std::mutex mtx;

    public:
        void insert(std::shared_ptr<hamza::socket> sock)
        {
            std::lock_guard<std::mutex> lock(mtx);
            sockets.push_back(sock);
        }

        void erase(std::shared_ptr<hamza::socket> sock)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sockets.begin();
            for (; it != sockets.end(); ++it)
            {
                if (!(*it))
                    continue;
                if ((*it)->get_file_descriptor() == sock->get_file_descriptor())
                {
                    sockets.erase(it);
                    break;
                }
            }
        }

        bool contains(std::shared_ptr<hamza::socket> sock) const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return std::find(sockets.begin(), sockets.end(), sock) != sockets.end();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return sockets.size();
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(mtx);
            sockets.clear();
        }

        void cleanup()
        {
            std::lock_guard<std::mutex> lock(mtx);
            sockets.erase(std::remove_if(sockets.begin(), sockets.end(),
                                         [](const std::shared_ptr<hamza::socket> &sock)
                                         {
                                             return !sock || !sock->is_connected();
                                         }),
                          sockets.end());
        }

        void for_each(std::function<void(std::shared_ptr<hamza::socket>)> func) const
        {
            std::vector<std::shared_ptr<hamza::socket>> copy;
            {
                std::lock_guard<std::mutex> lock(mtx);
                copy = sockets; // Copy under lock
            }
            for (const auto &sock : copy)
            {
                if (sock)
                    func(sock);
            }
        }

        int max() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (sockets.empty())
            {
                return -1;
            }
            int max_fd = -1;
            for (const auto &sock : sockets)
            {
                if (!sock)
                    continue;
                max_fd = std::max(max_fd, sock->get_file_descriptor().get());
            }
            return max_fd;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return sockets.empty();
        }
    };
}