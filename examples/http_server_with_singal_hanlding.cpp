

#include <http_server.hpp>
#include <http_request.hpp>
#include <http_response.hpp>
#include <csignal>
using namespace hamza_http;

class web : public http_server
{

public:
    web() : http_server("0.0.0.0", 8000) {}

protected:
    void on_request_received(http_request &request, http_response &response) override
    {
        try
        {
            response.add_header("content-type", "application/json");
            response.set_body("{\"message\": \"Hello you sent: " + std::to_string(request.get_body().size()) + " bytes of data\"}");
            response.add_header("Connection", "close");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling request: " << e.what() << std::endl;
            response.set_status(500, "Internal Server Error");
            response.set_body("An error occurred while processing your request.");
        }
        response.send();
        response.end();
    }
    void on_exception(std::shared_ptr<hamza::socket_exception> e) override
    {
        std::cerr << e->what() << std::endl;
    }
    void on_server_stopped() override
    {
        std::cout << "Server stopped." << std::endl;
    }
};

web server;

void handle_signal(int signal)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cerr << "Received signal: " << signal << std::endl;
    server.stop();
}

int main()
{
    if (!hamza::initialize_socket_library())
    {
        std::cerr << "Failed to initialize socket library." << std::endl;
        return 1;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGQUIT, handle_signal);
    std::signal(SIGPIPE, handle_signal);
    try
    {
        server.listen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error occurred:\n"
                  << e.what() << std::endl;
    }

    std::cout << "Before clean up";
    hamza::cleanup_socket_library();

    return 0;
}