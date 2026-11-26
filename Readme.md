# HTTP Server in C

A lightweight HTTP server written from scratch in C using Linux/POSIX system calls and the Berkeley socket API.

The project explores the fundamentals of network programming, TCP sockets, HTTP request handling, file descriptors, and low-level file transmission. It serves static files directly from the filesystem without relying on external HTTP libraries or frameworks.

## Features

- **TCP Socket Programming** – Creates a TCP server using the Berkeley socket API.
- **IPv4 & IPv6 Support** – Uses `AF_UNSPEC` and `sockaddr_storage` to support both address families.
- **DNS Address Resolution** – Uses `getaddrinfo()` to resolve local server addresses.
- **HTTP Request Parsing** – Extracts the requested resource from an incoming HTTP request.
- **Static File Serving** – Serves files directly from the filesystem.
- **MIME Type Detection** – Determines the content type based on file extensions.
- **Efficient File Transmission** – Uses Linux `sendfile()` to transfer files directly from the filesystem to the socket.
- **HTTP Error Handling** – Returns a `404 Not Found` response when the requested file does not exist.

## Architecture

The server follows a simple request-response architecture.

```text
                 HTTP Client
               (Browser / curl)
                      |
                      | TCP Connection
                      v
                +-----------+
                |  Socket   |
                +-----------+
                      |
                      v
                +-----------+
                |  bind()   |
                |  listen() |
                +-----------+
                      |
                      v
                +-----------+
                |  accept() |
                +-----------+
                      |
                      v
                +-----------+
                |  recv()   |
                +-----------+
                      |
                      v
                +-----------+
                |  Request  |
                |  Parser   |
                +-----------+
                      |
                      v
                +-----------+
                |  File     |
                |  Handler  |
                +-----------+
                   /     \
                  /       \
             File Found  Not Found
                 |           |
                 v           v
             sendfile()   HTTP 404
                 |           |
                 +-----+-----+
                       |
                       v
                 Close Connection
```

## How It Works

### 1. Socket Initialization

The server uses `getaddrinfo()` to obtain suitable local socket addresses.

```c
struct addrinfo hints = {0};

hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

getaddrinfo(NULL, PORT, &hints, &res);
```

- `AF_UNSPEC` allows IPv4 or IPv6.
- `SOCK_STREAM` specifies a TCP socket.
- `AI_PASSIVE` requests addresses suitable for binding to local interfaces.

### 2. Binding and Listening

The server creates a socket and binds it to port `3490`.

```c
int sockfd = socket(
    res->ai_family,
    res->ai_socktype,
    res->ai_protocol
);

bind(sockfd, res->ai_addr, res->ai_addrlen);
listen(sockfd, BACKLOG);
```

`SO_REUSEADDR` is enabled to make restarting the server easier when the port is in a reusable state.

### 3. Accepting Client Connections

The server continuously listens for incoming TCP connections.

```c
int client_fd = accept(
    sockfd,
    (struct sockaddr *)&their_addr,
    &addr_size
);
```

The client's IP address and port are extracted using `inet_ntop()` and `ntohs()`.

### 4. Receiving HTTP Requests

The server receives the HTTP request using `recv()`.

```c
char buff[1024] = {0};

int bytes_recv = recv(
    client_fd,
    buff,
    sizeof(buff) - 1,
    0
);
```

For example, a browser requesting the root page sends a request similar to:

```http
GET / HTTP/1.1
Host: localhost:3490
Connection: keep-alive
```

### 5. Parsing the Request

The `req_parser()` function extracts the requested resource from the HTTP request.

Currently, it supports the following behavior:

| HTTP Path | Filesystem Resource |
|---|---|
| `/` | `index.html` |
| `/about.html` | `about.html` |
| `/image.jpg` | `image.jpg` |

The root path is mapped to `index.html`.

### 6. MIME Type Detection

The `get_mime_type()` function determines the response content type based on the file extension.

Currently supported:

| Extension | Content Type |
|---|---|
| `.html` | `text/html` |
| `.jpg` | `image/jpg` |
| `.jpeg` | `image/jpg` |

Unsupported extensions currently fall back to `text/html`.

### 7. Sending HTTP Responses

The `send_file()` function opens the requested file and retrieves its metadata using `fstat()`.

It constructs an HTTP response header containing:

- HTTP status code
- Content-Type
- Content-Length
- Connection information

Example:

```http
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1250
Connection: close
```

The response header is sent using `send()`.

### 8. Efficient File Transfer with sendfile()

Instead of reading the entire file into a userspace buffer and sending it manually, the server uses Linux's `sendfile()` system call.

```c
sendfile(client_fd, open_fd, 0, st.st_size);
```

This allows the kernel to transfer file data directly between file descriptors, avoiding an additional userspace read/write copy.

This is particularly useful when serving larger static files.

## Project Structure

```text
http-server/
│
├── server.c
├── index.html
├── about.html
├── images/
│   └── example.jpg
│
└── README.md
```

The server currently serves files relative to its working directory.

## Requirements

- Linux operating system
- GCC or another compatible C compiler
- POSIX socket API
- Linux `sendfile()` support

## Build

Clone the repository:

```bash
git clone https://github.com/risvn/http-server.git
cd http-server
```

Compile:

```bash
gcc server.c -o server
```

## Run

Start the server:

```bash
./server
```

Expected output:

```text
Server running on port 3490
```

Open the server in your browser:

```text
http://localhost:3490
```

test using curl:

```bash
curl -v http://localhost:3490/
```

Request a file:

```bash
curl -v http://localhost:3490/index.html
```

