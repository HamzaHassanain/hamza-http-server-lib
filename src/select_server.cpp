#include <select_server.hpp>

namespace hamza
{

    void select_server::init(const file_descriptor &server_socket_fd)
    {
        FD_ZERO(&master_fds);
        FD_ZERO(&read_fds);
        FD_SET(server_socket_fd.get(), &master_fds);
        set_max_fd(server_socket_fd.get());
    }
    void select_server::set_timeout(int seconds)
    {
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
    }

    void select_server::add_fd(const file_descriptor &fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        FD_SET(fd.get(), &master_fds);
    }

    void select_server::remove_fd(const file_descriptor &fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        FD_CLR(fd.get(), &master_fds);
    }

    int select_server::select()
    {
        std::lock_guard<std::mutex> lock(mtx);

        read_fds = master_fds; // Copy master_fds to read_fds for select
        // timeout = {0, 0};      // Reset timeout to zero for non-blocking select
        return ::select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    }

    void select_server::set_max_fd(int max_fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        this->max_fd = max_fd;
    }

    bool select_server::is_fd_set(const file_descriptor &fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        return FD_ISSET(fd.get(), &read_fds);
    }

};