#!/usr/bin/env node

/**
 * http_fuzzer.js
 *
 * Fuzz test an HTTP server with random and malicious inputs to identify vulnerabilities.
 * JavaScript implementation with concurrent request execution.
 *
 * Features:
 * - Random valid/invalid HTTP headers
 * - Random method fuzzing
 * - Path traversal attempts
 * - HTTP header injection
 * - Random HTTP body sizes and content
 * - Protocol violations
 * - Timing attacks (slow requests)
 *
 * Usage:
 *   node http_fuzzer.js
 *   node http_fuzzer.js --host 0.0.0.0 --port 8080 --timeout 5000 --tests 100
 *   node http_fuzzer.js --mode malicious  # Focus on malicious tests
 *   node http_fuzzer.js --mode random     # Focus on random but valid tests
 *   node http_fuzzer.js --mode all        # Run all tests (default)
 */

const net = require("net");
const crypto = require("crypto");
const querystring = require("querystring");
const path = require("path");

// ------------------------------------------------------------
// Config
// ------------------------------------------------------------
const DEFAULT_HOST = "127.0.0.1";
const DEFAULT_PORT = 8081;
const RECV_MAX = 65536; // max bytes to read from server per test
const HEADER_INJECTION_MARKER = "Injected: yes";
const DEFAULT_FUZZ_COUNT = 100; // Number of random fuzz tests to run
const DEFAULT_CONCURRENCY = 10; // Number of tests to run concurrently
const DEFAULT_TIMEOUT_MS = 2000; // Timeout in milliseconds
// ------------------------------------------------------------

/**
 * Sends a raw request to the server and returns the response
 * @param {Buffer} rawRequest - The raw HTTP request as a Buffer
 * @param {string} host - The server host
 * @param {number} port - The server port
 * @param {number} timeout - Timeout in milliseconds
 * @param {boolean} slowSend - Whether to send the request slowly (for timing attacks)
 * @returns {Promise<{response: Buffer|null, error: string|null}>}
 */
async function sendAndReceive(
  rawRequest,
  host,
  port,
  timeout,
  slowSend = false
) {
  return new Promise((resolve) => {
    let response = Buffer.alloc(0);
    const socket = new net.Socket();

    // Set timeout
    socket.setTimeout(timeout);

    // Set up event handlers
    socket.on("data", (data) => {
      response = Buffer.concat([response, data]);
      if (response.length >= RECV_MAX) {
        socket.end();
      }
    });

    socket.on("error", (err) => {
      resolve({ response: null, error: `ERR:${err.message}` });
    });

    socket.on("timeout", () => {
      socket.end();
    });

    socket.on("end", () => {
      resolve({ response: response.length > 0 ? response : null, error: null });
    });

    socket.on("close", () => {
      resolve({ response, error: null });
    });

    // Connect to server
    socket.connect(port, host, () => {
      if (slowSend) {
        // Send byte by byte with small delays
        let offset = 0;
        const sendNextByte = () => {
          if (offset < rawRequest.length) {
            socket.write(rawRequest.slice(offset, offset + 1));
            offset++;
            setTimeout(sendNextByte, 10); // 10ms delay between bytes
          } else {
            // Done sending
          }
        };
        sendNextByte();
      } else {
        // Send all at once
        socket.write(rawRequest);
      }
    });
  });
}

/**
 * Parses the HTTP status code from the response
 * @param {Buffer} response - The HTTP response
 * @returns {number|null} - The status code or null if not found
 */
function parseStatus(response) {
  if (!response) return null;

  const statusMatch = response
    .toString("utf8", 0, Math.min(response.length, 50))
    .match(/HTTP\/\d+\.\d+\s+(\d{3})/);

  if (statusMatch) {
    try {
      return parseInt(statusMatch[1], 10);
    } catch (e) {
      return null;
    }
  }
  return null;
}

/**
 * Checks if the response contains the injection marker
 * @param {Buffer} response - The HTTP response
 * @returns {boolean} - Whether the response contains the injection marker
 */
function containsInjected(response) {
  if (!response) return false;
  return response.toString().includes(HEADER_INJECTION_MARKER);
}

/**
 * Prints a test result line
 * @param {string} name - The test name
 * @param {boolean} ok - Whether the test passed
 * @param {string} msg - Additional message
 */
function resultLine(name, ok, msg = "") {
  const status = ok ? "PASS" : "FAIL";
  console.log(`[${status}] ${name}: ${msg}`);
}

/**
 * Generates a random string
 * @param {number} length - The string length
 * @param {boolean} includeSpecial - Whether to include special characters
 * @returns {string} - A random string
 */
function randomString(length, includeSpecial = false) {
  const chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const specialChars = "!@#$%^&*()_+-=[]{}|;:,.<>?/\\'\"`~";

  let result = "";
  const charSet = includeSpecial ? chars + specialChars : chars;

  for (let i = 0; i < length; i++) {
    result += charSet.charAt(Math.floor(Math.random() * charSet.length));
  }

  return result;
}

/**
 * Generates random bytes
 * @param {number} length - The number of bytes
 * @returns {Buffer} - Random bytes
 */
function randomBytes(length) {
  return crypto.randomBytes(length);
}

/**
 * Generates a random HTTP method
 * @returns {string} - A random HTTP method
 */
function randomMethod(malicious = false) {
  const methods = [
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
    "UNLOCK",
    // Random invalid methods
  ];
  if (malicious) {
    methods.push(
      "INVALID",
      "ATTACK",
      "TEST",
      randomString(5, true),
      randomString(20, true)
    );
  }
  return methods[Math.floor(Math.random() * methods.length)];
}

/**
 * Generates a random URI
 * @param {boolean} includeTraversal - Whether to include path traversal attempts
 * @returns {string} - A random URI
 */
function randomURI(includeTraversal = false) {
  const depth = Math.floor(Math.random() * 5);

  if (includeTraversal && Math.random() < 0.5) {
    // Include path traversal attempts
    const traversalPatterns = [
      "/../../../etc/passwd",
      "/..%2f..%2f..%2fetc%2fpasswd",
      "/%2e%2e/%2e%2e/etc/passwd",
      "/././././././../../../etc/passwd",
      "/../../../windows/win.ini",
      "/..\\..\\..\\..\\..\\..\\",
    ];
    return traversalPatterns[
      Math.floor(Math.random() * traversalPatterns.length)
    ];
  }

  let path = "";
  for (let i = 0; i < depth; i++) {
    path += "/" + randomString(Math.floor(Math.random() * 10) + 1);
  }

  // Add query params sometimes
  if (Math.random() < 0.3) {
    path += "?";
    for (let i = 0; i < Math.floor(Math.random() * 3) + 1; i++) {
      path +=
        randomString(Math.floor(Math.random() * 8) + 1) +
        "=" +
        randomString(Math.floor(Math.random() * 10) + 1);
      if (Math.random() < 0.5 && i < 2) {
        path += "&";
      }
    }
  }

  return path || "/";
}

/**
 * Generates a random HTTP version
 * @returns {string} - A random HTTP version
 */
function randomHTTPVersion(malicious = false) {
  const versions = ["HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/0.9"];
  if (malicious) {
    versions.push(
      "HTTP/3.0",
      "HTTP/1.2",
      "HTTP/0.1",
      "HTTPX/1.1",
      "HTT/1.1",
      randomString(10),
      `HTTP/${Math.floor(Math.random() * 10)}.${Math.floor(Math.random() * 10)}`
    );
  }
  return versions[Math.floor(Math.random() * versions.length)];
}

/**
 * Generates a random header name
 * @param {boolean} malicious - Whether to generate potentially malicious header names
 * @returns {string} - A random header name
 */
function randomHeaderName(malicious = false) {
  const commonHeaders = [
    "Accept",
    "Accept-Charset",
    "Accept-Encoding",
    "Accept-Language",
    "Authorization",
    "Cache-Control",
    "Connection",
    "Content-Encoding",
    "Content-Length",
    "Content-Type",
    "Cookie",
    "Date",
    "Expect",
    "From",
    "Host",
    "If-Match",
    "If-Modified-Since",
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Max-Forwards",
    "Pragma",
    "Proxy-Authorization",
    "Range",
    "Referer",
    "TE",
    "Upgrade",
    "User-Agent",
    "Via",
    "Warning",
    "X-Forwarded-For",
    "X-Requested-With",
  ];

  if (malicious && Math.random() < 0.7) {
    const maliciousHeaders = [
      // Headers with control chars
      "X-Control-" + String.fromCharCode(Math.floor(Math.random() * 32)),
      // Very long header names
      "X-" + randomString(Math.floor(Math.random() * 401) + 100),
      // Headers with special chars
      "X-Special-\r\n-Header",
      "X-Null-\0-Header",
      // Non-ASCII headers
      "X-Unicode-" +
        String.fromCharCode(Math.floor(Math.random() * 872) + 128) +
        String.fromCharCode(Math.floor(Math.random() * 872) + 128),
      // Headers missing a colon (should be detected when building the request)
      "Invalid Header Without Colon",
      // Empty header
      "",
      // Binary data in header name
      "X-Binary-" + randomBytes(10).toString("latin1"),
    ];
    return maliciousHeaders[
      Math.floor(Math.random() * maliciousHeaders.length)
    ];
  }

  // 80% standard headers, 20% custom
  if (Math.random() < 0.8) {
    return commonHeaders[Math.floor(Math.random() * commonHeaders.length)];
  } else {
    const prefix = ["X-", "Custom-", "Test-", ""][
      Math.floor(Math.random() * 4)
    ];
    return prefix + randomString(Math.floor(Math.random() * 20) + 1);
  }
}

/**
 * Generates a random header value
 * @param {boolean} malicious - Whether to generate potentially malicious header values
 * @returns {string} - A random header value
 */
function randomHeaderValue(malicious = false) {
  if (!malicious || Math.random() < 0.6) {
    // 60% normal even in malicious mode
    return randomString(Math.floor(Math.random() * 30) + 1);
  }

  // Malicious header values
  const maliciousValues = [
    // CRLF injection
    "normal\r\nInjected: yes\r\nX-Injected: also-yes",
    // Percent-encoded CRLF
    "normal%0d%0aInjected: yes%0d%0aX-Percent: encoded",
    // Unicode newlines
    `normal${String.fromCharCode(0x2028)}Injected: unicode-line`,
    // NUL byte
    "normal\0value",
    // Control characters
    Array(5)
      .fill(0)
      .map(() => String.fromCharCode(Math.floor(Math.random() * 31) + 1))
      .join(""),
    // Very long value
    "X".repeat(Math.floor(Math.random() * 9001) + 1000),
    // XSS attempt
    "<script>alert('XSS')</script>",
    // SQL injection attempt
    "' OR 1=1 --",
    // Binary data
    randomBytes(20).toString("latin1"),
    // Slash escaping
    "value\\r\\nInjected: backslash",
  ];
  return maliciousValues[Math.floor(Math.random() * maliciousValues.length)];
}

/**
 * Generates a random content type
 * @returns {string} - A random content type
 */
function randomContentType() {
  const types = [
    "application/json",
    "application/x-www-form-urlencoded",
    "multipart/form-data; boundary=---------------------------974767299852498929531610575",
    "text/plain",
    "text/html",
    "application/xml",
    "application/octet-stream",
    // Invalid or uncommon types
    "invalid/content-type",
    "text/random-gibberish",
    randomString(10) + "/" + randomString(10),
    // XSS in content type
    'text/html; charset="><script>alert(1)</script>',
    // Overly complex
    'multipart/mixed; boundary=gc0p4Jq0M2Yt08jU534c0p; charset=utf-8; type="text/example"',
  ];
  return types[Math.floor(Math.random() * types.length)];
}

/**
 * Generates a random JSON object with the specified depth
 * @param {number} depth - The depth of nesting
 * @returns {any} - A random JSON-compatible value
 */
function generateRandomJSON(depth = 2) {
  if (depth <= 0 || Math.random() < 0.3) {
    // Leaf values
    const valType = Math.floor(Math.random() * 4) + 1;
    if (valType === 1) {
      return Math.floor(Math.random() * 2000000) - 1000000;
    } else if (valType === 2) {
      return Math.random() * 1000;
    } else if (valType === 3) {
      return [true, false, null][Math.floor(Math.random() * 3)];
    } else {
      return randomString(Math.floor(Math.random() * 30) + 1);
    }
  }

  // Container types
  if (Math.random() < 0.5) {
    // Dictionary
    const result = {};
    const size = Math.floor(Math.random() * 5) + 1;
    for (let i = 0; i < size; i++) {
      const key = randomString(Math.floor(Math.random() * 10) + 1);
      result[key] = generateRandomJSON(depth - 1);
    }
    return result;
  } else {
    // List
    const result = [];
    const size = Math.floor(Math.random() * 5) + 1;
    for (let i = 0; i < size; i++) {
      result.push(generateRandomJSON(depth - 1));
    }
    return result;
  }
}

/**
 * Generates a random HTTP body based on the content type
 * @param {string|null} contentType - The content type
 * @param {boolean} malicious - Whether to generate potentially malicious content
 * @param {number} maxSize - The maximum size of the body in bytes
 * @returns {Buffer} - A random body
 */
function randomBody(contentType = null, malicious = false, maxSize = 10240) {
  if (!contentType) {
    return Buffer.from("");
  }

  const size = malicious
    ? Math.floor(Math.random() * 100000)
    : Math.floor(Math.random() * maxSize);

  if (contentType.includes("json")) {
    // Generate random JSON
    const depth = Math.floor(Math.random() * 3) + 1;
    let body = JSON.stringify(generateRandomJSON(depth));
    if (malicious && Math.random() < 0.5) {
      // Make JSON invalid sometimes
      body = body.substring(0, body.length - 2);
    }
    return Buffer.from(body);
  } else if (contentType.includes("form-urlencoded")) {
    // Generate random form data
    const formItems = [];
    for (let i = 0; i < Math.floor(Math.random() * 10) + 1; i++) {
      const key = randomString(Math.floor(Math.random() * 10) + 1);
      let value = randomString(Math.floor(Math.random() * 20) + 1);
      if (malicious && Math.random() < 0.5) {
        // Add some problematic values
        value = [
          "value with spaces",
          "value&with=special&chars",
          "value\nwith\nnewlines",
          "<script>alert('xss')</script>",
        ][Math.floor(Math.random() * 4)];
      }
      formItems.push(`${key}=${encodeURIComponent(value)}`);
    }
    return Buffer.from(formItems.join("&"));
  } else if (contentType.includes("multipart/form-data")) {
    // Extract boundary
    const boundaryMatch = contentType.match(/boundary=([^\s;]+)/);
    const boundary = boundaryMatch
      ? boundaryMatch[1]
      : "----WebKitFormBoundary7MA4YWxkTrZu0gW";

    // Create multipart form data
    let body = "";
    for (let i = 0; i < Math.floor(Math.random() * 5) + 1; i++) {
      body += `--${boundary}\r\n`;
      body += `Content-Disposition: form-data; name="${randomString(
        5
      )}"\r\n\r\n`;
      body += `${randomString(Math.floor(Math.random() * 100) + 1)}\r\n`;
    }

    // Add file part sometimes
    if (Math.random() < 0.5) {
      body += `--${boundary}\r\n`;
      body += `Content-Disposition: form-data; name="file"; filename="${randomString(
        8
      )}.txt"\r\n`;
      body += `Content-Type: text/plain\r\n\r\n`;
      body += `${randomString(Math.floor(Math.random() * 990) + 10)}\r\n`;
    }

    body += `--${boundary}--\r\n`;
    return Buffer.from(body);
  } else if (contentType.includes("xml")) {
    // Generate random XML
    let xml = `<?xml version="1.0" encoding="UTF-8"?>\n<root>\n  <item>${randomString(
      20
    )}</item>\n</root>`;
    if (malicious && Math.random() < 0.5) {
      // XML with XXE attack attempt
      xml = `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE test [
  <!ENTITY xxe SYSTEM "file:///etc/passwd">
]>
<root>
  <data>&xxe;</data>
</root>`;
    }
    return Buffer.from(xml);
  } else if (contentType.includes("text/")) {
    // Random text
    return Buffer.from(randomString(size));
  } else {
    // Random binary data for unknown types
    return randomBytes(size);
  }
}

// -------------------------
// Test cases implementations
// -------------------------

/**
 * CRLF injection test
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testCRLFInjection(host, port, timeout) {
  const raw = Buffer.from(
    "GET / HTTP/1.1\r\nHost: localhost\r\nX-Exploit: safe\r\nInjected: yes\r\n\r\n"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const injected = containsInjected(response);
  const code = parseStatus(response);

  if (injected) {
    return {
      ok: false,
      msg: `response contained injected header (vulnerable). status=${code}`,
    };
  }

  return { ok: true, msg: `no injected header found. status=${code}` };
}

/**
 * Percent-encoded newline test
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testPercentEncodedNewline(host, port, timeout) {
  const raw = Buffer.from(
    "GET / HTTP/1.1\r\nHost: localhost\r\nX-Exploit: safe%0d%0aInjected: yes\r\n\r\n"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const injected = containsInjected(response);
  const code = parseStatus(response);

  if (injected) {
    return {
      ok: false,
      msg: `response contained injected header (vulnerable). status=${code}`,
    };
  }

  return { ok: true, msg: `no injected header found. status=${code}` };
}

/**
 * Test with null byte in header
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testNullByteInHeader(host, port, timeout) {
  const raw = Buffer.from(
    "GET / HTTP/1.1\r\nHost: localhost\r\nX-Null: value\0more\r\n\r\n"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const code = parseStatus(response);
  const injected = containsInjected(response);

  if (injected) {
    return { ok: false, msg: "injected marker present (unexpected)" };
  }

  // Prefer server to reject - accept null or 4xx/5xx as pass, fail on 2xx
  if (code === null || (400 <= code && code < 600)) {
    return { ok: true, msg: `rejected/closed as expected. status=${code}` };
  }

  return { ok: false, msg: `unexpected success (status=${code})` };
}

/**
 * Test header with missing colon
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testMissingColon(host, port, timeout) {
  const raw = Buffer.from(
    "GET / HTTP/1.1\r\nHost: localhost\r\nBadHeaderLine withoutcolon\r\n\r\n"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const code = parseStatus(response);

  if (code === null || (400 <= code && code < 600)) {
    return { ok: true, msg: `rejected/closed as expected. status=${code}` };
  }

  return { ok: false, msg: `unexpected success (status=${code})` };
}

/**
 * Test duplicate Content-Length headers
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testDuplicateContentLength(host, port, timeout) {
  const raw = Buffer.from(
    "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\nContent-Length: 5\r\n\r\nabcdefghij"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const code = parseStatus(response);

  // We expect rejection or close; fail if 2xx
  if (code === null || (400 <= code && code < 600)) {
    return { ok: true, msg: `rejected/closed as expected. status=${code}` };
  }

  return { ok: false, msg: `unexpected success (status=${code})` };
}

/**
 * Test Content-Length and Transfer-Encoding headers together
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testContentLengthAndTransferEncoding(host, port, timeout) {
  const raw = Buffer.from(
    "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const code = parseStatus(response);

  if (code === null || (400 <= code && code < 600)) {
    return { ok: true, msg: `rejected/closed as expected. status=${code}` };
  }

  return { ok: false, msg: `unexpected success (status=${code})` };
}

/**
 * Test extremely long header value
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testExtremelyLongHeader(host, port, timeout) {
  const header = "A".repeat(200000);
  const raw = Buffer.from(
    `GET / HTTP/1.1\r\nHost: localhost\r\nX-Long: ${header}\r\n\r\n`
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const code = parseStatus(response);

  // Prefer 431 or other 4xx or connection close
  if (code === null || (400 <= code && code < 600)) {
    return { ok: true, msg: `rejected/closed as expected. status=${code}` };
  }

  return { ok: false, msg: `unexpected success (status=${code})` };
}

/**
 * Test obsolete header folding
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testObsFold(host, port, timeout) {
  const raw = Buffer.from(
    "GET / HTTP/1.1\r\nHost: localhost\r\nX-Fold: part1\r\n part2\r\n\r\n"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  // Naive check: we don't want hidden header parts to be echoed
  const responseStr = response ? response.toString() : "";
  const injected =
    responseStr.includes("part2") && responseStr.includes("X-Fold");
  const code = parseStatus(response);

  if (injected) {
    return {
      ok: false,
      msg: `folded content appears in response headers/body (potentially unsafe). status=${code}`,
    };
  }

  // Accept reject/close or sanitized handling
  if (code === null || (400 <= code && code < 600)) {
    return { ok: true, msg: `rejected/closed as expected. status=${code}` };
  }

  // If server responds 200 but didn't echo folded content, treat as pass but warn
  return {
    ok: true,
    msg: `server accepted but did not echo folded content. status=${code}`,
  };
}

/**
 * Test with randomly generated HTTP request
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @param {boolean} malicious - Whether to generate potentially malicious request
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testRandomFuzz(host, port, timeout, malicious = true) {
  // Generate random request components
  const method = randomMethod(malicious);
  const uri = randomURI(malicious);
  const version = randomHTTPVersion(malicious);

  // Decide if we want to make a request with a body
  const hasBody = Math.random() < 0.5;

  // Build headers
  const headers = [];

  // Always include Host header (required for HTTP/1.1)
  headers.push(
    `Host: ${
      [host, "localhost", randomString(10)][Math.floor(Math.random() * 3)]
    }`
  );

  // Add random number of headers
  const headerCount = Math.floor(Math.random() * 20) + 1;
  for (let i = 0; i < headerCount; i++) {
    const headerName = randomHeaderName(malicious);
    // Skip if we got an invalid header name that would break request format
    if (
      headerName.includes("\r") ||
      headerName.includes("\n") ||
      headerName.includes(":")
    ) {
      continue;
    }
    const headerValue = randomHeaderValue(malicious);
    headers.push(`${headerName}: ${headerValue}`);
  }

  // Add Content-Type and Content-Length for requests with body
  let contentType = null;
  let body = Buffer.from("");

  if (hasBody) {
    contentType = randomContentType();
    headers.push(`Content-Type: ${contentType}`);

    // Generate body before setting Content-Length to ensure correct length
    body = randomBody(contentType, malicious);

    // Decide whether to set accurate Content-Length
    const accurateLength = Math.random() < 0.8; // 80% accurate, 20% incorrect

    if (accurateLength) {
      headers.push(`Content-Length: ${body.length}`);
    } else {
      // Set incorrect Content-Length to test server's handling
      const fakeLength = Math.max(
        0,
        body.length + Math.floor(Math.random() * 2001) - 1000
      );
      headers.push(`Content-Length: ${fakeLength}`);
    }
  }

  // Build the raw request
  const requestLine = `${method} ${uri} ${version}\r\n`;
  const headerSection = headers.join("\r\n") + "\r\n\r\n";

  // Combine all parts into the raw request
  const rawRequest = Buffer.concat([
    Buffer.from(requestLine, "latin1"),
    Buffer.from(headerSection, "latin1"),
    body,
  ]);

  // Decide if we want to test slow request sending
  const slowSend = Math.random() < 0.1; // 10% chance of slow sending

  // Send the request and get response
  const { response, error } = await sendAndReceive(
    rawRequest,
    host,
    port,
    timeout,
    slowSend
  );

  if (error) {
    // Some connection errors are expected with malformed requests
    if (error.includes("CONNREFUSED")) {
      return { ok: false, msg: "connection refused (server down?)" };
    }
    return { ok: true, msg: `server closed connection: ${error}` };
  }

  // Parse status code
  const code = parseStatus(response);

  // For malicious requests, closing connection or returning error code is good
  if (malicious) {
    if (code === null) {
      return {
        ok: true,
        msg: "server closed connection (good for malicious input)",
      };
    } else if (400 <= (code || 0) && (code || 0) < 600) {
      return {
        ok: true,
        msg: `server rejected with error ${code} (good for malicious input)`,
      };
    } else {
      // Server accepted potentially dangerous request
      return {
        ok: true,
        msg: `server accepted potentially dangerous request with status ${code}`,
      };
    }
  } else {
    // For valid requests, we expect a successful response most of the time
    if (code === null) {
      return { ok: false, msg: "server closed connection unexpectedly" };
    } else if ((200 <= code && code < 300) || (300 <= code && code < 400)) {
      return { ok: true, msg: `server returned success/redirect ${code}` };
    } else {
      // Server rejected valid request - might be a false positive
      return {
        ok: true,
        msg: `server rejected with ${code} - might be overly strict`,
      };
    }
  }
}

/**
 * Test chunked encoding
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @param {boolean} malicious - Whether to generate potentially malicious chunked encoding
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testChunkedEncoding(host, port, timeout, malicious = true) {
  const method = "POST";
  const uri = "/";
  const version = "HTTP/1.1";

  const headers = [`Host: ${host}`, "Transfer-Encoding: chunked"];

  // Build chunked body
  let chunks = "";

  if (!malicious) {
    // Valid chunks
    for (let i = 0; i < Math.floor(Math.random() * 5) + 1; i++) {
      const chunkSize = Math.floor(Math.random() * 100) + 1;
      const chunkData = randomString(chunkSize);
      chunks += `${chunkSize.toString(16)}\r\n${chunkData}\r\n`;
    }
    // Final chunk
    chunks += "0\r\n\r\n";
  } else {
    // Malicious/invalid chunks
    const malformedOptions = [
      // Missing chunk size
      "\r\nchunk data\r\n0\r\n\r\n",
      // Invalid hex in chunk size
      "ZX\r\nchunk data\r\n0\r\n\r\n",
      // Chunk size too large
      "ffffffff\r\n" + "X".repeat(1000) + "\r\n0\r\n\r\n",
      // Incomplete chunk (missing final CRLF)
      "5\r\nhello",
      // Negative chunk size
      "-5\r\nhello\r\n0\r\n\r\n",
      // Chunk with chunk-extension
      "5;extension=value\r\nhello\r\n0\r\n\r\n",
      // Missing final chunk
      "5\r\nhello\r\n",
      // Very large number of tiny chunks
      Array(100).fill("1\r\nX\r\n").join("") + "0\r\n\r\n",
    ];
    chunks =
      malformedOptions[Math.floor(Math.random() * malformedOptions.length)];
  }

  // Build the raw request
  const requestLine = `${method} ${uri} ${version}\r\n`;
  const headerSection = headers.join("\r\n") + "\r\n\r\n";
  const rawRequest = Buffer.concat([
    Buffer.from(requestLine, "latin1"),
    Buffer.from(headerSection, "latin1"),
    Buffer.from(chunks, "latin1"),
  ]);

  // Send request and get response
  const { response, error } = await sendAndReceive(
    rawRequest,
    host,
    port,
    timeout
  );

  if (error) {
    return {
      ok: true,
      msg: `connection error (expected for malformed chunks): ${error}`,
    };
  }

  const code = parseStatus(response);

  if (malicious) {
    // For invalid chunking, we expect an error or connection close
    if (code === null) {
      return {
        ok: true,
        msg: "server closed connection (good for invalid chunks)",
      };
    } else if (400 <= (code || 0) && (code || 0) < 600) {
      return {
        ok: true,
        msg: `server rejected with error ${code} (good for invalid chunks)`,
      };
    } else {
      return {
        ok: false,
        msg: `server accepted invalid chunked encoding with status ${code}`,
      };
    }
  } else {
    // For valid chunking, we expect success
    if (code === null) {
      return { ok: false, msg: "server closed connection unexpectedly" };
    } else if (200 <= (code || 0) && (code || 0) < 300) {
      return {
        ok: true,
        msg: `server accepted valid chunked encoding with status ${code}`,
      };
    } else {
      return {
        ok: false,
        msg: `server rejected valid chunked encoding with status ${code}`,
      };
    }
  }
}

/**
 * Test HTTP protocol violations
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testHTTPProtocolViolation(host, port, timeout) {
  const violations = [
    // Request line with no space
    Buffer.from("GET/HTTP/1.1\r\nHost: localhost\r\n\r\n"),
    // No HTTP version
    Buffer.from("GET / \r\nHost: localhost\r\n\r\n"),
    // No method
    Buffer.from(" / HTTP/1.1\r\nHost: localhost\r\n\r\n"),
    // No URI
    Buffer.from("GET  HTTP/1.1\r\nHost: localhost\r\n\r\n"),
    // Extra spaces in request line
    Buffer.from("GET  /  HTTP/1.1\r\nHost: localhost\r\n\r\n"),
    // Missing request line ending
    Buffer.from("GET / HTTP/1.1Host: localhost\r\n\r\n"),
    // Missing final CRLF
    Buffer.from("GET / HTTP/1.1\r\nHost: localhost\r\n"),
    // No headers at all
    Buffer.from("GET / HTTP/1.1\r\n\r\n"),
    // Unicode in request line
    Buffer.from("GET /üñíçøðé HTTP/1.1\r\nHost: localhost\r\n\r\n", "utf8"),
    // HTTP/0.9 simple request (no headers, no version)
    Buffer.from("GET /\r\n\r\n"),
  ];

  const violation = violations[Math.floor(Math.random() * violations.length)];

  const { response, error } = await sendAndReceive(
    violation,
    host,
    port,
    timeout
  );

  if (error) {
    return {
      ok: true,
      msg: `connection error (expected for protocol violation): ${error}`,
    };
  }

  const code = parseStatus(response);

  // For protocol violations, we expect error status or connection close
  if (code === null) {
    return {
      ok: true,
      msg: "server closed connection (good for protocol violation)",
    };
  } else if (400 <= (code || 0) && (code || 0) < 600) {
    return {
      ok: true,
      msg: `server rejected with error ${code} (good for protocol violation)`,
    };
  } else {
    return {
      ok: false,
      msg: `server accepted protocol violation with status ${code}`,
    };
  }
}

/**
 * Test resource exhaustion attacks
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testResourceExhaustion(host, port, timeout) {
  const attackType = Math.floor(Math.random() * 4) + 1;
  let raw;

  if (attackType === 1) {
    // Very large request line
    const uri = "/" + "A".repeat(Math.floor(Math.random() * 40001) + 10000);
    raw = Buffer.from(`GET ${uri} HTTP/1.1\r\nHost: localhost\r\n\r\n`, "utf8");
  } else if (attackType === 2) {
    // Many headers
    const headers = ["Host: localhost"];
    for (let i = 0; i < Math.floor(Math.random() * 1001) + 1000; i++) {
      headers.push(`X-Header-${i}: ${randomString(10)}`);
    }
    raw = Buffer.from(
      "GET / HTTP/1.1\r\n" + headers.join("\r\n") + "\r\n\r\n",
      "utf8"
    );
  } else if (attackType === 3) {
    // Very large single header
    const headerValue = randomString(Math.floor(Math.random() * 50001) + 50000);
    raw = Buffer.from(
      `GET / HTTP/1.1\r\nHost: localhost\r\nX-Large: ${headerValue}\r\n\r\n`,
      "utf8"
    );
  } else {
    // Very large body with incorrect Content-Length
    const bodySize = Math.floor(Math.random() * 40001) + 10000;
    const body = randomBytes(bodySize);
    // Set Content-Length much larger than actual body
    raw = Buffer.concat([
      Buffer.from(
        `POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: ${
          bodySize * 10
        }\r\n\r\n`,
        "utf8"
      ),
      body,
    ]);
  }

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return {
      ok: true,
      msg: `connection error (expected for resource attack): ${error}`,
    };
  }

  const code = parseStatus(response);

  // For resource attacks, rejection is good
  if (code === null) {
    return {
      ok: true,
      msg: "server closed connection (good for resource attack)",
    };
  } else if ((code || 0) >= 400) {
    return {
      ok: true,
      msg: `server rejected with error ${code} (good for resource attack)`,
    };
  } else {
    return {
      ok: false,
      msg: `server accepted resource attack with status ${code}`,
    };
  }
}

/**
 * Test HTTP method handling
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testHTTPMethodHandling(host, port, timeout) {
  const methods = [
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
    "COPY",
    "MOVE",
    "PROPPATCH",
    "MKCOL",
    "LOCK",
    "UNLOCK",
    // Non-standard methods
    "PURGE",
    "REPORT",
    "SUBSCRIBE",
    "UNSUBSCRIBE",
    "ARBITRARY",
    // Made-up methods
    "HAMZAMETHOD",
    "TEST",
    "FUZZ",
    randomString(10).toUpperCase(),
  ];

  const method = methods[Math.floor(Math.random() * methods.length)];
  const raw = Buffer.from(
    `${method} / HTTP/1.1\r\nHost: localhost\r\n\r\n`,
    "utf8"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return { ok: false, msg: `connection error: ${error}` };
  }

  const code = parseStatus(response);

  // For standard methods (GET, POST, etc.), we expect success or redirect
  const standardMethods = [
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "HEAD",
    "OPTIONS",
    "PATCH",
  ];
  if (standardMethods.includes(method)) {
    if (code === null) {
      return {
        ok: false,
        msg: `server closed connection for standard method ${method}`,
      };
    } else if (200 <= (code || 0) && (code || 0) < 400) {
      return {
        ok: true,
        msg: `server accepted standard method ${method} with status ${code}`,
      };
    } else if ((code || 0) === 405) {
      return {
        ok: true,
        msg: `server rejected method ${method} with Method Not Allowed (expected if not implemented)`,
      };
    } else {
      return {
        ok: false,
        msg: `server rejected standard method ${method} with unexpected status ${code}`,
      };
    }
  } else {
    // For non-standard methods, acceptance or 501/405 rejection are both valid
    if (code === null) {
      return {
        ok: false,
        msg: `server closed connection for non-standard method ${method}`,
      };
    } else if (200 <= (code || 0) && (code || 0) < 300) {
      return {
        ok: true,
        msg: `server accepted non-standard method ${method} with status ${code}`,
      };
    } else if ((code || 0) === 405 || (code || 0) === 501) {
      return {
        ok: true,
        msg: `server rejected non-standard method ${method} with appropriate status ${code}`,
      };
    } else {
      return {
        ok: false,
        msg: `server rejected non-standard method ${method} with unexpected status ${code}`,
      };
    }
  }
}

/**
 * Test HTTP version handling
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @returns {Promise<{ok: boolean, msg: string}>} - Test result
 */
async function testHTTPVersionHandling(host, port, timeout) {
  const versions = [
    "HTTP/0.9",
    "HTTP/1.0",
    "HTTP/1.1",
    "HTTP/2.0",
    // Invalid versions
    "HTTP/1.2",
    "HTTP/3.0",
    "HTTP/10.0",
    "HTTP/1",
    "HTTP/A.1",
    // Completely invalid
    "HTTPX/1.1",
    "NotHTTP/1.1",
    randomString(8),
  ];

  const version = versions[Math.floor(Math.random() * versions.length)];
  const raw = Buffer.from(
    `GET / ${version}\r\nHost: localhost\r\n\r\n`,
    "utf8"
  );

  const { response, error } = await sendAndReceive(raw, host, port, timeout);

  if (error) {
    return {
      ok: true,
      msg: `connection error (expected for invalid version): ${error}`,
    };
  }

  const code = parseStatus(response);

  // Standard versions should be accepted
  const standardVersions = ["HTTP/1.0", "HTTP/1.1"];
  if (standardVersions.includes(version)) {
    if (code === null) {
      return {
        ok: false,
        msg: `server closed connection for standard version ${version}`,
      };
    } else if (200 <= (code || 0) && (code || 0) < 400) {
      return {
        ok: true,
        msg: `server accepted standard version ${version} with status ${code}`,
      };
    } else {
      return {
        ok: false,
        msg: `server rejected standard version ${version} with status ${code}`,
      };
    }
  } else {
    // Non-standard versions should be rejected or handled safely
    if (code === null) {
      return {
        ok: true,
        msg: `server closed connection for non-standard version ${version} (good)`,
      };
    } else if (400 <= (code || 0) && (code || 0) < 600) {
      return {
        ok: true,
        msg: `server rejected non-standard version ${version} with appropriate status ${code}`,
      };
    } else {
      return {
        ok: false,
        msg: `server accepted non-standard version ${version} with status ${code}`,
      };
    }
  }
}

// -------------------------
// Test execution functions
// -------------------------

// Map test name to function for original tests
const ORIGINAL_TESTS = [
  { name: "CRLF injection (response-splitting)", fn: testCRLFInjection },
  {
    name: "Percent-encoded newline (attempted bypass)",
    fn: testPercentEncodedNewline,
  },
  { name: "Header with NUL / control byte", fn: testNullByteInHeader },
  { name: "Missing colon in header line", fn: testMissingColon },
  {
    name: "Duplicate Content-Length (conflicting)",
    fn: testDuplicateContentLength,
  },
  {
    name: "Content-Length + Transfer-Encoding: chunked",
    fn: testContentLengthAndTransferEncoding,
  },
  {
    name: "Extremely long header value (header bomb)",
    fn: testExtremelyLongHeader,
  },
  { name: "Obs-fold / header folding", fn: testObsFold },
];

// New fuzzing tests
const FUZZING_TESTS = [
  { name: "HTTP Method Fuzzing", fn: testHTTPMethodHandling },
  { name: "HTTP Version Fuzzing", fn: testHTTPVersionHandling },
  { name: "HTTP Protocol Violation", fn: testHTTPProtocolViolation },
  { name: "Resource Exhaustion Attack", fn: testResourceExhaustion },
  {
    name: "Chunked Encoding Fuzzing",
    fn: (host, port, timeout) => testChunkedEncoding(host, port, timeout, true),
  },
  {
    name: "Valid Chunked Encoding",
    fn: (host, port, timeout) =>
      testChunkedEncoding(host, port, timeout, false),
  },
];

/**
 * Run a batch of tests concurrently
 * @param {Array} tests - Array of test objects with name and fn properties
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @param {number} concurrency - Maximum number of concurrent tests
 * @returns {Promise<boolean>} - Whether all tests passed
 */
async function runTestBatch(tests, host, port, timeout, concurrency) {
  let allOk = true;

  // Process tests in batches for controlled concurrency
  for (let i = 0; i < tests.length; i += concurrency) {
    const batch = tests.slice(i, i + concurrency);
    const promises = batch.map(async (test) => {
      try {
        const { ok, msg } = await test.fn(host, port, timeout);
        resultLine(test.name, ok, msg);
        return ok;
      } catch (e) {
        resultLine(test.name, false, `exception during test: ${e.message}`);
        return false;
      }
    });

    const results = await Promise.all(promises);

    if (results.some((ok) => !ok)) {
      allOk = false;
    }
  }

  return allOk;
}

/**
 * Run random fuzz tests
 * @param {string} host - Server host
 * @param {number} port - Server port
 * @param {number} timeout - Timeout in milliseconds
 * @param {number} count - Number of tests to run
 * @param {boolean} malicious - Whether to generate malicious requests
 * @param {number} concurrency - Maximum number of concurrent tests
 * @returns {Promise<boolean>} - Whether all tests passed
 */
async function runRandomFuzzTests(
  host,
  port,
  timeout,
  count,
  malicious = true,
  concurrency = DEFAULT_CONCURRENCY
) {
  const prefix = malicious ? "Malicious" : "Random valid";
  console.log(
    `\nRunning ${count} ${prefix.toLowerCase()} fuzz tests (concurrency: ${concurrency})...\n`
  );

  let passed = 0;
  const total = count;

  // Process tests in batches for controlled concurrency
  for (let i = 0; i < count; i += concurrency) {
    const batchSize = Math.min(concurrency, count - i);
    const batch = Array(batchSize)
      .fill()
      .map((_, idx) => ({
        name: `${prefix} Fuzz Test #${i + idx + 1}`,
        fn: () => testRandomFuzz(host, port, timeout, malicious),
      }));

    const promises = batch.map(async (test) => {
      try {
        const { ok, msg } = await test.fn();
        resultLine(test.name, ok, msg);
        return ok;
      } catch (e) {
        resultLine(test.name, false, `exception during test: ${e.message}`);
        return false;
      }
    });

    const results = await Promise.all(promises);
    passed += results.filter(Boolean).length;
  }

  console.log(`\n${passed}/${total} ${prefix.toLowerCase()} fuzz tests passed`);
  return passed === total;
}

// -------------------------
// Main program
// -------------------------

/**
 * Simple command line argument parser
 * @returns {Object} - Parsed options
 */
function parseArgs() {
  const args = process.argv.slice(2);
  const options = {
    host: DEFAULT_HOST,
    port: DEFAULT_PORT,
    timeout: DEFAULT_TIMEOUT_MS,
    tests: DEFAULT_FUZZ_COUNT,
    concurrency: DEFAULT_CONCURRENCY,
    mode: "all",
  };

  // Parse arguments
  for (let i = 0; i < args.length; i++) {
    const arg = args[i];

    if (arg === "--host" && i + 1 < args.length) {
      options.host = args[++i];
    } else if (arg === "--port" && i + 1 < args.length) {
      options.port = parseInt(args[++i], 10);
    } else if (arg === "--timeout" && i + 1 < args.length) {
      options.timeout = parseInt(args[++i], 10);
    } else if (arg === "--tests" && i + 1 < args.length) {
      options.tests = parseInt(args[++i], 10);
    } else if (arg === "--concurrency" && i + 1 < args.length) {
      options.concurrency = parseInt(args[++i], 10);
    } else if (arg === "--mode" && i + 1 < args.length) {
      const mode = args[++i];
      if (["all", "original", "fuzz", "random", "malicious"].includes(mode)) {
        options.mode = mode;
      } else {
        console.error(`Invalid mode: ${mode}. Using default mode: all`);
      }
    } else if (arg === "--help" || arg === "-h") {
      console.log(`
HTTP Server Fuzzer
Usage: node ${path.basename(process.argv[1])} [options]

Options:
  --host <host>           Server host (default: ${DEFAULT_HOST})
  --port <port>           Server port (default: ${DEFAULT_PORT})
  --timeout <ms>          Socket timeout in milliseconds (default: ${DEFAULT_TIMEOUT_MS})
  --tests <count>         Number of random fuzz tests to run (default: ${DEFAULT_FUZZ_COUNT})
  --concurrency <count>   Maximum number of concurrent tests (default: ${DEFAULT_CONCURRENCY})
  --mode <mode>           Test mode: all, original, fuzz, random, malicious (default: all)
  --help, -h              Show this help message
      `);
      process.exit(0);
    }
  }

  return options;
}

// Parse command line arguments
const options = parseArgs();

// Main function
async function main() {
  // Determine which tests to run based on mode
  const originalEnabled = ["all", "original"].includes(options.mode);
  const fuzzEnabled = ["all", "fuzz"].includes(options.mode);
  const randomEnabled = ["all", "random", "fuzz"].includes(options.mode);
  const maliciousEnabled = ["all", "malicious", "fuzz"].includes(options.mode);

  // Print test configuration
  console.log("HTTP Server Fuzzer");
  console.log("=================");
  console.log(`Target: ${options.host}:${options.port}`);
  console.log(`Timeout: ${options.timeout} ms`);
  console.log(`Test mode: ${options.mode}`);
  console.log(`Concurrency: ${options.concurrency} requests`);

  if (randomEnabled || maliciousEnabled) {
    console.log(`Random tests: ${options.tests}`);
  }
  console.log();

  let allOk = true;

  // Run original tests if enabled
  if (originalEnabled) {
    console.log(
      `Running ${ORIGINAL_TESTS.length} original bad-header tests...\n`
    );
    const ok = await runTestBatch(
      ORIGINAL_TESTS,
      options.host,
      options.port,
      options.timeout,
      options.concurrency
    );
    if (!ok) allOk = false;
  }

  // Run fuzzing tests if enabled
  if (fuzzEnabled) {
    console.log(
      `\nRunning ${FUZZING_TESTS.length} fuzzing test categories...\n`
    );
    const ok = await runTestBatch(
      FUZZING_TESTS,
      options.host,
      options.port,
      options.timeout,
      options.concurrency
    );
    if (!ok) allOk = false;
  }

  // Run random valid tests if enabled
  if (randomEnabled) {
    const ok = await runRandomFuzzTests(
      options.host,
      options.port,
      options.timeout,
      options.tests,
      false,
      options.concurrency
    );
    if (!ok) allOk = false;
  }

  // Run malicious tests if enabled
  if (maliciousEnabled) {
    const ok = await runRandomFuzzTests(
      options.host,
      options.port,
      options.timeout,
      options.tests,
      true,
      options.concurrency
    );
    if (!ok) allOk = false;
  }

  // Print summary
  console.log("\nSummary:");
  if (allOk) {
    console.log(
      "All tests PASSED! Your HTTP server appears to handle malformed inputs safely."
    );
    return 0;
  } else {
    console.log(
      "One or more tests FAILED. Review the failing cases to improve server robustness."
    );
    return 2;
  }
}

// Run the main function
main()
  .then((code) => process.exit(code))
  .catch((err) => {
    console.error("Fatal error:", err);
    process.exit(1);
  });
