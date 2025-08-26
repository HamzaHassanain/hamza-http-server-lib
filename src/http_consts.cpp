#include "../includes/http_consts.hpp"

namespace hh_http
{
    namespace epoll_config
    {
        /// @brief Maximum number of pending connections
        int BACKLOG_SIZE = 1024 * 1024;

        /// @brief Maximum number of open file descriptors
        int MAX_FILE_DESCRIPTORS = 1024 * 32;

        /// @brief Maximum timeout for connections (in milliseconds)
        int TIMEOUT_MILLISECONDS = 1000;

    }
    namespace config
    {
        /// @brief Maximum idle time for connections before cleanup (in seconds)
        std::chrono::seconds MAX_IDLE_TIME_SECONDS = std::chrono::seconds(5);
        /// @brief Maximum size of HTTP headers (in bytes)
        size_t MAX_HEADER_SIZE = 1024 * 16;
        /// @brief Maximum size of HTTP body (in bytes)
        size_t MAX_BODY_SIZE = 1024 * 1024 * 5; // 5 MB

    }

    /// @brief Convert string to uppercase.
    /// @param input String to convert
    /// @note does not modify the original string, returns a new uppercase string
    /// @return Uppercase version of input string
    std::string to_upper_case(const std::string &input)
    {
        std::string upper_case_str = input;
        std::transform(upper_case_str.begin(), upper_case_str.end(), upper_case_str.begin(),
                       [](unsigned char c)
                       { return std::toupper(c); });
        return upper_case_str;
    }
}