#include <bits/stdc++.h>
#include "web/web.hpp"

hamza::web::listen_success_callback_t listen_success_callback = []() -> void
{
    std::cout << "Server is listening -^- " << std::endl;
};

hamza::web::error_callback_t error_callback = [](std::unique_ptr<hamza::general_socket_exception> e) -> void
{
    std::cerr << "Error occurred: " << e->type() << std::endl;
    std::cerr << "Error occurred: " << e->what() << std::endl;
};

int main()
{
    try
    {
        hamza::web::web_server server("127.0.0.1", 12349);
        server.listen(listen_success_callback , error_callback);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
