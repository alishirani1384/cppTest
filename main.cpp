#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <netdb.h>
#include <cstring>
#include <iomanip>
#include <arpa/inet.h>

std::mutex mtx; 

std::string resolveDomain(const std::string& domain) {
    struct addrinfo hints, *res;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(domain.c_str(), NULL, &hints, &res) != 0) {
        return "Error";
    }

    
    void *addr;
    if (res->ai_family == AF_INET) { 
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
        addr = &(ipv4->sin_addr);
    } else { 
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
        addr = &(ipv6->sin6_addr);
    }

    inet_ntop(res->ai_family, addr, ipstr, sizeof ipstr);
    freeaddrinfo(res); 
    return std::string(ipstr);
}


std::vector<std::string> readDomains(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> domains;
    std::string line;

    if (file.is_open()) {
        getline(file, line); 
        while (getline(file, line)) {
            domains.push_back(line);
        }
        file.close();
    }
    return domains;
}


void updateCSV(const std::string& filename, const std::vector<std::string>& domains, const std::vector<std::string>& results, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(mtx); 
    std::ofstream file(filename, std::ios_base::app); 

    if (file.is_open()) {
        file << timestamp; 
        for (const auto& result : results) {
            file << "," << result; 
        }
        file << "\n"; 
        file.close();
    }
}

int main(int argc, char* argv[]) {
    std::string filename = "domains.csv";

    if (argc > 1) {
        filename = argv[1]; 
    }

    auto domains = readDomains(filename);
    
    std::vector<std::string> results(domains.size());
    
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M");
    std::string timestamp = ss.str();

    std::vector<std::thread> threads;
    
    for (size_t i = 0; i < domains.size(); ++i) {
        threads.emplace_back([&, i]() {
            results[i] = resolveDomain(domains[i]);
        });
    }

    for (auto& th : threads) {
        th.join(); 
    }

    updateCSV(filename, domains, results, timestamp); 

    return 0;
}