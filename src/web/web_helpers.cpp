#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <web/web_helpers.hpp>
#include <web/web_utilities.hpp>
namespace hamza::web::helpers
{
    // Helper functions for web-related tasks
    std::string url_encode(const std::string &value)
    {
        std::ostringstream escaped;
        for (const char &c : value)
        {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else
            {
                escaped << '%' << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(c);
            }
        }
        return escaped.str();
    }

    std::string url_decode(const std::string &value)
    {
        std::string decoded;
        for (size_t i = 0; i < value.length(); ++i)
        {
            if (value[i] == '%')
            {
                if (i + 2 < value.length())
                {
                    std::string hex = value.substr(i + 1, 2);
                    char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                    decoded += decoded_char;
                    i += 2;
                }
            }
            else
            {
                decoded += value[i];
            }
        }
        return decoded;
    }

    std::map<std::string, std::string> parse_form_data(const std::string &body)
    {

        std::map<std::string, std::string> form_data;

        std::istringstream stream(body);
        std::string pair;

        size_t start = 0;
        size_t end = 0;

        while (end != std::string::npos)
        {
            end = body.find('&', start);
            std::string pair = body.substr(start, (end == std::string::npos) ? std::string::npos : end - start);

            // Parse each key=value pair
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string::npos)
            {
                std::string key = pair.substr(0, eq_pos);
                std::string value = pair.substr(eq_pos + 1);

                // URL decode if needed (basic version)
                // You might want to add proper URL decoding here
                form_data[key] = value;
            }

            start = (end == std::string::npos) ? std::string::npos : end + 1;
        }
        return form_data;
    }

    std::string get_mime_type_from_extension(const std::string &extension)
    {
        return hamza::web::get_mime_type_from_extension(extension);
    }

}
