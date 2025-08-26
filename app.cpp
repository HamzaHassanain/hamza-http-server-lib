#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <thread>
#include "http-lib.hpp"
#include "includes/thread_pool.hpp"
/**
 * @brief Example HTTP server using callback-based architecture
 *
 * This example demonstrates how to use the http_server class with callbacks
 * to handle HTTP requests without inheritance.
 */

hh_http::thread_pool pool(std::thread::hardware_concurrency()); // Create a thread pool with available hardware threads

const std::vector<std::string> allowed_methods = {
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "HEAD",
    "OPTIONS",
    "PATCH",
    "TRACE",
    "CONNECT",
    "PROPFIND",
    "MKCOL",
    "COPY",
    "MOVE",
    "LOCK",
    "UNLOCK"};

void handler(std::shared_ptr<hh_http::http_request> request, std::shared_ptr<hh_http::http_response> response)
{

    if (std::find(allowed_methods.begin(), allowed_methods.end(), request->get_method()) == allowed_methods.end())
    {
        std::cout << "Received " << request->get_method() << " request for " << request->get_uri() << std::endl;

        response->set_status(405, "Method Not Allowed");
        response->send();
        response->end();
        return;
    }

    // Set response headers
    response->set_version("HTTP/1.1");
    response->set_status(200, "OK");
    response->add_header("Content-Type", "text/html; charset=utf-8");
    response->add_header("Server", "hh-HTTP-Server/1.0");
    response->add_header("Connection", "close");

    response->send();
    response->end();
}
void handle_request(hh_http::http_request &request, hh_http::http_response &response)
{
    // Offload request handling to thread pool
    auto req_ptr = std::make_shared<hh_http::http_request>(std::move(request));
    auto res_ptr = std::make_shared<hh_http::http_response>(std::move(response));
    pool.enqueue([req_ptr, res_ptr]()
                 { handler(req_ptr, res_ptr); });
}

void on_client_connected(std::shared_ptr<hh_socket::connection> conn)
{
    // std::cout << "✅ Client connected from " << conn->get_remote_address().to_string() << std::endl;
}

void on_client_disconnected(std::shared_ptr<hh_socket::connection> conn)
{
    // std::cout << "❌ Client disconnected from " << conn->get_remote_address().to_string() << std::endl;
}

void on_server_started()
{
    std::cout << "🚀 Callback-based HTTP server started successfully!" << std::endl;
    std::cout << "📡 Server is listening on http://localhost:8081" << std::endl;
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
        hh_http::config::MAX_IDLE_TIME_SECONDS = std::chrono::seconds(5);
        hh_http::config::MAX_HEADER_SIZE = 1024 * 32;
        hh_http::config::MAX_BODY_SIZE = 1024 * 20; // 20 KB
        if (!hh_socket::initialize_socket_library())
        {
            std::cerr << "Failed to initialize socket library." << std::endl;
            return 1;
        }
        std::cout << "🔧 Starting callback-based HTTP server..." << std::endl;

        // Create HTTP server on port 8081
        hh_http::http_server server(8081, "0.0.0.0");

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
