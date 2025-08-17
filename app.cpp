#include <bits/stdc++.h>

#include <http_server.hpp>

int main()
{
    hamza_http::http_server server("127.0.0.1", 12345);

    server.set_request_callback([](hamza_http::http_request &request, hamza_http::http_response &response)
                                {
        // Handle the request and prepare the response
        response.set_status(200 , "OK");
        response.add_header("Content-Type", "text/plain");
        response.set_body("Hello, World!");
        response.end(); });
    server.run();

    return 0;
}