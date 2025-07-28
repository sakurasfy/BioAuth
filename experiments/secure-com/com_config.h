//by sakara
#ifndef COM_CONFIG_H
#define COM_CONFIG_H

#include <string>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <iostream>

/**
 * Network configuration structure for FSS experiments
 * Defines network parameters for different network types (LAN/MAN/WAN)
 */
struct NetworkConfig {
    std::string name;
    int delay_ms;       
    int bandwidth_mbps; 
    std::string description;
    
    NetworkConfig(const std::string& n, int delay, int bw, const std::string& desc)
        : name(n), delay_ms(delay), bandwidth_mbps(bw), description(desc) {}
};

/**
 * Setup network configuration using Linux traffic control (tc)
 * Requires sudo privileges to modify network settings
 * 
 * @param config Network configuration to apply
 */
inline void setupNetwork(const NetworkConfig& config) {
    if (config.delay_ms == 0) {
        system("sudo tc qdisc del dev lo root 2>/dev/null");
        return;
    }
    
    std::string cmd = 
        "sudo tc qdisc del dev lo root 2>/dev/null; "
        "sudo tc qdisc add dev lo root handle 1: netem delay " + 
        std::to_string(config.delay_ms) + "ms; "
        "sudo tc qdisc add dev lo parent 1:1 handle 10: tbf rate " + 
        std::to_string(config.bandwidth_mbps) + "mbit burst 32kbit latency 50ms";
    
    system(cmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

/**
 * Clean up network configuration by removing all tc rules
 */
inline void cleanupNetwork() {
    system("sudo tc qdisc del dev lo root 2>/dev/null");
}

/**
 * Get predefined network configurations for testing
 * 
 * @return Vector of network configurations (LAN, MAN, WAN)
 */
inline std::vector<NetworkConfig> getNetworkConfigurations() {
    return {
        NetworkConfig("LAN", 1, 1000, "LAN: RTT 2ms, 1Gbps"),
        NetworkConfig("MAN", 10, 100, "MAN: RTT 20ms, 100Mbps"),
        NetworkConfig("WAN", 50, 50, "WAN: RTT 100ms, 50Mbps"),
    };
}

/**
 * Print network configuration test header
 * 
 * @param net Network configuration being tested
 */
inline void printNetworkTestHeader(const NetworkConfig& net) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Testing network configuration: " << net.description << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Network: " << net.name << " (" << net.description << ")" << std::endl;
}

/**
 * Print final test completion message
 */
inline void printTestCompletion() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "All network configurations tested" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

/**
 * Handle network cleanup and inter-test delay
 */
inline void handleTestCleanup() {
    cleanupNetwork();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

/**
 * Common FSS test configuration constants
 */
namespace FSS_Config {
    constexpr size_t DEFAULT_ROUNDS = 256;
    constexpr uint32_t FSS_NUM_BITS = 10;
    constexpr uint32_t FSS_NUM_PARTIES = 2;
    constexpr uint64_t TEST_VALUE_A = 3;
    constexpr uint64_t TEST_VALUE_B0 = 1;
    constexpr uint64_t TEST_VALUE_B1 = -1;
}

#endif // COM_CONFIG_H