//by sakara
#include "dot_product_db_config.h"
#include "share/Spdz2kShare.h"
#include "protocols/Circuit.h"
#include "utils/print_vector.h"
#include "utils/rand.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <random>
#include <iomanip>
#include <unordered_set>
#include <algorithm>

using namespace std;
using namespace bioauth;
using namespace bioauth::experiments::dot_product;

constexpr int PARTY_ID = 0;

struct NetworkConfig {
    std::string name;
    int delay_ms;       
    int bandwidth_mbps; 
    std::string description;
    
    NetworkConfig(const std::string& n, int delay, int bw, const std::string& desc)
        : name(n), delay_ms(delay), bandwidth_mbps(bw), description(desc) {}
};

void setupNetwork(const NetworkConfig& config) {
    if (config.delay_ms == 0) {
        int result = system("sudo tc qdisc del dev lo root 2>/dev/null");
        if (result != 0) {
            std::cerr << "Warning: Failed to reset network configuration" << std::endl;
        }
        return;
    }
    
    std::string cmd = 
        "sudo tc qdisc del dev lo root 2>/dev/null; "
        "sudo tc qdisc add dev lo root handle 1: netem delay " + 
        std::to_string(config.delay_ms) + "ms; "
        "sudo tc qdisc add dev lo parent 1:1 handle 10: tbf rate " + 
        std::to_string(config.bandwidth_mbps) + "mbit burst 32kbit latency 50ms";
    
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "Warning: Failed to setup network configuration for " << config.name << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void cleanupNetwork() {
    int result = system("sudo tc qdisc del dev lo root 2>/dev/null");
    if (result != 0) {
        std::cerr << "Warning: Failed to cleanup network configuration" << std::endl;
    }
}

int main() {
    std::vector<NetworkConfig> networks = {
        NetworkConfig("LAN", 1, 1000, "LAN: RTT 2ms, 1Gbps"),
        NetworkConfig("MAN", 10, 100, "MAN: RTT 20ms, 100Mbps"),
        NetworkConfig("WAN", 50, 50, "WAN: RTT 100ms, 50Mbps"),
    };
    
    using ShrType = Spdz2kShare64;
    using ClearType = ShrType::ClearType;
    
    std::cout << "=== Party 0 online ===" << std::endl;
    std::cout << "Vector length: " << dim << std::endl;
    std::cout << "Database size: " << dbsize << std::endl;
    std::cout << std::endl;
    
    for (const auto& net : networks) {
        try {
            std::cout << "Testing network: " << net.description << std::endl;
            setupNetwork(net);
            
            PartyWithFakeOffline<ShrType> party(0, 2, 5050, kJobName);
            Circuit<ShrType> circuit(party);
            
            auto a = circuit.input(0, 1, dim);
            auto b = circuit.input(1, dim, dbsize);
            auto c = circuit.multiply(a, b);
            auto d = circuit.output(c);
            circuit.addEndpoint(d);
            
            std::vector<ClearType> vec_a(dim);
            std::generate(vec_a.begin(), vec_a.end(), [] { 
                return getRand<ClearType>(); 
            });
            a->setInput(vec_a);
            
            circuit.readOfflineFromFile();
            circuit.runOnlineWithBenckmark();
            
            std::cout << "--------- Online Phase ---------" << std::endl;
            double comm0 = party.bytes_sent_actual() / 1024.0 ;
            double time0 = circuit.timer().elapsed();
            
            std::cout << "[" << net.name << "] Time: " << std::fixed 
                     << std::setprecision(2) << time0 
                     << " ms, Communication: " << std::fixed 
                     << std::setprecision(2) << comm0 << " KB, " << comm0/1024.0<<"MB"<<std::endl;
                     
        } catch (const std::exception& e) {
            std::cout << "Error [" << net.name << "]: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    cleanupNetwork();
    return 0;
}