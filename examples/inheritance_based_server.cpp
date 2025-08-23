#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <http_server.hpp>
#include <socket_address.hpp>

/**
 * @brief Example HTTP server using inheritance-based architecture
 *
 * This example demonstrates how to extend the http_server class
 * by overriding virtual methods to customize behavior.
 */

class CustomHttpServer : public hamza_http::http_server
{
private:
    // Route handler type
    using RouteHandler = std::function<void(hamza_http::http_request &, hamza_http::http_response &)>;

    // Route storage: method -> path -> handler
    std::map<std::string, std::map<std::string, RouteHandler>> routes;

    // Statistics
    size_t request_count = 0;
    size_t connection_count = 0;

public:
    CustomHttpServer(int port, const std::string &ip = "0.0.0.0")
        : hamza_http::http_server(port, ip, 1000)
    {
        setup_routes();
    }

private:
    void setup_routes()
    {
        // GET routes
        add_route("GET", "/", [this](hamza_http::http_request &req, hamza_http::http_response &res)
                  { handle_home(req, res); });

        add_route("GET", "/stats", [this](hamza_http::http_request &req, hamza_http::http_response &res)
                  { handle_stats(req, res); });

        add_route("GET", "/hello", [this](hamza_http::http_request &req, hamza_http::http_response &res)
                  { handle_hello(req, res); });

        add_route("GET", "/api/info", [this](hamza_http::http_request &req, hamza_http::http_response &res)
                  { handle_api_info(req, res); });

        // POST routes
        add_route("POST", "/api/echo", [this](hamza_http::http_request &req, hamza_http::http_response &res)
                  { handle_api_echo(req, res); });

        add_route("POST", "/api/uppercase", [this](hamza_http::http_request &req, hamza_http::http_response &res)
                  { handle_api_uppercase(req, res); });
    }

    void add_route(const std::string &method, const std::string &path, RouteHandler handler)
    {
        routes[method][path] = handler;
    }

    // Override virtual methods for custom behavior

    /**
     * @brief Override request processing with custom routing
     */
    void on_request_received(hamza_http::http_request &request, hamza_http::http_response &response) override
    {
        request_count++;

        std::string method = request.get_method();
        std::string uri = request.get_uri();

        std::cout << "🔄 [" << request_count << "] " << method << " " << uri << std::endl;

        // Set common headers
        response.set_version("HTTP/1.1");
        response.add_header("Server", "Custom-Hamza-HTTP-Server/1.0");
        response.add_header("Connection", "close");
        response.add_header("X-Request-ID", std::to_string(request_count));

        // Find and execute route handler
        if (routes.count(method) && routes[method].count(uri))
        {
            routes[method][uri](request, response);
        }
        else
        {
            handle_not_found(request, response);
        }

        response.send();
    }

    /**
     * @brief Override connection opened for custom logging
     */
    void on_connection_opened(std::shared_ptr<hamza_socket::connection> conn) override
    {
        connection_count++;
        std::cout << "🔗 Connection #" << connection_count << " opened from "
                  << conn->get_remote_address().to_string() << std::endl;

        // Call parent implementation (if needed)
        hamza_http::http_server::on_connection_opened(conn);
    }

    /**
     * @brief Override connection closed for custom cleanup
     */
    void on_connection_closed(std::shared_ptr<hamza_socket::connection> conn) override
    {
        std::cout << "💔 Connection closed from " << conn->get_remote_address().to_string() << std::endl;

        // Call parent implementation (if needed)
        hamza_http::http_server::on_connection_closed(conn);
    }

    /**
     * @brief Override server startup
     */
    void on_listen_success() override
    {
        std::cout << "🎉 Custom HTTP Server started successfully!" << std::endl;
        std::cout << "🌐 Server is listening on http://localhost:8080" << std::endl;
        std::cout << "📊 Available routes:" << std::endl;

        for (const auto &method_routes : routes)
        {
            for (const auto &route : method_routes.second)
            {
                std::cout << "   " << method_routes.first << " " << route.first << std::endl;
            }
        }

        std::cout << "⏹️  Press Ctrl+C to stop the server" << std::endl;
    }

    /**
     * @brief Override error handling
     */
    void on_exception_occurred(const std::exception &e) override
    {
        std::cerr << "🚨 Server exception: " << e.what() << std::endl;
        // Could implement custom error recovery here
    }

    /**
     * @brief Override idle periods for custom maintenance
     */
    void on_waiting_for_activity() override
    {
        static int idle_count = 0;
        if (++idle_count % 30 == 0)
        { // Every 30 seconds
            std::cout << "📈 Server stats: " << request_count << " requests served, "
                      << connection_count << " total connections" << std::endl;
        }
    }

    // Route handler implementations

    void handle_home(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "text/html; charset=utf-8");

        res.set_body(R"(
<!DOCTYPE html>
<html>
<head>
    <title>Custom HTTP Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 900px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .endpoint { background: #e8f4f8; padding: 15px; margin: 10px 0; border-radius: 8px; border-left: 4px solid #2196F3; }
        .method { font-weight: bold; color: #1976D2; }
        h1 { color: #1976D2; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🏗️ Inheritance-Based HTTP Server</h1>
        <p>This server extends the <code>http_server</code> class and overrides virtual methods for custom behavior.</p>
        
        <h2>🛣️ Available Routes:</h2>
        <div class="endpoint"><span class="method">GET /</span> - This home page</div>
        <div class="endpoint"><span class="method">GET /hello</span> - Simple greeting message</div>
        <div class="endpoint"><span class="method">GET /stats</span> - Server statistics and metrics</div>
        <div class="endpoint"><span class="method">GET /api/info</span> - JSON server information</div>
        <div class="endpoint"><span class="method">POST /api/echo</span> - Echo back the request body</div>
        <div class="endpoint"><span class="method">POST /api/uppercase</span> - Convert request body to uppercase</div>
        
        <h2>🧪 Test Commands:</h2>
        <div class="endpoint">
            <code>curl http://localhost:8080/hello</code><br>
            <code>curl -X POST -d "hello world" http://localhost:8080/api/echo</code><br>
            <code>curl -X POST -d "make me loud" http://localhost:8080/api/uppercase</code>
        </div>
        
        <p><em>🚀 Powered by inheritance and virtual method overrides!</em></p>
    </div>
</body>
</html>
        )");
    }

    void handle_hello(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "text/plain");
        res.set_body("👋 Hello from the custom inheritance-based HTTP server!\n🏗️ Built with virtual method overrides!\n");
    }

    void handle_stats(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "text/html; charset=utf-8");

        std::string stats_html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Server Statistics</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }
        .stat { background: #e8f5e8; padding: 15px; margin: 10px 0; border-radius: 8px; display: flex; justify-content: space-between; }
        .stat-value { font-weight: bold; color: #2e7d32; }
    </style>
</head>
<body>
    <div class="container">
        <h1>📊 Server Statistics</h1>
        <div class="stat">
            <span>Total Requests Served:</span>
            <span class="stat-value">)" +
                                 std::to_string(request_count) + R"(</span>
        </div>
        <div class="stat">
            <span>Total Connections:</span>
            <span class="stat-value">)" +
                                 std::to_string(connection_count) + R"(</span>
        </div>
        <div class="stat">
            <span>Server Type:</span>
            <span class="stat-value">Inheritance-Based</span>
        </div>
        <div class="stat">
            <span>Uptime:</span>
            <span class="stat-value">)" +
                                 std::to_string(time(nullptr)) + R"( seconds</span>
        </div>
        <p><a href="/">← Back to home</a></p>
    </div>
</body>
</html>
        )";

        res.set_body(stats_html);
    }

    void handle_api_info(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "application/json");

        std::string json_response = R"({
    "server": "Custom Hamza HTTP Server",
    "version": "1.0",
    "architecture": "inheritance-based",
    "features": [
        "virtual method overrides",
        "custom routing system",
        "request statistics",
        "connection tracking"
    ],
    "stats": {
        "requests_served": )" + std::to_string(request_count) +
                                    R"(,
        "total_connections": )" + std::to_string(connection_count) +
                                    R"(,
        "timestamp": )" + std::to_string(time(nullptr)) +
                                    R"(
    },
    "current_request": {
        "method": ")" + req.get_method() +
                                    R"(",
        "uri": ")" + req.get_uri() + R"(",
        "http_version": ")" + req.get_version() +
                                    R"("
    }
})";

        res.set_body(json_response);
    }

    void handle_api_echo(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "application/json");

        std::string echo_json = R"({
    "echo": ")" + req.get_body() +
                                R"(",
    "method": ")" + req.get_method() +
                                R"(",
    "uri": ")" + req.get_uri() + R"(",
    "content_length": )" + std::to_string(req.get_body().length()) +
                                R"(,
    "timestamp": )" + std::to_string(time(nullptr)) +
                                R"(
})";

        res.set_body(echo_json);
    }

    void handle_api_uppercase(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "application/json");

        std::string body = req.get_body();
        std::transform(body.begin(), body.end(), body.begin(), ::toupper);

        std::string response_json = R"({
    "original": ")" + req.get_body() +
                                    R"(",
    "uppercase": ")" + body + R"(",
    "length": )" + std::to_string(body.length()) +
                                    R"(,
    "processed_by": "inheritance-based server"
})";

        res.set_body(response_json);
    }

    void handle_not_found(hamza_http::http_request &req, hamza_http::http_response &res)
    {
        res.set_status(404, "Not Found");
        res.add_header("Content-Type", "text/html; charset=utf-8");

        std::string not_found_html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>404 - Not Found</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; text-align: center; background: #f5f5f5; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }
        .error { color: #d32f2f; font-size: 4em; margin: 20px 0; }
    </style>
</head>
<body>
    <div class="container">
        <div class="error">🔍</div>
        <h1>404 - Route Not Found</h1>
        <p>The requested route <code>)" +
                                     req.get_method() + " " + req.get_uri() + R"(</code> was not found.</p>
        <p>This error was handled by the custom inheritance-based server.</p>
        <p><a href="/">🏠 Go back to home</a></p>
    </div>
</body>
</html>
        )";

        res.set_body(not_found_html);
    }
};

int main()
{
    try
    {
        if (!hamza_socket::initialize_socket_library())
        {
            std::cerr << "Failed to initialize socket library." << std::endl;
            return 1;
        }
        std::cout << "🔧 Starting inheritance-based HTTP server..." << std::endl;

        // Create custom server instance
        CustomHttpServer server(8080, "0.0.0.0");

        // Start the server (this will block)
        server.listen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "💥 Failed to start server: " << e.what() << std::endl;
        return 1;
    }

    hamza_socket::cleanup_socket_library();

    return 0;
}
