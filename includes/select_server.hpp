#include <memory>
#include <socket.hpp>
#include <file_descriptor.hpp>
#include <mutex>

namespace hamza
{
    class select_server
    {
    private:
        fd_set master_fds;
        fd_set read_fds;
        int max_fds;
        int max_fd;
        struct timeval timeout;

        std::mutex mtx;

    public:
        void init(const file_descriptor &);
        void set_max_fd(int max_fd);
        void set_timeout(int seconds);
        void remove_fd(const file_descriptor &fd);
        void add_fd(const file_descriptor &fd);
        bool is_fd_set(const file_descriptor &fd);
        int select();
    };
};