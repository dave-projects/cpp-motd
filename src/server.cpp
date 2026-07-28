#include "motd_server.hpp"
#include "logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/json.hpp>
#include <memory>

namespace json = boost::json;

class ConnectionHandler : public boost::enable_shared_from_this<ConnectionHandler> {
public:
    typedef boost::shared_ptr<ConnectionHandler> pointer;
    
    static pointer create(boost::asio::io_context& io_context,
                         ssl::context& context,
                         std::shared_ptr<Logger> logger,
                         std::string& motd_file) {
        return pointer(new ConnectionHandler(io_context, context, logger, motd_file));
    }
    
    ssl::stream<tcp::socket>::lowest_layer_type& socket() {
        return ssl_socket_.lowest_layer();
    }
    
    void start() {
        ssl_socket_.async_handshake(ssl::stream_base::server,
            boost::bind(&ConnectionHandler::handle_handshake, shared_from_this(),
                boost::asio::placeholders::error));
    }

private:
    ConnectionHandler(boost::asio::io_context& io_context,
                     ssl::context& context,
                     std::shared_ptr<Logger> logger,
                     std::string& motd_file)
        : ssl_socket_(io_context, context),
          logger_(logger),
          motd_file_(motd_file) {}
    
    void handle_handshake(const boost::system::error_code& error) {
        if (!error) {
            std::cout << "SSL handshake successful" << std::endl;
            // Start reading data after successful handshake
            async_read_request();
        } else {
            std::cerr << "Handshake failed: " << error.message() << std::endl;
        }
    }
    
    void async_read_request() {
        // Use a streambuf for proper HTTP message handling
        boost::asio::async_read(ssl_socket_,
            buffer_,
            boost::asio::transfer_at_least(1),
            boost::bind(&ConnectionHandler::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
        std::cout << "handle_read called with error: " << error.message() << ", bytes: " << bytes_transferred << std::endl;
        
        if (!error && bytes_transferred > 0) {
            try {
                // Extract data from streambuf
                std::istream is(&buffer_);
                std::string request_str;
                std::getline(is, request_str); // Read first line
                
                std::string method, path, http_version;
                std::istringstream iss(request_str);
                iss >> method >> path >> http_version;
                
                std::cout << "Method: " << method << ", Path: " << path << std::endl;
                
                // Get client IP
                std::string client_ip = ssl_socket_.lowest_layer().remote_endpoint().address().to_string();
                
                // Read headers to find Content-Length
                std::string line;
                size_t content_length = 0;
                while (std::getline(is, line) && line != "\r") {
                    if (line.find("Content-Length:") != std::string::npos) {
                        content_length = std::stoi(line.substr(16));
                    }
                }
                
                // Read body if present
                std::string body;
                if (content_length > 0) {
                    body.resize(content_length);
                    is.read(&body[0], content_length);
                }
                
                // Handle requests
                std::string response;
                int status_code = 404;
                
                if (method == "GET" && path == "/motd") {
                    response = handle_get();
                    status_code = 200;
                    logger_->log(client_ip, method, path, status_code, "Retrieved MOTD");
                } else if (method == "PUT" && path == "/motd") {
                    std::string details;
                    response = handle_put(body, details);
                    status_code = 200;
                    logger_->log(client_ip, method, path, status_code, details);
                } else {
                    response = create_error_response(404, "Not Found");
                    logger_->log(client_ip, method, path, status_code, "Invalid request");
                }
                
                // Send response
                boost::asio::async_write(ssl_socket_,
                    boost::asio::buffer(response),
                    boost::bind(&ConnectionHandler::handle_write, shared_from_this(),
                        boost::asio::placeholders::error));
            } catch (const std::exception& e) {
                std::cerr << "Request processing error: " << e.what() << std::endl;
            }
        } else if (error) {
            std::cerr << "Read error: " << error.message() << std::endl;
        }
    }
    
    void handle_write(const boost::system::error_code& error) {
        if (error) {
            std::cerr << "Write error: " << error.message() << std::endl;
        }
    }
    
    std::string handle_get() {
        try {
            std::ifstream file(motd_file_);
            std::string motd;
            if (file.is_open()) {
                std::getline(file, motd);
                file.close();
            } else {
                motd = "Welcome to the system!";
            }
            
            json::object response;
            response["motd"] = motd;
            std::string body = json::serialize(response);
            
            return create_http_response(200, "OK", body);
        } catch (const std::exception& e) {
            return create_error_response(500, std::string("Error: ") + e.what());
        }
    }
    
    std::string handle_put(const std::string& body, std::string& details) {
        try {
            // Remove HTTP headers from body if present
            std::string clean_body = body;
            size_t pos = clean_body.find("\r\n\r\n");
            if (pos != std::string::npos) {
                clean_body = clean_body.substr(pos + 4);
            }
            
            // Parse JSON using Boost.JSON
            json::object request = json::parse(clean_body).as_object();
            std::string new_motd = json::value_to<std::string>(request.at("motd"));
            
            // Write to file
            std::ofstream file(motd_file_);
            file << new_motd;
            file.close();
            
            details = "Updated MOTD to '" + new_motd.substr(0, 50) + "'";
            if (new_motd.length() > 50) {
                details += "...";
            }
            
            json::object response;
            response["status"] = "success";
            response["motd"] = new_motd;
            std::string response_body = json::serialize(response);
            
            return create_http_response(200, "OK", response_body);
        } catch (const std::exception& e) {
            return create_error_response(400, std::string("Invalid request: ") + e.what());
        }
    }
    
    std::string create_http_response(int status_code, const std::string& status_text,
                                    const std::string& body) {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << body.length() << "\r\n"
            << "Connection: close\r\n"
            << "\r\n"
            << body;
        return oss.str();
    }
    
    std::string create_error_response(int status_code, const std::string& message) {
        json::object error;
        error["error"] = message;
        std::string body = json::serialize(error);
        return create_http_response(status_code, "Error", body);
    }
    
    ssl::stream<tcp::socket> ssl_socket_;
    std::shared_ptr<Logger> logger_;
    std::string& motd_file_;
    boost::asio::streambuf buffer_;
};

MOTDServer::MOTDServer(boost::asio::io_context& io_context, int port,
                       const std::string& cert_file, const std::string& key_file,
                       const std::string& motd_file, const std::string& log_file)
    : io_context_(io_context),
      motd_file_(motd_file),
      ssl_context_(std::make_unique<ssl::context>(ssl::context::sslv23)) {
    
    logger_ = std::make_shared<Logger>(log_file);
    logger_->info("Initializing MOTD server on port " + std::to_string(port));
    
    // Configure SSL context
    ssl_context_->set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::single_dh_use);
    ssl_context_->use_certificate_chain_file(cert_file);
    ssl_context_->use_private_key_file(key_file, ssl::context::pem);
    
    // Load initial MOTD
    current_motd_ = read_motd();
    
    // Create acceptor
    tcp::endpoint endpoint(tcp::v4(), port);
    acceptor_ = std::make_unique<tcp::acceptor>(io_context, endpoint);
    logger_->info("Server listening on port " + std::to_string(port));
}

void MOTDServer::start() {
    accept_connection();
}

void MOTDServer::stop() {
    if (acceptor_) {
        acceptor_->close();
    }
    logger_->info("Server stopped");
}

void MOTDServer::accept_connection() {
    ConnectionHandler::pointer new_connection =
        ConnectionHandler::create(io_context_, *ssl_context_, logger_, motd_file_);
    
    acceptor_->async_accept(new_connection->socket(),
        boost::bind(&MOTDServer::handle_connection, this,
            new_connection,
            boost::asio::placeholders::error));
}

void MOTDServer::handle_connection(ConnectionHandler::pointer connection,
                                   const boost::system::error_code& error) {
    if (!error) {
        connection->start();
    } else {
        logger_->info("Accept error: " + error.message());
    }
    accept_connection();
}

std::string MOTDServer::read_motd() {
    try {
        std::ifstream file(motd_file_);
        std::string motd;
        if (file.is_open()) {
            std::getline(file, motd);
            file.close();
            return motd;
        }
    } catch (const std::exception&) {}
    return "Welcome to the system!";
}

void MOTDServer::write_motd(const std::string& motd) {
    try {
        std::ofstream file(motd_file_);
        file << motd;
        file.close();
        current_motd_ = motd;
    } catch (const std::exception& e) {
        logger_->info(std::string("Error writing MOTD: ") + e.what());
    }
}

// Main server program
int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io_context;
        
        int port = 8443;
        if (argc > 1) {
            port = std::atoi(argv[1]);
        }
        
        MOTDServer server(io_context, port,
                         "certs/server.crt",
                         "certs/server.key",
                         "data/motd.txt",
                         "logs/motd.log");
        
        std::cout << "MOTD Server starting on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        server.start();
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
