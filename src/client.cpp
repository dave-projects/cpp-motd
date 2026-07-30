#include "motd_client.hpp"
#include <iostream>
#include <sstream>
#include <boost/bind/bind.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>

namespace json = boost::json;
namespace po = boost::program_options;

MOTDClient::MOTDClient(const std::string& host, const std::string& port)
    : host_(host), port_(port) {}

// Function to get the current message of the day
std::string MOTDClient::get_motd() {
    try {
        std::string response = send_request("GET", "/motd");
        
        // Parse JSON response - we are looking to extract the motd value
        // from the JSON body returned in the HTTP response
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

// Function to set a new message of the day
bool MOTDClient::set_motd(const std::string& motd) {
    try {
        json::object request;
        request["motd"] = motd;
        std::string request_body = json::serialize(request);
        
        std::string response = send_request("PUT", "/motd", request_body);
        
        // Parse JSON response looking for the status in the JSON body
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

// Function to send an HTTP request
std::string MOTDClient::send_request(const std::string& method, const std::string& path,
                                     const std::string& body) {
    boost::asio::io_context io_context;
    
    // Create SSL context
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::single_dh_use);

    // Load the client certificate and key
    ctx.use_certificate_chain_file("certs/client.crt");
    ctx.use_private_key_file("certs/client.key", ssl::context::pem);
    
    // Load the CA
    ctx.load_verify_file("certs/ca.crt");
    ctx.set_verify_mode(ssl::context::verify_peer);
    
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

    std::string host, port, command, message;

    try {
        // Use boost::program_options to parse the command line options
        po::options_description description("Usage: motd-client [-h host] [-p port] -g|-s [message]");
        description.add_options()
            ("help,?", "Display this help message")
            ("version,v", "Show version")
            ("host,h", po::value<std::string>()->default_value("localhost"), "Host on which the server is running")
            ("port,p", po::value<int>()->default_value(8443), "Port to use")
            ("get,g",  "Command to get the current message of the day")
            ("set,s",  po::value<std::string>(), "Command to set a new message of the day");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << description;
            return 0;
        }

        if (vm.count("version")) {
            std::cout << "Version 1.0" << std::endl;
            return 0;
        }

        if (!vm.count("get") && !vm.count("set")) {
            std::cerr << "Error: a --get or --set command must be specified, for help use --help option" << std::endl;
            return 1;
        }

        if (vm.count("port")) {
            if ((vm["port"].as<int>() > 0) && (vm["port"].as<int>() < 65535)) {
                port = std::to_string(vm["port"].as<int>());
            }
            else {
                std::cerr << "Error: invalid port number" << std::endl;
                return 1;
            }
        }

        host = vm["host"].as<std::string>();

        MOTDClient client(host, port);
        
        if (vm.count("get")) {
            std::string motd = client.get_motd();
            if (!motd.empty()) {
                std::cout << "Current MOTD: " << motd << std::endl;
            } else {
                std::cerr << "Failed to retrieve MOTD" << std::endl;
                return 1;
            }
        } else if (vm.count("set")) {
            message = vm["set"].as<std::string>();

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
    } 
    catch (po::error const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
	    
    return 0;
}
