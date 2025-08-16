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
    res->add_cookie("session_id", "123456");
    return hamza::web::CONTINUE;
};

auto index_handler = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    try
    {

        std::ifstream file("html/index.html");
        if (file)
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res->html(buffer.str());
        }
        else
        {
            res->set_status(404, "Not Found");
            res->end();
        }

        return hamza::web::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(500, "Internal Server Error");
        res->text("Error: " + std::string(e.what()));
        res->end();
        return hamza::web::EXIT;
    }
};
auto home_handler = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    res->html("<h1>Welcome to the Home Page</h1>");
    res->end();
    return hamza::web::EXIT;
};

auto create_connection_handler = [](std::shared_ptr<hamza::web::web_request> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    // Handle the creation of a new socket connection
    std::cout << "GOT BODY: " << req->get_body() << std::endl;
    res->set_status(201, "Created");
    res->end();
    return hamza::web::EXIT;
};

int main()
{
    try
    {
        hamza::web::web_server server("0.0.0.0", 8080);

        hamza::web::web_route index_route("/", hamza::web::methods::GET, {auth, index_handler});
        hamza::web::web_route create_connection_route("/create-connection", hamza::web::methods::POST, { create_connection_handler});
        hamza::web::web_route home_route("/home/:id/xxx/:param", hamza::web::methods::GET, {home_handler});
        hamza::web::web_router router;

        server.register_static("static");
        router.register_route(std::move(home_route));
        router.register_route(std::move(index_route));
        router.register_route(std::move(create_connection_route));

        server.register_router(std::move(router));

        server.listen(listen_success_callback);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
