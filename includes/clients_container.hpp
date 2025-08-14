#pragma once

#include <socket_address.hpp>
#include <socket.hpp>
#include <thread>
#include <set>
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
        std::mutex mtx;

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
            std::cout << "Before erase size: " << sockets.size() << std::endl;
            for (; it != sockets.end(); ++it)
            {
                if ((*it)->get_file_descriptor() == sock->get_file_descriptor())
                {
                    sockets.erase(it);
                    break;
                }
            }

            std::cout << "After erase size: " << sockets.size() << std::endl;

            // throw std::runtime_error("Socket not found");
        }

        bool contains(std::shared_ptr<hamza::socket> sock) const
        {
            return std::find(sockets.begin(), sockets.end(), sock) != sockets.end();
        }
        size_t size() const
        {
            return sockets.size();
        }
        void clear()
        {
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

            for (auto x : sockets)
            {
                if (x == nullptr || !x->is_connected())
                {
                    throw std::runtime_error("Invalid socket pointer in clients container.");
                }
            }
        }
        std::vector<std::shared_ptr<hamza::socket>>::const_iterator begin() const
        {
            return sockets.begin();
        }
        std::vector<std::shared_ptr<hamza::socket>>::const_iterator end() const
        {
            return sockets.end();
        }

        int max() const
        {
            if (sockets.empty())
            {
                return -1;
            }
            int max_fd = -1;
            for (const auto &sock : sockets)
            {
                max_fd = std::max(max_fd, sock->get_file_descriptor().get());
            }
            return max_fd;
        }
    };
};