# http_consts

Source: `includes/http_consts.hpp`

This header centralizes small but important HTTP-related constants and simple utilities used across the library. It contains commonly used HTTP version strings, status codes, method and header name constants, CRLF definitions, and a small utility to convert strings to upper-case. It also exposes a couple of configuration namespaces for server-level tunables that are defined elsewhere.

## Design goals

- Provide a single place for shared HTTP constants so code is consistent across the project.
- Keep the constants constexpr where possible so they are inlined and efficient.
- Expose a tiny utility for header-name normalization used by request/response classes.
- Group mutable configuration in named namespaces so values can be defined and tuned in a single translation unit.

## Contents overview

### Namespaces

- `hh_http::epoll_config` — extern integer values for epoll-related limits and timing (e.g., backlog, max file descriptors, default timeouts). These are declared `extern` here and must be defined (given storage) in a single .cpp file.

  - `BACKLOG_SIZE` — maximum number of pending connections for listen(2).
  - `MAX_FILE_DESCRIPTORS` — maximum file descriptor count used by the server.
  - `TIMEOUT_MILLISECONDS` — default epoll/select timeout in milliseconds.

- `hh_http::config` — general HTTP server configuration.

  - `MAX_HEADER_SIZE` — maximum allowed size for request headers (bytes).
  - `MAX_BODY_SIZE` — maximum allowed size for request body (bytes).
  - `MAX_IDLE_TIME_SECONDS` — maximum idle time before closing an idle connection (std::chrono::seconds).

Notes

- Because the configuration values are `extern` in this header, search the project for their definition (typically in a `*.cpp`) to see or change runtime defaults.

### HTTP Version constants

- `HTTP_VERSION_1_0` — "HTTP/1.0"
- `HTTP_VERSION_1_1` — "HTTP/1.1"

These are `constexpr const char*` and suitable for use in status lines and comparisons.

### HTTP Status codes

Common status codes are provided as `constexpr int` values such as:

- `HTTP_OK` (200)
- `HTTP_CREATED` (201)
- `HTTP_NO_CONTENT` (204)
- `HTTP_BAD_REQUEST` (400)
- `HTTP_UNAUTHORIZED` (401)
- `HTTP_FORBIDDEN` (403)
- `HTTP_NOT_FOUND` (404)
- `HTTP_INTERNAL_SERVER_ERROR` (500)

These constants make handler code more readable (use `HTTP_NOT_FOUND` instead of the literal `404`).

### HTTP Methods

Common request methods are provided as `constexpr const char*`:

- `HTTP_GET`, `HTTP_POST`, `HTTP_PUT`, `HTTP_DELETE`, `HTTP_HEAD`, `HTTP_OPTIONS`, `HTTP_PATCH`.

These are convenient when comparing or writing methods into logs or responses.

### Common header name constants

Several common header names are defined as `constexpr const char*` such as:

- `HEADER_CONTENT_TYPE`, `HEADER_CONTENT_LENGTH`, `HEADER_CONNECTION`, `HEADER_HOST`, `HEADER_USER_AGENT`, `HEADER_ACCEPT`, `HEADER_AUTHORIZATION`, `HEADER_REFERER`, `HEADER_COOKIE`, `HEADER_IF_MODIFIED_SINCE`, `HEADER_IF_NONE_MATCH`, `HEADER_EXPECT`.

Note: The header constants preserve their conventional mixed-case presentation (e.g. "Content-Type"). The request/response implementation in this project normalizes header names to upper-case internally via `to_upper_case` when storing headers; use the header name constants as a readability aid rather than relying on their exact case for lookups.

### Line ending helpers

- `CRLF` — "\r\n"
- `DOUBLE_CRLF` — "\r\n\r\n"

Useful when formatting HTTP messages manually.

### Utility: `to_upper_case(const std::string &input)`

- Declared in this header and intended to return an upper-case copy of the provided string.
- Typical use: normalize header names for case-insensitive comparisons/storage.
- Behavior notes:
  - Does not modify the input string; returns a new uppercase string.
  - Use this function for consistent header-name normalization across request/response code.

## Examples

1. Using constants in a response builder

```cpp
res.add_header(HEADER_CONTENT_TYPE, "text/plain; charset=utf-8");
res.set_status(HTTP_OK, "OK");
res.set_version(HTTP_VERSION_1_1);
```

2. Normalizing a header name for lookup

```cpp
auto key = to_upper_case("Content-Type");
auto vals = req.get_header(key);
```

3. Checking a status code

```cpp
if (status == HTTP_NOT_FOUND) {
    // handle 404
}
```

## Implementation notes & best practices

- Prefer the provided constexpr constants over string/numeric literals to improve readability and reduce typos.
- Be aware of the project's header normalization convention (upper-case internal storage). When interacting directly with internal maps use `to_upper_case` for lookups.
- The `extern` configuration variables must be defined once in a .cpp file. To change runtime defaults, update that definition rather than the header.
- The header is intentionally small; avoid adding heavy dependencies or large utilities here.
