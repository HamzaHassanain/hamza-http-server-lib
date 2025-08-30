# http_server

Source: `includes/http_server.hpp` (implementation in `src/http_server.cpp`)

The `http_server` class implements a high-level HTTP/1.1 server on top of the project's TCP server infrastructure (`hh_socket::epoll_server`). It combines connection management, request parsing (via `http_message_handler`), and request/response lifecycle handling to provide a simple integration surface for application code.

## Design goals

- Separate transport (TCP/epoll) from HTTP concerns so each layer stays small and testable.
- Provide both callback-based and virtual-method integration styles to suit different application architectures.
- Offer safe, move-only request/response objects that encapsulate minimal connection operations via injected callbacks (send/close).
- Enforce pragmatic limits and simple error signalling to protect against malformed or malicious clients.

## Key characteristics

- Built on `hh_socket::epoll_server` — uses the underlying epoll-based event loop and socket lifecycle.
- Callback + subclassing model — `set_*` setters for common hooks and `on_*` virtuals for overrides.
- Uses `http_message_handler` to parse request lines, headers, content-length and chunked bodies and to accumulate partial requests.
- Produces `http_request` / `http_response` objects for handlers; these objects receive server-supplied lambdas for `send_message` and `close_connection`.
- Enforces `config::MAX_HEADER_SIZE`, `config::MAX_BODY_SIZE`, and `config::MAX_IDLE_TIME_SECONDS` to limit resource usage.

## Constructors & lifecycle

### `http_server(const hh_socket::socket_address &addr, int timeout_milliseconds)`

- Create and register a listening socket via `hh_socket::make_listener_socket`.
- Register the listener with the parent `epoll_server` and start the epoll event loop when `listen()` is called.
- Launches a detached background thread that periodically calls `handler.cleanup_idle_connections(...)` using `config::MAX_IDLE_TIME_SECONDS` to prune idle per-connection parse state.
- Throws on socket creation/bind/listen failures.

### `http_server(int port, const std::string &ip = "0.0.0.0", int timeout_milliseconds = epoll_config::TIMEOUT_MILLISECONDS)`

- Convenience constructor that builds a `socket_address` and forwards to the primary constructor.

### Destructor

- Default destructor inherited; ensure server is stopped and resources are cleaned up via parent class APIs where appropriate.

## Public API (function-level detail)

#### `void set_request_callback(std::function<void(http_request &, http_response &)> callback)`

- Register application callback invoked for each complete HTTP request.
- The callback receives a parsed `http_request` and an empty `http_response` to populate and send.
- Must be set before calling `listen()` or `on_request_received()` will throw when no handler is registered.

#### `void set_listen_success_callback(std::function<void()> callback)`

- Optional: invoked when the server successfully starts listening.

#### `void set_server_stopped_callback(std::function<void()> callback)`

- Optional: invoked when the server shuts down cleanly.

#### `void set_error_callback(std::function<void(const std::exception &)> callback)`

- Optional: centralized error handling for parsing, socket and runtime errors.
- Receives exceptions forwarded from the server internals.

#### `void set_client_connected_callback(std::function<void(std::shared_ptr<hh_socket::connection>)> callback)`

- Optional: invoked when a new client connection is accepted.

#### `void set_client_disconnected_callback(std::function<void(std::shared_ptr<hh_socket::connection>)> callback)`

- Optional: invoked when a client connection is closed and cleaned up.

#### `void set_waiting_for_activity_callback(std::function<void()> callback)`

- Optional: invoked approximately on each I/O wait timeout (useful for periodic tasks while the server idles).

#### `void set_headers_received_callback(std::function<void(std::shared_ptr<hh_socket::connection>, const std::multimap<std::string, std::string> &, const std::string &, const std::string &, const std::string &, const std::string &)> callback)`

- Optional: invoked when headers (and initial body fragment, if present) have been parsed. Useful for pre-body hooks such as authentication or logging.

#### `void listen()`

- Start the server event loop. By default this calls `epoll_server::listen(timeout_milliseconds)` and blocks until `stop_server()` is invoked or an error occurs.

## Message flow (what happens when bytes arrive)

1. The underlying `epoll_server` calls `on_message_received(conn, message)` when bytes are available on a client connection.
2. `on_message_received` constructs two small lambdas bound to the server and the connection:
   - `close_connection_for_objects()` — closes that particular connection when invoked.
   - `send_message_for_request(const std::string &)` — forwards a string to `send_message(conn, data_buffer)` for network transmission.
     These lambdas are injected into `http_request` and `http_response` objects so handler code can send or terminate without direct socket access.
3. The server delegates parsing to `handler.handle(conn, message)` which returns `http_handled_data`.
   - If `completed == false`, parsing is incomplete and the server returns early (more bytes required).
   - If parsing returns an error-coded result, the server stops reading and creates a `http_request` with the error token in the `method` field so the application can respond appropriately.
4. For a complete request the server stops reading from the connection (`stop_reading_from_connection(conn)`), constructs `http_request` and `http_response` (injecting the lambdas), and invokes `on_request_received(request, response)`.

## Error handling

- Parsing errors are signalled by `http_message_handler` via textual codes embedded in `http_handled_data`. The server converts these into a `http_request` carrying the error token so the application may detect and respond.
- Runtime and networking exceptions are forwarded to `set_error_callback` via `on_exception_occurred`.
- The server stops reading from connections when handing requests to handlers to avoid interleaving reads unless the application implements keep-alive semantics.

## Concurrency & threading

- `epoll_server` provides the event loop; `http_server` operates within that context and typically runs on the thread that invoked `listen()`.
- `http_message_handler` protects its internal state with a `std::mutex`, enabling `handle(...)` to be called concurrently if desired.
- A detached background thread periodically runs `handler.cleanup_idle_connections(...)` to close and remove stale partial-request state.

## Limitations & design trade-offs

- Connection lifecycle: the implementation currently favors simple "connection: close" semantics and stops reading from a connection while a request is processed. You can implement the `on_request_received` virtual to resume reading if you want keep-alive behavior, but this requires careful lifecycle management.

- Trailer handling: trailer headers are parsed in chunked flows but not consistently merged into the primary header map in every code path.

- Error signalling: returning error strings via the `method` field in `http_handled_data` is pragmatic but not type-safe. Consider adding an explicit error field for future clarity.

- Background thread: the idle-cleaner is a detached thread with no stop signal. Improve lifecycle management by joining or signaling this thread on shutdown.

## Examples

### Minimal server using callbacks

```cpp
hh_http::http_server server(8080);
server.set_request_callback([](hh_http::http_request &req, hh_http::http_response &res) {
    res.set_body("Hello\n");
    res.add_header("Content-Type", "text/plain");
    res.add_header("Content-Length", std::to_string(res.get_body().size()));
    res.send();
    res.end();
});
server.listen();
```

### Subclassing to override behavior

```cpp
class MyServer : public hh_http::http_server {
protected:
    void on_request_received(hh_http::http_request &req, hh_http::http_response &res) override {
        // custom behavior
    }
};
```

### Some complete examples

See the `examples` directory in the project repository for full working examples demonstrating various features and integration styles.

## Best practices & recommendations

- Register an `error_callback` to capture and log parsing/network errors centrally.
- Tune `config::MAX_BODY_SIZE` and `config::MAX_HEADER_SIZE` to match expected workloads and mitigate resource risks.
- Move `http_request`/`http_response` into worker threads or a thread pool rather than copying; the types are intentionally move-only, or simply move them into an `std::shared_ptr` and pass to the working threads.

- If you need keep-alive/persistent connections, plan for a lifecycle change where reading is resumed after response send and the server can accept multiple requests per connection. in other words, do not end the connection after a single request/response cycle. and you can think about when to end the connection.
