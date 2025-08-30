#include <iostream>
#include <string>
#include "../http-lib.hpp"


/**
 * @brief Examplet HTTP server using callback-based architecture
 *
 * This example demonstrates how to use the http_server class with callbacks
 * to handle HTTP requests without inheritance.
 */

void handle_request(hh_http::http_request &request, hh_http::http_response &response)
{
    std::cout << "Received " << request.get_method() << " request for " << request.get_uri() << std::endl;

    // Set response headers
    response.set_version("HTTP/1.1");
    response.add_header("Content-Type", "text/html; charset=utf-8");
    response.add_header("Server", "hh-HTTP-Server/1.0");
    response.add_header("Connection", "close");

    // Route handling based on URI
    std::string uri = request.get_uri();

    if (uri == "/" || uri == "/index")
    {
        response.set_status(200, "OK");
        response.set_body(R"(
<!DOCTYPE html>
<html>
<head>
    <title>Callback-Based HTTP Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 800px; margin: 0 auto; }
        .endpoint { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 Callback-Based HTTP Server</h1>
        <p>This server uses callback functions to handle HTTP requests.</p>
        
        <h2>Available Endpoints:</h2>
        <div class="endpoint"><strong>GET /</strong> - This page</div>
        <div class="endpoint"><strong>GET /hello</strong> - Simple greeting</div>
        <div class="endpoint"><strong>GET /info</strong> - Server information</div>
        <div class="endpoint"><strong>POST /echo</strong> - Echo request body</div>
        <div class="endpoint"><strong>GET /headers</strong> - Show request headers</div>
        
        <p><em>Try visiting these endpoints or use curl to test POST requests!</em></p>
    </div>
</body>
</html>
        )");
    }
    else if (uri == "/hello")
    {
        response.set_status(200, "OK");
        response.add_header("Content-Type", "text/plain");
        response.set_body("Hello from callback-based HTTP server! 👋\n");
    }
    else if (uri == "/info")
    {
        response.set_status(200, "OK");
        response.add_header("Content-Type", "application/json");

        std::string info = R"({
    "server": "hh HTTP Server",
    "version": "1.0",
    "architecture": "callback-based",
    "method": ")" + request.get_method() +
                           R"(",
    "uri": ")" + request.get_uri() +
                           R"(",
    "http_version": ")" + request.get_version() +
                           R"(",
    "timestamp": ")" + std::to_string(time(nullptr)) +
                           R"("
})";
        response.set_body(info);
    }
    else if (uri == "/echo" && request.get_method() == "POST")
    {
        response.set_status(200, "OK");
        response.add_header("Content-Type", "text/plain");

        std::string echo_response = "Echo Response:\n";
        echo_response += "Method: " + request.get_method() + "\n";
        echo_response += "URI: " + request.get_uri() + "\n";
        echo_response += "Body Length: " + std::to_string(request.get_body().length()) + "\n";
        echo_response += "Body Content:\n" + request.get_body();

        response.set_body(echo_response);
    }
    else if (uri == "/headers")
    {
        response.set_status(200, "OK");
        response.add_header("Content-Type", "text/plain");

        std::string headers_info = "Request Headers:\n";
        headers_info += "Method: " + request.get_method() + "\n";
        headers_info += "URI: " + request.get_uri() + "\n";
        headers_info += "Version: " + request.get_version() + "\n\n";

        auto headers = request.get_headers();
        for (const auto &header : headers)
        {
            headers_info += header.first + ": " + header.second + "\n";
        }

        response.set_body(headers_info);
    }
    else
    {
        // 404 Not Found
        response.set_status(404, "Not Found");
        response.add_header("Content-Type", "text/html");
        response.set_body(R"(
<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>
    <h1>404 - Page Not Found</h1>
    <p>The requested resource <code>)" +
                          uri + R"(</code> was not found on this server.</p>
    <p><a href="/">Go back to home page</a></p>
</body>
</html>
        )");
    }

    // Send the response
    response.send();
}

void on_client_connected(std::shared_ptr<hh_socket::connection> conn)
{
    std::cout << "✅ Client connected from " << conn->get_remote_address().to_string() << std::endl;
}

void on_client_disconnected(std::shared_ptr<hh_socket::connection> conn)
{
    std::cout << "❌ Client disconnected from " << conn->get_remote_address().to_string() << std::endl;
}

void on_server_started()
{
    std::cout << "🚀 Callback-based HTTP server started successfully!" << std::endl;
    std::cout << "📡 Server is listening on http://localhost:8080" << std::endl;
    std::cout << "🔄 Press Ctrl+C to stop the server" << std::endl;
}

void on_server_error(const std::exception &e)
{
    std::cerr << "❌ Server error: " << e.what() << std::endl;
}

void on_waiting_for_activity()
{
    // This is called during idle periods - useful for periodic tasks
    static int counter = 0;
    if (++counter % 10 == 0)
    { // Print every 10 seconds
        std::cout << "💤 Server idle... waiting for connections" << std::endl;
    }
}

int main()
{
    try
    {
        if (!hh_socket::initialize_socket_library())
        {
            std::cerr << "Failed to initialize socket library." << std::endl;
            return 1;
        }
        std::cout << "🔧 Starting callback-based HTTP server..." << std::endl;

        // Create HTTP server on port 8080
        hh_http::http_server server(8080, "0.0.0.0", 1000);

        // Set up all the callbacks
        server.set_request_callback(handle_request);
        server.set_client_connected_callback(on_client_connected);
        server.set_client_disconnected_callback(on_client_disconnected);
        server.set_listen_success_callback(on_server_started);
        server.set_error_callback(on_server_error);
        server.set_waiting_for_activity_callback(on_waiting_for_activity);

        // Start the server (this will block)
        server.listen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "💥 Failed to start server: " << e.what() << std::endl;
        return 1;
    }

    hh_socket::cleanup_socket_library();
    return 0;
}
