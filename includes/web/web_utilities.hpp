
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace hamza::web
{
    extern const std::vector<std::string> static_extensions;
    extern const std::unordered_map<std::string, std::string> mime_types;
    std::string get_mime_type_from_extension(const std::string &extension);
    std::string get_file_extension_from_mime(const std::string &mime_type);
    std::string get_file_extension_from_uri(const std::string &uri);
    std::string sanitize_path(const std::string &path);
    bool is_uri_static(const std::string &uri);

}
