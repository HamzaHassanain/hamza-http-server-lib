#include <bits/stdc++.h>
#include <web_server.hpp>

hamza::web::listen_success_callback_t listen_success_callback = []() -> void
{
    std::cout << "Server is listening -^- " << std::endl;
};

hamza::web::error_callback_t error_callback = [](std::shared_ptr<hamza::general_socket_exception> e) -> void
{
    std::cerr << "Error occurred: " << e->type() << std::endl;
    std::cerr << "Error occurred: " << e->what() << std::endl;
};

hamza::web::request_callback_t index_router = [](hamza::http::http_request &request, hamza::http::http_response &response) -> void
{
    response.set_status(200, "OK");
    response.add_header("Content-Type", "text/html");

    std::string index = R"(<!DOCTYPE html>
                            <html lang="en">
                            <head>
                                <meta charset="UTF-8" />
                                <meta name="viewport" content="width=device-width, initial-scale=1.0" />
                                <title>HAMZAAA</title>
                            </head>
                            <body>
                                <h1>HAMZAAA</h1>
                            </body>
                            </html>
                            )";

    response.set_body(index);
    response.end();
};

hamza::web::request_callback_t home_router = [](hamza::http::http_request &request, hamza::http::http_response &response) -> void
{
    response.set_status(200, "OK");
    response.add_header("Content-Type", "text/html");

    std::string home = R"(<!DOCTYPE html>
                            <html lang="en">
                            <head>
                                <meta charset="UTF-8" />
                                <meta name="viewport" content="width=device-width, initial-scale=1.0" />
                                <title>Home</title>
                            </head>
                            <body>
                                <h1>Welcome to the Home Page</h1>
                            </body>
                            </html>
                            )";

    response.set_body(home);
    response.end();
};

hamza::web::request_callback_t form_handler = [](hamza::http::http_request &request, hamza::http::http_response &response) -> void
{
    response.set_status(200, "OK");
    response.add_header("Content-Type", "text/html");

    std::string form = R"(<!DOCTYPE html>
                            <html lang="en">
                            <head>
                                <meta charset="UTF-8" />
                                <meta name="viewport" content="width=device-width, initial-scale=1.0" />
                                <title>Form</title>
                            </head>
                            <body>
                                <h1>Form Submission</h1>
                                <form method="POST" action="/">
                                    <input type="text" name="data" placeholder="Enter some data" required />
                                    <button type="submit">Submit</button>
                                </form>
                            </body>
                            </html>
                            )";

    response.set_body(form);
    response.end();
};

hamza::web::request_callback_t form_submit_handler = [](hamza::http::http_request &request, hamza::http::http_response &response) -> void
{
    response.set_status(200, "OK");
    response.add_header("Content-Type", "text/html");

    std::string data = request.get_body();
    std::string message = "You submitted: " + data;

    std::string response_body = R"(<!DOCTYPE html>
                                    <html lang="en">
                                    <head>
                                        <meta charset="UTF-8" />
                                        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
                                        <title>Form Submitted</title>
                                    </head>
                                    <body>
                                        <h1>)" +
                                message + R"(</h1>
                                    </body>
                                    </html>)";

    response.set_body(response_body);
    response.end();
};

int main()
{
    try
    {
        hamza::web::web_server server("127.0.0.1", 12349);
        std::cout << "Hello World!" << std::endl;
        // hamza::web::web_router router({hamza::web::web_route(hamza::web::GET, "/", index_router),
        //                                hamza::web::web_route(hamza::web::GET, "/home", home_router)
        //                                    hamza::web::web_route(hamza::web::GET, "/form", form_handler)
        //                                        hamza::web::web_route(hamza::web::POST, "/form", form_submit_handler)});
        // server.register_router(router);

        server.listen(listen_success_callback, error_callback);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
