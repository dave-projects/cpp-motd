#ifndef MOTD_CLIENT_HPP
#define MOTD_CLIENT_HPP

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

class MOTDClient {
public:
    MOTDClient(const std::string& host, const std::string& port, bool verify_ssl = true);
    
    std::string get_motd();
    bool set_motd(const std::string& motd);
    
private:
    std::string host_;
    std::string port_;
    bool verify_ssl_;
    
    std::string send_request(const std::string& method, const std::string& path,
                            const std::string& body = "");
};

#endif // MOTD_CLIENT_HPP
