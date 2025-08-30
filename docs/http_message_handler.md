# http_message_handler

Source: `includes/http_message_handler.hpp`

The `http_message_handler` class is a small, thread-safe parser/assembler for HTTP requests received over TCP. It accepts raw bytes from a `hh_socket::connection` and produces a `http_handled_data` result indicating whether a complete HTTP request is available or whether more bytes must be read and accumulated.

## Design goals

- Provide a single place to parse HTTP request lines, headers and bodies.
- Support both Content-Length and chunked Transfer-Encoding request bodies.
- Accumulate partial requests across multiple TCP segments using an in-memory per-socket state store (`http_data_under_handling`).
- Be safe to call from multiple threads by protecting internal state with a mutex.

## Key characteristics

- Thread-safe entry points: `handle(...)` locks a mutex and dispatches to the appropriate internal path.
- Stateful accumulation: partial requests are stored in `under_handling_data` keyed by a `socket_key` (remote address string) until complete.
- Supports two handling strategies: `CONTENT_LENGTH` and `CHUNKED` as defined by `handling_type`.
- Enforces limits using `hh_http::config` values (`MAX_HEADER_SIZE`, `MAX_BODY_SIZE`) to protect from resource exhaustion.

## Public API

All public methods are defined inline in the header.

### `http_handled_data handle(std::shared_ptr<hh_socket::connection> conn, const hh_socket::data_buffer &message)`

- Purpose: Main entry point. Accepts a connection and a buffer of received bytes and returns a `http_handled_data` indicating either a complete request or that more bytes are required.
- Behavior:
  - Locks an internal mutex to protect `under_handling_data`.
  - Builds a `socket_key` from `conn->get_remote_address().to_string()` and checks for existing in-progress state.
  - If an entry exists, continues handling via `continue_handling(...)`; otherwise starts a fresh parse via `start_handling(...)`.
- Return: `http_handled_data` whose `completed` flag indicates whether a full request has been assembled.

### `http_handled_data continue_handling(http_data_under_handling &data, const hh_socket::data_buffer &message)`

- Purpose: Continue parsing a previously-partially-received request (either `CONTENT_LENGTH` or `CHUNKED`).
- Behavior: Updates `last_activity` timestamp and dispatches to `continue_chunked_handling(...)` or `continue_content_length_handling(...)` based on `data.type`.

### `http_handled_data start_handling(const std::string &socket_key, const hh_socket::data_buffer &message, int FD)`

- Purpose: Begin parsing a new incoming request from the supplied message buffer.
- Steps performed:
  1. Parse the request line into `method`, `uri`, `version` with `parse_request_line(...)`.
  2. Parse headers with `parse_headers(...)`, enforcing `config::MAX_HEADER_SIZE`.
  3. Inspect `Content-Length` and `Transfer-Encoding` headers (case-normalized), and validate combinations (reject repeated `Content-Length` or simultaneous `Content-Length` and `Transfer-Encoding`).
  4. If `Content-Length` present, call `handle_content_length(...)` which either returns a complete `http_handled_data` or creates an `http_data_under_handling` entry to accumulate the body.
  5. If `Transfer-Encoding: chunked` present, call `handle_chunked_encoding(...)` which will parse chunks from the buffer and either return a completed request or create an `http_data_under_handling` for subsequent continuation.
  6. If neither header present, returns a completed `http_handled_data` with empty body.
- Errors: Returns `http_handled_data` with `completed == true` and a textual error code in the `method` field for parse/validation errors (e.g., `BAD_METHOD_OR_URI_OR_VERSION`, `HEADERS_TOO_LARGE`, `REPEATED_LENGTH_OR_TRANSFER_ENCODING_OR_BOTH`).

### `void cleanup_idle_connections(std::chrono::seconds max_idle_time, std::function<void(int)> close_connection)`

- Purpose: Remove and close per-connection parse state that has been idle for longer than `max_idle_time`.
- Behavior: Iterates `under_handling_data` under lock; for entries older than `max_idle_time` calls the supplied `close_connection(fd)` and erases the entry.
- Intended use: Called periodically by higher-level server code to reclaim resources.

## Private helpers (high-level overview)

The header defines several private parsing helpers that implement the parsing logic.

- `parse_request_line(std::istringstream &request_stream, std::string &method, std::string &uri, std::string &version)` — parses the request-line and validates that method/uri/version are present.

- `parse_headers(std::istringstream &request_stream, const std::string &uri, const std::string &version)` — reads header lines until a blank line, trims whitespace, enforces `config::MAX_HEADER_SIZE`, and stores header names normalized via `hh_socket::to_upper_case(header_name)`.

- `contains_chunked(range)` — inspects a Transfer-Encoding range to decide whether "chunked" appears (case-insensitive).

- `handle_content_length(...)` — when full body is present returns completed result; if partial, creates an `http_data_under_handling` entry and returns `completed == false`.

- `handle_chunked_encoding(...)` — parses initial chunks from the provided buffer, validates chunk size and CRLFs, enforces `config::MAX_BODY_SIZE`, collects trailer headers (basic parsing), and either returns completed data or registers an in-progress `http_data_under_handling` entry.

- `continue_chunked_handling(...)` / `continue_content_length_handling(...)` — continue parsing for in-progress chunked or content-length requests using newly-received bytes; when request completes the `under_handling_data` entry is erased and a completed `http_handled_data` is returned.

## Error handling & limits

- Parsing functions return textual error codes inside `http_handled_data` for common parse/validation failures (e.g., `BAD_CHUNK_ENCODING`, `CONTENT_TOO_LARGE`).
- Header and body sizes are checked against `hh_http::config::MAX_HEADER_SIZE` and `hh_http::config::MAX_BODY_SIZE` to mitigate resource exhaustion and abusive clients.

## Concurrency & safety

- The class protects its `under_handling_data` map with a `std::mutex` so that `handle(...)` may be called concurrently from multiple threads.
- The stored `http_data_under_handling` instances are modified in-place while the mutex is held — callers that wish to access or transfer those objects must do so through the `handle` API.

## Examples

### Single-call complete request

```cpp
// conn: shared_ptr<hh_socket::connection>, msg: hh_socket::data_buffer containing a full request
http_message_handler parser;
auto result = parser.handle(conn, msg);
if (result.completed) {
    // process result.method, result.uri, result.headers, result.body
}
```

### Partial request across two reads (Content-Length model)

```cpp
// First read: headers + partial body
auto r1 = parser.handle(conn, first_buf);
if (!r1.completed) {
    // server continues reading and passes next buffer to parser
    auto r2 = parser.handle(conn, next_buf);
    if (r2.completed) {
        // full request available
    }
}
```

### Idle cleanup usage

```cpp
// called periodically by server
parser.cleanup_idle_connections(std::chrono::seconds(30), [&](int fd){ close(fd); });
```

## Notes & best practices

- The parser is intended to be a pragmatic, robust implementation rather than an RFC-complete HTTP parser. It performs basic validation and enforces size limits.
- Trailer headers are parsed but currently not merged with the original header map in every code path; review logic if you rely on trailers for application behavior.
- Chunk parsing uses temporary dynamic buffers; ensure the `config::MAX_BODY_SIZE` is set appropriately for your deployment to bound memory usage.
- Error codes are returned inside `http_handled_data.method` in some failure cases — treat these specially in calling code.
