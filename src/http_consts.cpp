#include <http_consts.hpp>

namespace hamza_http
{
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