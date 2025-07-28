//by sakara
#include <chrono>
#include <iostream>
#include <iomanip>
#include "networking/fss-common.h"
#include "networking/fss-client.h"
#include "com_config.h"

using namespace std;

int main()
{
    auto networks = getNetworkConfigurations();
    
    cout << "=== Secure comparison Party_0 ===" << endl;
    
    for (const auto& net : networks) {
        try {
            printNetworkTestHeader(net);
            setupNetwork(net);
            
            // Set up variables using config constants
            Fss fClient;
            ServerKeyReLU relu_k0_0;
            ServerKeyReLU relu_k1_0;
            ServerKeyReLU relu_k0_1;
            ServerKeyReLU relu_k1_1;
            
            initializeClient(&fClient, FSS_Config::FSS_NUM_BITS, FSS_Config::FSS_NUM_PARTIES);
            
            cout << "Starting offline phase ..." << endl;
            auto t_begin = chrono::high_resolution_clock::now();
            size_t rounds = FSS_Config::DEFAULT_ROUNDS;
            
            for(size_t i = 0; i < rounds; i++) {
                generateKeyReLu(&fClient, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1);
                generateKeyReLu(&fClient, &relu_k0_0, &relu_k0_1, &relu_k1_0, &relu_k1_1);
            }
            
            auto t_end = chrono::high_resolution_clock::now();
            
            // Calculate performance metrics
            double total_time = chrono::duration<double, milli>(t_end - t_begin).count();
            double offline_com = sizeof(ServerKeyReLU) * rounds * 12 / (1024.0 * 1024.0); // MB
            double throughput = (rounds * 2 * 1000.0) / total_time; // key-pairs per second
            
            cout << "\n--- Results for " << net.name << " ---" << endl;
            cout << "Rounds: " << rounds << endl;
            cout << "Total time: " << fixed << setprecision(2) << total_time << " ms" << endl;
            cout << "Offline communication: " << fixed << setprecision(2) << offline_com << " MB" << endl;
        
            double time_in_seconds = total_time / 1000.0;
            double effective_bandwidth = (offline_com * 8) / time_in_seconds; // Mbps
            double bandwidth_utilization = (effective_bandwidth / net.bandwidth_mbps) * 100.0;
            
            //cout << "Effective bandwidth usage: " << fixed << setprecision(2) << effective_bandwidth << " Mbps" << endl;
            //cout << "Bandwidth utilization: " << fixed << setprecision(1) << bandwidth_utilization << "%" << endl;
            
            cout << "Client (Party 0) completed key generation phase for " << net.name << endl;
            
        } catch (const exception& e) {
            cout << "Error [" << net.name << "]: " << e.what() << endl;
        }
        
        handleTestCleanup();
    }
    
    printTestCompletion();
    
    return 0;
}