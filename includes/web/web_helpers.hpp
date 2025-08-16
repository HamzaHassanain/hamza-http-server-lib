#include <string>
#include <sstream>
#include <iomanip>
#include <map>

namespace hamza::web::helpers
{
    // Helper functions for web-related tasks
    std::string url_encode(const std::string &value);
    std::string url_decode(const std::string &value);
    std::map<std::string, std::string> parse_form_data(const std::string &body);
    std::string get_mime_type_from_extension(const std::string &extension);
    
}
