#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <web/web_utilities.hpp>
namespace hamza::web
{
    const std::vector<std::string> static_extensions = {
        // Web Documents
        "html", "htm", "xhtml", "xml",
        // Stylesheets
        "css", "scss", "sass", "less",
        // JavaScript
        "js", "mjs", "jsx", "ts", "tsx",
        // Images
        "png", "jpg", "jpeg", "gif", "bmp", "tiff", "tif",
        "svg", "webp", "ico", "cur", "avif",
        // Fonts
        "woff", "woff2", "ttf", "otf", "eot",
        // Audio
        "mp3", "wav", "ogg", "m4a", "aac", "flac",
        // Video
        "mp4", "webm", "avi", "mov", "wmv", "flv", "mkv",
        // Documents
        "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "txt", "rtf", "odt", "ods", "odp",
        // Archives
        "zip", "rar", "7z", "tar", "gz", "bz2",
        // Data formats
        "json", "csv", "yaml", "yml", "toml",
        // Web Manifests & Config
        "manifest", "webmanifest", "map", "htaccess",
        // Other common formats
        "swf", "eps", "ai", "psd", "sketch"};

    const std::unordered_map<std::string, std::string> mime_types = {
        // Web Documents
        {"html", "text/html"},
        {"htm", "text/html"},
        {"xhtml", "application/xhtml+xml"},
        {"xml", "application/xml"},

        // Stylesheets
        {"css", "text/css"},
        {"scss", "text/x-scss"},
        {"sass", "text/x-sass"},
        {"less", "text/x-less"},

        // JavaScript
        {"js", "application/javascript"},
        {"mjs", "application/javascript"},
        {"jsx", "text/jsx"},
        {"ts", "application/typescript"},
        {"tsx", "text/tsx"},

        // Images
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
        {"tiff", "image/tiff"},
        {"tif", "image/tiff"},
        {"svg", "image/svg+xml"},
        {"webp", "image/webp"},
        {"ico", "image/x-icon"},
        {"cur", "image/x-icon"},
        {"avif", "image/avif"},

        // Fonts
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"otf", "font/otf"},
        {"eot", "application/vnd.ms-fontobject"},

        // Audio
        {"mp3", "audio/mpeg"},
        {"wav", "audio/wav"},
        {"ogg", "audio/ogg"},
        {"m4a", "audio/mp4"},
        {"aac", "audio/aac"},
        {"flac", "audio/flac"},

        // Video
        {"mp4", "video/mp4"},
        {"webm", "video/webm"},
        {"avi", "video/x-msvideo"},
        {"mov", "video/quicktime"},
        {"wmv", "video/x-ms-wmv"},
        {"flv", "video/x-flv"},
        {"mkv", "video/x-matroska"},

        // Documents
        {"pdf", "application/pdf"},
        {"doc", "application/msword"},
        {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {"xls", "application/vnd.ms-excel"},
        {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {"ppt", "application/vnd.ms-powerpoint"},
        {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        {"txt", "text/plain"},
        {"rtf", "application/rtf"},
        {"odt", "application/vnd.oasis.opendocument.text"},
        {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
        {"odp", "application/vnd.oasis.opendocument.presentation"},

        // Archives
        {"zip", "application/zip"},
        {"rar", "application/vnd.rar"},
        {"7z", "application/x-7z-compressed"},
        {"tar", "application/x-tar"},
        {"gz", "application/gzip"},
        {"bz2", "application/x-bzip2"},

        // Data formats
        {"json", "application/json"},
        {"csv", "text/csv"},
        {"yaml", "application/x-yaml"},
        {"yml", "application/x-yaml"},
        {"toml", "application/toml"},

        // Web Manifests & Config
        {"manifest", "text/cache-manifest"},
        {"webmanifest", "application/manifest+json"},
        {"map", "application/json"},
        {"htaccess", "text/plain"},

        // Other common formats
        {"swf", "application/x-shockwave-flash"},
        {"eps", "application/postscript"},
        {"ai", "application/postscript"},
        {"psd", "image/vnd.adobe.photoshop"},
        {"sketch", "application/x-sketch"}};

    std::string get_mime_type_from_extension(const std::string &extension)
    {
        auto it = mime_types.find(extension);
        if (it != mime_types.end())
        {
            return it->second;
        }
        return "application/octet-stream";
    }
    std::string get_file_extension_from_mime(const std::string &mime_type)
    {
        for (const auto &pair : mime_types)
        {
            if (pair.second == mime_type)
            {
                return pair.first;
            }
        }
        return "";
    }
    std::string get_file_extension_from_uri(const std::string &uri)
    {
        size_t dot_pos = uri.find_last_of('.');
        if (dot_pos != std::string::npos)
        {
            return uri.substr(dot_pos + 1);
        }
        return "";
    }
    std::string sanitize_path(const std::string &path)
    {
        std::string sanitized = path;
        // Remove any potentially dangerous sequences
        // For example, remove ".." to prevent directory traversal
        size_t pos;
        while ((pos = sanitized.find("..")) != std::string::npos)
        {
            sanitized.erase(pos, 2);
        }
        return sanitized;
    }
    bool is_uri_static(const std::string &uri)
    {
        // Check if the URI has a static file extension
        std::string extension = get_file_extension_from_uri(uri);
        return std::find(static_extensions.begin(), static_extensions.end(), extension) != static_extensions.end();
    }

}
