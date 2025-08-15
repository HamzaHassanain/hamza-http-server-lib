#include <bits/stdc++.h>
#include <web/web_server.hpp>
#include <web/web_route.hpp>
#include <web/web_router.hpp>
hamza::web::web_listen_success_callback_t listen_success_callback = []() -> void
{
    std::cout << "Server is listening -^- " << std::endl;
};

hamza::web::web_error_callback_t error_callback = [](std::shared_ptr<hamza::general_socket_exception> e) -> void
{
    std::cerr << "Error occurred: " << e->type() << std::endl;
    std::cerr << "Error occurred: " << e->what() << std::endl;
};

auto index_handler = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    res->html("<h1>Welcome to the Index Page</h1>");
    res->end();
    return hamza::web::EXIT;
};
auto home_handler = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    res->html("<h1>Welcome to the Home Page</h1>");
    res->end();
    return hamza::web::EXIT;
};

int main()
{
    try
    {
        hamza::web::web_route index_route(hamza::web::GET, "/", {index_handler});
        hamza::web::web_route home_route(hamza::web::GET, "/home", {home_handler});

        hamza::web::web_server server("127.0.0.1", 12349);
        server.register_route(index_route);
        server.register_route(home_route);

        server.listen(listen_success_callback, error_callback);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
