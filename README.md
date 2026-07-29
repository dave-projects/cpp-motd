# cpp-motd: HTTPS REST Message of the Day Server

A C++ HTTPS REST server that provides a message of the day (MOTD) resource with persistent storage and comprehensive logging.

## Features

- **HTTPS REST API** using Boost.ASIO, Boost.Beast, and Boost.JSON
- **GET Method**: Retrieve the current message of the day
- **PUT Method**: Update the message of the day
- **Persistent Storage**: MOTD stored in file for persistence across server restarts
- **Comprehensive Logging**: Activity logs with timestamps, client IP, access type, and modification details
- **C++ Client**: Command-line client for interacting with the server
- **Linux Compatible**: Built with standard Linux development tools

## Project Structure

```
cpp-motd/
├── Makefile
├── README.md
├── src/
│   ├── server.cpp           # HTTPS REST server implementation
│   ├── client.cpp           # HTTPS REST client implementation
│   └── logger.hpp           # Logging utility header
├── include/
│   ├── motd_server.hpp      # Server interface
│   └── motd_client.hpp      # Client interface
├── data/
│   └── motd.txt             # Default MOTD file (persisted)
└── logs/
    └── motd.log             # Server activity log
```

## Prerequisites

- GCC/G++ (C++17 or later)
- Boost libraries (asio, beast, system, ssl, json)
- OpenSSL development libraries
- Make

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential libboost-all-dev libssl-dev
```

## Building

```bash
make build
```

This will compile:
- `bin/motd-server` - The HTTPS REST server
- `bin/motd-client` - The HTTPS client

## Running the Server

```bash
./bin/motd-server
```

The server will:
- Listen on `https://localhost:8443` (where the port is configurable)
- Store MOTD in `data/motd.txt`
- Log activity to `logs/motd.log`
- Require a valid client certificate signed by the local CA

Generate development certificates before running:

```bash
make certs
```

## Using the Client

### Get the current MOTD

```bash
./bin/motd-client -g
```

### Set a new MOTD

```bash
./bin/motd-client -s "Your new message here"
```

## API Endpoints

### GET /motd

Retrieve the current message of the day.

**Response:**
```json
{
  "motd": "Welcome to the system!"
}
```

### PUT /motd

Update the message of the day.

**Request Body:**
```json
{
  "motd": "New message of the day"
}
```

**Response:**
```json
{
  "status": "success",
  "motd": "New message of the day"
}
```

## Logging

The server logs all activity to `logs/motd.log` with entries including:
- Timestamp (ISO 8601 format)
- Client IP address
- Access type (GET/PUT)
- Details of any modifications
- HTTP status codes

### Log Entry Format

```
2026-07-27T14:30:45.123 | CLIENT_IP: 127.0.0.1 | METHOD: GET | PATH: /motd | STATUS: 200 | DETAILS: Retrieved MOTD
2026-07-27T14:30:50.456 | CLIENT_IP: 127.0.0.1 | METHOD: PUT | PATH: /motd | STATUS: 200 | DETAILS: Updated MOTD to 'New message'
```

## Building and Testing

```bash
# Build all targets
make build

# Run the server in background
./bin/motd-server &

# Get the current MOTD
./bin/motd-client -g

# Set a new MOTD
./bin/motd-client -s "System maintenance window: 2-4 PM UTC"

# Get the updated MOTD
./bin/motd-client -g

# Check the logs
cat logs/motd.log

# Stop the server
killall motd-server
```

## Clean Up

```bash
make clean      # Remove build artifacts
make distclean  # Remove build artifacts and generated certificates/logs
```

## Security Considerations

- **Development**: The server generates self-signed certificates for development purposes
- **Production**: Replace certificates with proper CA-signed certificates
- **HTTPS**: All communications are encrypted over HTTPS (TLS 1.2+)

## License

MIT
