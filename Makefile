# Makefile for cpp-motd HTTPS MOTD Server

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
INCLUDE_DIRS := -Iinclude
LIB_DIRS := 
LIBS := -lboost_system -lboost_thread -lboost_json -lboost_program_options -lssl -lcrypto -lpthread

SRC_DIR := src
INCLUDE_DIR := include
BIN_DIR := bin
OBJ_DIR := obj
DATA_DIR := data
LOG_DIR := logs

# Source files
SERVER_SRC := $(SRC_DIR)/server.cpp
CLIENT_SRC := $(SRC_DIR)/client.cpp

# Object files
SERVER_OBJ := $(OBJ_DIR)/server.o
CLIENT_OBJ := $(OBJ_DIR)/client.o

# Executables
SERVER_BIN := $(BIN_DIR)/motd-server
CLIENT_BIN := $(BIN_DIR)/motd-client

# Certificate files
CERT_DIR := certs
CA_KEY_FILE := $(CERT_DIR)/ca.key
CA_CERT_FILE := $(CERT_DIR)/ca.crt
SERVER_KEY_FILE := $(CERT_DIR)/server.key
SERVER_CERT_FILE := $(CERT_DIR)/server.crt
CLIENT_KEY_FILE := $(CERT_DIR)/client.key
CLIENT_CERT_FILE := $(CERT_DIR)/client.crt

# Default target
all: build

# Create necessary directories
$(BIN_DIR) $(OBJ_DIR) $(DATA_DIR) $(LOG_DIR):
	@mkdir -p $@

certs-dir:
	@mkdir -p $(CERT_DIR)

# Build all targets
build: directories $(SERVER_BIN) $(CLIENT_BIN)

# Server executable
$(SERVER_BIN): $(SERVER_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJ) -o $@ $(LIB_DIRS) $(LIBS)
	@echo "Server built: $@"

# Client executable
$(CLIENT_BIN): $(CLIENT_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJ) -o $@ $(LIB_DIRS) $(LIBS)
	@echo "Client built: $@"

# Server object file
$(SERVER_OBJ): $(SERVER_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Client object file
$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Directories target
directories: $(BIN_DIR) $(OBJ_DIR) $(DATA_DIR) $(LOG_DIR) certs-dir

# Generate CA, server, and client certificates for mTLS (development)
certs: $(CA_KEY_FILE) $(CA_CERT_FILE) $(SERVER_KEY_FILE) $(SERVER_CERT_FILE) $(CLIENT_KEY_FILE) $(CLIENT_CERT_FILE)

$(CA_KEY_FILE) $(CA_CERT_FILE): | certs-dir
	@echo "Generating CA certificate..."
	openssl req -x509 -newkey rsa:2048 -keyout $(CA_KEY_FILE) -out $(CA_CERT_FILE) \
		-days 365 -nodes -subj "/CN=cpp-motd-ca" 2>/dev/null

$(SERVER_KEY_FILE) $(SERVER_CERT_FILE): $(CA_KEY_FILE) $(CA_CERT_FILE) | certs-dir
	@echo "Generating server certificate signed by CA..."
	openssl req -newkey rsa:2048 -keyout $(SERVER_KEY_FILE) -out $(CERT_DIR)/server.csr \
		-nodes -subj "/CN=localhost" 2>/dev/null
	openssl x509 -req -in $(CERT_DIR)/server.csr -CA $(CA_CERT_FILE) -CAkey $(CA_KEY_FILE) \
		-CAcreateserial -out $(SERVER_CERT_FILE) -days 365 -sha256 2>/dev/null
	@rm -f $(CERT_DIR)/server.csr

$(CLIENT_KEY_FILE) $(CLIENT_CERT_FILE): $(CA_KEY_FILE) $(CA_CERT_FILE) | certs-dir
	@echo "Generating client certificate signed by CA..."
	openssl req -newkey rsa:2048 -keyout $(CLIENT_KEY_FILE) -out $(CERT_DIR)/client.csr \
		-nodes -subj "/CN=cpp-motd-client" 2>/dev/null
	openssl x509 -req -in $(CERT_DIR)/client.csr -CA $(CA_CERT_FILE) -CAkey $(CA_KEY_FILE) \
		-CAcreateserial -out $(CLIENT_CERT_FILE) -days 365 -sha256 2>/dev/null
	@rm -f $(CERT_DIR)/client.csr

# Clean build artifacts
clean:
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BIN_DIR)
	@echo "Build artifacts cleaned"

# Distclean: remove build artifacts and generated files
distclean: clean
	@rm -rf certs
	@rm -f $(LOG_DIR)/*.log
	@echo "All generated files cleaned"

# Help target
help:
	@echo "Available targets:"
	@echo "  make build      - Build the server and client"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make distclean  - Remove build artifacts and generated files"
	@echo "  make help       - Show this help message"

.PHONY: all build clean distclean directories certs certs-dir help
