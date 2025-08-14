#pragma once

#include <string>
#include <vector>
#include <utilities.hpp>
namespace hamza
{

    class data_buffer
    {
    private:
        std::vector<char> buffer;

    public:
        data_buffer() = default;

        // Constructor to create buffer from string
        explicit data_buffer(const std::string &str) : buffer(str.begin(), str.end()) {}

        // Constructor to create buffer from raw data
        data_buffer(const char *data, std::size_t size) : buffer(data, data + size) {}

        void append(const char *data, std::size_t size)
        {
            buffer.insert(buffer.end(), data, data + size);
        }

        void append(const std::string &str)
        {
            buffer.insert(buffer.end(), str.begin(), str.end());
        }

        const char *data() const
        {
            return buffer.data();
        }

        std::size_t size() const
        {
            return buffer.size();
        }

        bool empty() const
        {
            return buffer.empty();
        }

        void clear()
        {
            buffer.clear();
        }

        // Convert to string (useful for text data)
        std::string to_string() const
        {
            return std::string(buffer.begin(), buffer.end());
        }
    };

};