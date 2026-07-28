# Makefile for cpp-motd HTTPS MOTD Server

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
INCLUDE_DIRS := -Iinclude
LIB_DIRS := 
LIBS := -lboost_system -lboost_thread -lboost_json -lssl -lcrypto -lpthread

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
KEY_FILE := certs/server.key
CERT_FILE := certs/server.crt

# Default target
all: build

# Create necessary directories
$(BIN_DIR) $(OBJ_DIR) $(DATA_DIR) $(LOG_DIR) certs:
	@mkdir -p $@

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
directories: $(BIN_DIR) $(OBJ_DIR) $(DATA_DIR) $(LOG_DIR) certs

# Generate self-signed certificates (for development)
certs: $(KEY_FILE) $(CERT_FILE)

$(KEY_FILE) $(CERT_FILE): | certs
	@echo "Generating self-signed SSL certificates..."
	openssl req -x509 -newkey rsa:2048 -keyout $(KEY_FILE) -out $(CERT_FILE) \
		-days 365 -nodes -subj "/CN=localhost" 2>/dev/null || true

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

.PHONY: all build clean distclean directories help
