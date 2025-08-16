#include <bits/stdc++.h>
#include <web/web_types.hpp>
#include <web/web_server.hpp>
#include <web/web_route.hpp>
#include <web/web_router.hpp>
#include <web/web_exceptions.hpp>
#include <web/web_helpers.hpp>

using namespace hamza::web;

class CR : public hamza::web::web_request
{
public:
    std::map<std::string, std::string> form_data;
    CR(hamza::http::http_request &&req)
        : web_request(std::move(req)) {}
};

using req_handler = web_request_handler_t<CR, web_response>;

hamza::web::web_listen_success_callback_t listen_success_callback = []() -> void
{
    std::cout << "Server is listening on port 8080" << std::endl;
};

req_handler auth = [](std::shared_ptr<CR> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    res->add_cookie("session_id", "123456");
    return hamza::web::CONTINUE;
};

req_handler index_handler = [](std::shared_ptr<CR> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    try
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate some processing delay
        std::ifstream file("html/index.html");
        if (file)
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res->html(buffer.str());
        }
        else
        {
            throw hamza::web::web_internal_server_error_exception("Failed to open index.html");
        }

        return hamza::web::EXIT;
    }
    catch (const web_general_exception &e)
    {
        std::cout << "Error Type " << e.type() << std::endl;
        std::cout << "Error: " << e.what() << std::endl;
        res->set_status(500, "Internal Server Error");
        res->text("Error: " + std::string(e.what()));
        res->end();
        return hamza::web::EXIT;
    }
};

req_handler form_parser = [](std::shared_ptr<CR> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    try
    {
        req->form_data = helpers::parse_form_data(req->get_body());

        return hamza::web::CONTINUE;
    }
    catch (const std::exception &e)
    {
        res->set_status(400, "Bad Request");
        res->text("Error parsing form data: " + std::string(e.what()));
        return hamza::web::EXIT;
    }
};
req_handler create_connection_handler = [](std::shared_ptr<CR> req, std::shared_ptr<hamza::web::web_response> res) -> int
{
    res->set_status(201, "Created");
    std::string data;
    for (const auto &[key, value] : req->form_data)
    {
        data += key + ": " + value + "\n";
    }
    res->set_body("Connection created:\n" + data);
    return hamza::web::EXIT;
};
using ptr_route = std::shared_ptr<web_route<CR>>;
using ptr_router = std::shared_ptr<web_router<CR>>;
int main()
{
    try
    {
        web_server<CR> server("0.0.0.0", 8080);

        auto index_route = ptr_route(new web_route<CR>("/", methods::GET, {auth, index_handler}));
        auto create_connection_route = ptr_route(new web_route<CR>("/create-connection", methods::POST, {auth, form_parser, create_connection_handler}));

        auto router = ptr_router(new web_router<CR>());

        router->register_route(index_route);
        router->register_route((create_connection_route));

        server.register_static("static");
        server.register_router(router);

        server.listen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
