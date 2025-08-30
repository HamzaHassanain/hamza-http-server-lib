# http_request

Source: `includes/http_request.hpp`

The `http_request` class models a parsed HTTP request delivered to the server's request handler. It is a move-only object that encapsulates method, URI, version, headers, body, and a callback to close the underlying client connection.

## Design goals

- Provide a small, ownership-safe request object for handlers.
- Preserve header multiplicity while normalizing names for consistent lookup.
- Make explicit lifecycle control available (close connection) while preventing accidental copies.

## Key characteristics

- Move-only: copy construction and copy assignment are deleted; move construction is provided.
- Header normalization: internally stores header names using `to_upper_case` to make lookups consistent.
- Explicit close semantics: exposes `destroy(bool Isure)` which invokes a server-supplied `close_connection` callback when called with `true`.
- Not thread-safe: intended to be used by the thread that owns the request/connection.

## Constructors & destructor

### `http_request(...)` (private)

Signature

- `http_request(const std::string &method, const std::string &uri, const std::string &version,
            const std::multimap<std::string, std::string> &headers,
            const std::string &body, std::function<void()> close_connection)`

Purpose

- Construct a request object with supplied metadata and a `close_connection` callback. The constructor normalizes header names by inserting entries using `to_upper_case(header.first)`.

Notes

- This constructor is private and intended to be used by `http_server` (friend).

### `~http_request()`

- Default destructor; cleans up members. The `close_connection` callback remains available until invoked.

## Move semantics

- `http_request(http_request &&other)` — move constructor transfers ownership of method, uri, version, headers, body and the close callback.
- Move assignment is deleted to avoid reassignment after construction.
- After move the source object is in a moved-from (invalid) state and must not be used.

## Public API (function-level detail)

#### `void destroy(bool Isure)`

- Signature: `void destroy(bool Isure)`
- Purpose: Explicitly close the underlying client connection by calling the stored `close_connection()` callback.
- Behavior: Implementation requires `Isure == true`; if false a `std::runtime_error` is thrown with the message "Insure is false, cannot destroy request.". When successful the implementation clears `uri`, `headers`, and `body`.
- Notes: Current implementation contains a redundant second `body.clear()` call (harmless but redundant).

#### `std::string get_method() const`

- Returns the request method (e.g., "GET", "POST").

#### `std::string get_uri() const`

- Returns the request target (path and optional query string).

#### `std::string get_version() const`

- Returns the HTTP version string (e.g., "HTTP/1.1").

#### `std::vector<std::string> get_header(const std::string &name) const`

- Returns all values for a header name. Lookup name is normalized with `to_upper_case(name)` to match internal storage.

#### `std::vector<std::pair<std::string, std::string>> get_headers() const`

- Returns all headers as a list of name/value pairs; names are returned in upper-case form via `to_upper_case` in the implementation.

#### `std::string get_body() const`

- Returns the request body as a string.

## Examples

### Simple inspection inside a handler

```cpp
void handle_request(hh_http::http_request& req) {
    auto method = req.get_method();
    auto uri = req.get_uri();
    auto content_type = req.get_header("Content-Type");
    // build response
}
```

### Explicitly close connection from handler

```cpp
void drop_connection(hh_http::http_request& req) {
    // confirm before closing
    req.destroy(true);
}
```

## Notes and best practices

- Do not use moved-from `http_request` objects; they are invalid after move construction.
- `destroy(true)` permanently closes the client connection and clears request data — use only when you will not send a response.
- Header lookup uses upper-case normalization; normalize names in application code for consistent comparisons if needed.
- The class is not thread-safe; transfer ownership via move when handing the request to another thread.
