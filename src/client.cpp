#include "motd_client.hpp"
#include <iostream>
#include <sstream>
#include <boost/bind/bind.hpp>
#include <boost/json.hpp>

namespace json = boost::json;

MOTDClient::MOTDClient(const std::string& host, const std::string& port, bool verify_ssl)
    : host_(host), port_(port), verify_ssl_(verify_ssl) {}

std::string MOTDClient::get_motd() {
    try {
        std::string response = send_request("GET", "/motd");
        
        // Parse JSON response
        size_t body_start = response.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            std::string body = response.substr(body_start + 4);
            json::object json_response = json::parse(body).as_object();
            return json::value_to<std::string>(json_response.at("motd"));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting MOTD: " << e.what() << std::endl;
    }
    return "";
}

bool MOTDClient::set_motd(const std::string& motd) {
    try {
        json::object request;
        request["motd"] = motd;
        std::string request_body = json::serialize(request);
        
        std::string response = send_request("PUT", "/motd", request_body);
        
        // Parse JSON response
        size_t body_start = response.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            std::string body = response.substr(body_start + 4);
            json::object json_response = json::parse(body).as_object();
            if (json_response.count("status") > 0) {
                auto status = json::value_to<std::string>(json_response.at("status"));
                if (status == "success") {
                    return true;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error setting MOTD: " << e.what() << std::endl;
    }
    return false;
}

std::string MOTDClient::send_request(const std::string& method, const std::string& path,
                                     const std::string& body) {
    boost::asio::io_context io_context;
    
    // Create SSL context
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::single_dh_use);
    ctx.use_certificate_chain_file("certs/client.crt");
    ctx.use_private_key_file("certs/client.key", ssl::context::pem);
    
    if (!verify_ssl_) {
        ctx.set_verify_mode(ssl::context::verify_none);
    } else {
        ctx.load_verify_file("certs/ca.crt");
        ctx.set_verify_mode(ssl::context::verify_peer);
    }
    
    // Resolve host
    tcp::resolver resolver(io_context);
    auto results = resolver.resolve(host_, port_);
    
    // Create and connect socket
    ssl::stream<tcp::socket> socket(io_context, ctx);
    socket.lowest_layer().connect(*results.begin());
    
    // Perform SSL handshake
    socket.handshake(ssl::stream_base::client);
    
    // Build HTTP request
    std::ostringstream request_stream;
    request_stream << method << " " << path << " HTTP/1.1\r\n"
                   << "Host: " << host_ << ":" << port_ << "\r\n"
                   << "Content-Type: application/json\r\n"
                   << "Content-Length: " << body.length() << "\r\n"
                   << "Connection: close\r\n"
                   << "\r\n";
    
    if (!body.empty()) {
        request_stream << body;
    }
    
    std::string request = request_stream.str();
    
    // Send request
    boost::asio::write(socket, boost::asio::buffer(request));
    
    // Read response
    std::string response;
    std::array<char, 4096> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);
    
    while (len > 0 || error == boost::asio::error::eof) {
        response.append(buf.begin(), buf.begin() + len);
        if (error == boost::asio::error::eof) {
            break;
        }
        len = socket.read_some(boost::asio::buffer(buf), error);
    }
    
    // Shutdown SSL connection
    socket.shutdown();
    
    return response;
}

// Main client program
int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cout << "Usage: motd-client [--get|--set message]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --get              Get the current message of the day" << std::endl;
            std::cout << "  --set MESSAGE      Set a new message of the day" << std::endl;
            std::cout << "  --insecure         Disable SSL certificate verification" << std::endl;
            return 1;
        }
        
        std::string host = "localhost";
        std::string port = "8443";
        bool verify_ssl = true;
        std::string command = argv[1];
        
        // Parse arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--insecure") {
                verify_ssl = false;
            }
        }
        
        MOTDClient client(host, port, verify_ssl);
        
        if (command == "--get") {
            std::string motd = client.get_motd();
            if (!motd.empty()) {
                std::cout << "Current MOTD: " << motd << std::endl;
            } else {
                std::cerr << "Failed to retrieve MOTD" << std::endl;
                return 1;
            }
        } else if (command == "--set") {
            if (argc < 3) {
                std::cerr << "Error: --set requires a message argument" << std::endl;
                return 1;
            }
            
            std::string message = argv[2];
            
            if (client.set_motd(message)) {
                std::cout << "MOTD successfully updated to: " << message << std::endl;
            } else {
                std::cerr << "Failed to set MOTD" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            return 1;
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
