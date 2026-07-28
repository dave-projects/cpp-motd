#ifndef MOTD_SERVER_HPP
#define MOTD_SERVER_HPP

#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include "logger.hpp"

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

// Forward declaration
class ConnectionHandler;

class MOTDServer {
public:
    MOTDServer(boost::asio::io_context& io_context, int port,
               const std::string& cert_file, const std::string& key_file,
               const std::string& motd_file, const std::string& log_file);
    
    void start();
    void stop();
    
private:
    boost::asio::io_context& io_context_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::shared_ptr<Logger> logger_;
    std::string motd_file_;
    std::string current_motd_;
    
    ssl::context& get_ssl_context();
    void accept_connection();
    void handle_connection(boost::shared_ptr<ConnectionHandler> connection,
                          const boost::system::error_code& error);
    std::string read_motd();
    void write_motd(const std::string& motd);
    
    std::unique_ptr<ssl::context> ssl_context_;
};

#endif // MOTD_SERVER_HPP
