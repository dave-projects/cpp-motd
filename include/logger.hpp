#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <string>

class Logger {
private:
    std::ofstream log_file;
    std::mutex log_mutex;

public:
    Logger(const std::string& filename) {
        log_file.open(filename, std::ios::app);
    }

    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    // Function to log a message of the day request
    void log(const std::string& client_ip, const std::string& method, 
             const std::string& path, int status_code, 
             const std::string& details) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        // Get current timestamp in ISO 8601 format
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream timestamp;
        timestamp << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
        
        std::stringstream log_entry;
        log_entry << timestamp.str() 
                  << " | CLIENT_IP: " << client_ip 
                  << " | METHOD: " << method 
                  << " | PATH: " << path 
                  << " | STATUS: " << status_code 
                  << " | DETAILS: " << details;
        
        if (log_file.is_open()) {
            log_file << log_entry.str() << std::endl;
            log_file.flush();
        }
    }

    // General purpose log function
    void info(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream timestamp;
        timestamp << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
        
        if (log_file.is_open()) {
            log_file << timestamp.str() << " | " << message << std::endl;
            log_file.flush();
        }
    }
};

#endif // LOGGER_HPP
