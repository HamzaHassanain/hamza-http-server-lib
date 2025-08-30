# http_data_under_handling

Source: `includes/http_data_under_handling.hpp`

The `http_data_under_handling` struct stores intermediate parsing state for HTTP requests that are received across multiple TCP segments. It is used by the message parser/handler to accumulate headers and body until the full request is available.

## Purpose

- Maintain parse state for a single client connection while a request is still being received.
- Support both Content-Length and chunked Transfer-Encoding handling strategies.
- Track last activity time to allow idle-timeout cleanup.

## Structure

Fields

- `std::string socket_key` — an identifier for the client connection (for example, remote address string); used as a key when storing per-connection parse state.
- `int FD` — file descriptor associated with the connection.
- `handling_type type` — enum indicating the parsing strategy: `CONTENT_LENGTH` or `CHUNKED`.
- `std::size_t content_length` — expected body size when using `CONTENT_LENGTH` mode.
- `std::string method` — HTTP method string.
- `std::string uri` — Request URI.
- `std::string version` — HTTP version.
- `std::multimap<std::string, std::string> headers` — Accumulated headers; multiple values per name preserved.
- `std::string body` — Accumulated body bytes.
- `std::chrono::steady_clock::time_point last_activity` — Timestamp of the last activity on this connection, used for timeouts and cleanup.

Constructors

- Default and convenience constructor:

  - `http_data_under_handling()` — default-initialized.
  - `http_data_under_handling(const std::string &socket_key, handling_type type)` — initialize `socket_key` and parsing `type`.

Operators

- `bool operator<(const http_data_under_handling &other) const` — orders instances by `socket_key` (useful if stored in sorted containers).

## Usage patterns

- A parser receives bytes from a connection and locates or creates a `http_data_under_handling` for that `socket_key`.
- The parser appends headers/body to the structure and updates `last_activity`.
- When the request is complete (based on `content_length` or final chunk), the structure is converted into a final request representation (for example `http_handled_data` or `http_request`) and removed from the under-handling store.

## Notes and best practices

- Because this structure holds intermediate state it is expected to be modified in place as bytes arrive; callers should take care to synchronize access if used across threads.
- Use `last_activity` to implement idle connection pruning.
- The `headers` field stores header names as provided; the higher-level request object may normalize names when converting or when performing lookups.
