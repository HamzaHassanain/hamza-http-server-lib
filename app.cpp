#include <bits/stdc++.h>
#include <web/web_server.hpp>
#include <web/web_route.hpp>
#include <web/web_router.hpp>
#include <web/web_exceptions.hpp>
hamza::web::web_listen_success_callback_t listen_success_callback = []() -> void
{
    std::cout << "Server is listening ......... " << std::endl;
};

hamza::web::web_error_callback_t error_callback = [](std::shared_ptr<hamza::general_socket_exception> e) -> void
{
    std::cerr << "Error occurred: " << e->type() << std::endl;
    std::cerr << "Error occurred: " << e->what() << std::endl;
};

auto auth = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    // throw hamza::web::web_unauthorized_exception("Unauthorized access");
    return hamza::web::CONTINUE;
};

auto index_handler = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    // simulate processing, that takes some time
    std::this_thread::sleep_for(std::chrono::seconds(10));
    res->set_status(200, "OK");
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
        hamza::web::web_server server("127.0.0.1", 12349);

        hamza::web::web_route index_route("/", hamza::web::GET, {auth, index_handler});
        hamza::web::web_route home_route("/home/:id/xxx/:param", hamza::web::GET, {home_handler});
        hamza::web::web_router router;

        router.register_route(std::move(home_route));
        router.register_route(std::move(index_route));
        server.register_router(std::move(router));

        server.listen(listen_success_callback);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
